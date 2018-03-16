#include "overviewpage.h"
#include "ui_overviewpage.h"

#include "walletmodel.h"
#include "bitcoinunits.h"
#include "optionsmodel.h"
#include "transactiontablemodel.h"
#include "transactionfilterproxy.h"
#include "guiutil.h"
#include "guiconstants.h"
#include "askpassphrasedialog.h"
#include "main.h"

#include <QAbstractItemDelegate>
#include <QPainter>

#include <boost/filesystem.hpp>

#define DECORATION_SIZE 64
#define NUM_ITEMS 6

class TxViewDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    TxViewDelegate(): QAbstractItemDelegate(), unit(BitcoinUnits::BTC)
    {

    }

    inline void paint(QPainter *painter, const QStyleOptionViewItem &option,
                      const QModelIndex &index ) const
    {
        painter->save();

        QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
        QRect mainRect = option.rect;
        QRect decorationRect(mainRect.topLeft(), QSize(DECORATION_SIZE, DECORATION_SIZE));
        int xspace = DECORATION_SIZE + 8;
        int ypad = 6;
        int halfheight = (mainRect.height() - 2*ypad)/2;
        QRect amountRect(mainRect.left() + xspace, mainRect.top()+ypad, mainRect.width() - xspace, halfheight);
        QRect addressRect(mainRect.left() + xspace, mainRect.top()+ypad+halfheight, mainRect.width() - xspace, halfheight);
        icon.paint(painter, decorationRect);

        QDateTime date = index.data(TransactionTableModel::DateRole).toDateTime();
        QString address = index.data(Qt::DisplayRole).toString();
        qint64 amount = index.data(TransactionTableModel::AmountRole).toLongLong();
        bool confirmed = index.data(TransactionTableModel::ConfirmedRole).toBool();
        QVariant value = index.data(Qt::ForegroundRole);
        QColor foreground = option.palette.color(QPalette::Text);
        /*if(qVariantCanConvert<QColor>(value))
        {
            foreground = qvariant_cast<QColor>(value);
        }*/

        painter->setPen(foreground);
        painter->drawText(addressRect, Qt::AlignLeft|Qt::AlignVCenter, address);

        if(amount < 0)
        {
            foreground = COLOR_NEGATIVE;
        }
        else if(!confirmed)
        {
            foreground = COLOR_UNCONFIRMED;
        }
        else
        {
            foreground = option.palette.color(QPalette::Text);
        }
        painter->setPen(foreground);
        QString amountText = BitcoinUnits::formatWithUnit(unit, amount, true);
        if(!confirmed)
        {
            amountText = QString("[") + amountText + QString("]");
        }
        painter->drawText(amountRect, Qt::AlignRight|Qt::AlignVCenter, amountText);

        painter->setPen(option.palette.color(QPalette::Text));
        painter->drawText(amountRect, Qt::AlignLeft|Qt::AlignVCenter, GUIUtil::dateTimeStr(date));

        painter->restore();
    }

    inline QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        return QSize(DECORATION_SIZE, DECORATION_SIZE);
    }

    int unit;

};
#include "overviewpage.moc"

extern bool fBurstMode; // from main.cpp
extern bool fBurstLaunchedNow;  // from main.cpp

int iBurstTimerProgress = 0;


void setBurstStateConfig(bool state)
{
    try
    {
        boost::filesystem::path configFilePath = GetDataDir() / "burst_config";

        if (!state)
        {
            if (boost::filesystem::exists(configFilePath))
            {
                boost::filesystem::remove(configFilePath);
            }
        }
        else
        {
            if (!boost::filesystem::exists(configFilePath))
            {
                boost::filesystem::ofstream f(configFilePath);
                f << "1";
            }
        }
    }
    catch (std::exception&)
    {
    }
}

bool getBurstStateConfig()
{
    try
    {
        boost::filesystem::path configFilePath = GetDataDir() / "burst_config";

        if (boost::filesystem::exists(configFilePath))
        {
            return true;
        }
    }
    catch (std::exception&)
    {
    }
    return false;
}

OverviewPage::OverviewPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OverviewPage),
    currentBalance(-1),
    currentStake(0),
    currentUnconfirmedBalance(-1),
    currentImmatureBalance(-1),
    txdelegate(new TxViewDelegate()),
    filter(0)
{
    ui->setupUi(this);

    // Recent transactions
    ui->listTransactions->setItemDelegate(txdelegate);
    ui->listTransactions->setIconSize(QSize(DECORATION_SIZE, DECORATION_SIZE));
    ui->listTransactions->setMinimumHeight(NUM_ITEMS * (DECORATION_SIZE + 2));
    ui->listTransactions->setAttribute(Qt::WA_MacShowFocusRect, false);

    connect(ui->listTransactions, SIGNAL(clicked(QModelIndex)), this, SLOT(handleTransactionClicked(QModelIndex)));

    // init "out of sync" warning labels
    ui->labelWalletStatus->setText("(" + tr("out of sync") + ")");
    ui->labelTransactionsStatus->setText("(" + tr("out of sync") + ")");

    // start with displaying the "out of sync" warning
    showOutOfSyncWarning(true);

    fBurstMode = getBurstStateConfig();
    ui->radioBurst->setChecked(fBurstMode);
    ui->radioPoS->setChecked(!fBurstMode);

    timer = new QTimer(this);

    // setup signal and slot
    connect(timer, SIGNAL(timeout()), this, SLOT(updateOnTimer()));

    // msec
    timer->start(1000);
}

void OverviewPage::handleTransactionClicked(const QModelIndex &index)
{
    if(filter)
        emit transactionClicked(filter->mapToSource(index));
}

OverviewPage::~OverviewPage()
{
    delete ui;
}

void OverviewPage::setBalance(qint64 balance, qint64 stake, qint64 unconfirmedBalance, qint64 immatureBalance)
{
    int unit = model->getOptionsModel()->getDisplayUnit();
    currentBalance = balance;
    currentStake = stake;
    currentUnconfirmedBalance = unconfirmedBalance;
    currentImmatureBalance = immatureBalance;
    ui->labelBalance->setText(BitcoinUnits::formatWithUnit(unit, balance));
    ui->labelStake->setText(BitcoinUnits::formatWithUnit(unit, stake));
    ui->labelUnconfirmed->setText(BitcoinUnits::formatWithUnit(unit, unconfirmedBalance));
    ui->labelImmature->setText(BitcoinUnits::formatWithUnit(unit, immatureBalance));

    // only show immature (newly mined) balance if it's non-zero, so as not to complicate things
    // for the non-mining users
    bool showImmature = immatureBalance != 0;
    ui->labelImmature->setVisible(showImmature);
    ui->labelImmatureText->setVisible(showImmature);
}

void OverviewPage::setNumTransactions(int count)
{
    ui->labelNumTransactions->setText(QLocale::system().toString(count));
}

void OverviewPage::setModel(WalletModel *model)
{
    this->model = model;
    if(model && model->getOptionsModel())
    {
        // Set up transaction list
        filter = new TransactionFilterProxy();
        filter->setSourceModel(model->getTransactionTableModel());
        filter->setLimit(NUM_ITEMS);
        filter->setDynamicSortFilter(true);
        filter->setSortRole(Qt::EditRole);
        filter->sort(TransactionTableModel::Date, Qt::DescendingOrder);

        ui->listTransactions->setModel(filter);
        ui->listTransactions->setModelColumn(TransactionTableModel::ToAddress);

        // Keep up to date with wallet
        setBalance(model->getBalance(), model->getStake(), model->getUnconfirmedBalance(), model->getImmatureBalance());
        connect(model, SIGNAL(balanceChanged(qint64, qint64, qint64, qint64)), this, SLOT(setBalance(qint64, qint64, qint64, qint64)));

        setNumTransactions(model->getNumTransactions());
        connect(model, SIGNAL(numTransactionsChanged(int)), this, SLOT(setNumTransactions(int)));

        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));
    }

    // update the display unit, to not use the default ("MINT")
    updateDisplayUnit();
}

void OverviewPage::updateDisplayUnit()
{
    if(model && model->getOptionsModel())
    {
        if(currentBalance != -1)
            setBalance(currentBalance, model->getStake(), currentUnconfirmedBalance, currentImmatureBalance);

        // Update txdelegate->unit with the current unit
        txdelegate->unit = model->getOptionsModel()->getDisplayUnit();

        ui->listTransactions->update();
    }
}

void OverviewPage::showOutOfSyncWarning(bool fShow)
{
    ui->labelWalletStatus->setVisible(fShow);
    ui->labelTransactionsStatus->setVisible(fShow);
}

void OverviewPage::updateBurstState()
{
    if (ui->radioBurst->isChecked())
    {
        ui->progressBarBurst->setEnabled(false);
        fBurstMode = true;
        fBurstLaunchedNow = false;
    }
    else
    {
        updateBurstButtonIsAllowed(false);
        ui->progressBarBurst->setEnabled(false);
        fBurstMode = false;
        fBurstLaunchedNow = false;
    }
    setBurstStateConfig(fBurstMode);
}

void OverviewPage::on_radioBurst_clicked()
{
    updateBurstState();
}

void OverviewPage::on_radioPoS_clicked()
{
    updateBurstState();
}

void OverviewPage::on_buttonBurstNow_clicked()
{
    if (fBurstMode)
    {
        fBurstLaunchedNow = true;
        iBurstTimerProgress = 0;
    }
}

extern bool fWalletIsUnlockedForBurst; // from bitcoingui.cpp
extern uint64 nWeightForBURST; // from bitcoingui.cpp

bool fBurstButtonEnabled = false;

void OverviewPage::updateBurstButtonIsAllowed(bool allowed)
{
    ui->buttonBurstNow->setEnabled(allowed);
    fBurstButtonEnabled = allowed;
}

void OverviewPage::updateOnTimer()
{
    if (fBurstMode)
    {
        if (IsInitialBlockDownload())
        {
            ui->labelBurstState->setText("You can not use BURST until wallet is fully synced");
            updateBurstButtonIsAllowed(false);
        }
        else if (!fWalletIsUnlockedForBurst)
        {
            ui->labelBurstState->setText("You have to unlock wallet for staking first to use BURST");
            updateBurstButtonIsAllowed(false);
        }
        else if (nWeightForBURST == 0)
        {
            ui->labelBurstState->setText("You don't have mature coins to use BURST");
            iBurstTimerProgress = 100;
            updateBurstButtonIsAllowed(false);
        }
        else
        {
            // TODO: also need to check minted coins amount

            // All good
            ui->labelBurstState->setText("");
            updateBurstButtonIsAllowed(true);
        }
    }
    else
    {
        // Burst is disable, so we do not care here
        ui->labelBurstState->setText("");
    }

    if (fBurstLaunchedNow)
    {
        iBurstTimerProgress += 3;
        if (iBurstTimerProgress < 100)
        {
            // BURST IS RUNNING
            ui->progressBarBurst->setValue(iBurstTimerProgress);
            ui->progressBarBurst->setEnabled(true);
            updateBurstButtonIsAllowed(false);
            ui->radioPoS->setEnabled(false);
        }
        else
        {
            // BURST IS DONE
            ui->progressBarBurst->setValue(0);
            ui->progressBarBurst->setEnabled(false);
            updateBurstButtonIsAllowed(true);
            ui->radioPoS->setEnabled(true);
            fBurstLaunchedNow = false;
        }
    }
}
