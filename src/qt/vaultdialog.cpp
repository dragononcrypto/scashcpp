// Copyright (c) 2017-2018 Scash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "vaultdialog.h"
#include "ui_vaultdialog.h"
#include "init.h"
#include "walletmodel.h"
#include "addresstablemodel.h"
#include "addressbookpage.h"
#include "bitcoinunits.h"
#include "addressbookpage.h"
#include "optionsmodel.h"
#include "sendcoinsentry.h"
#include "messageentry.h"
#include "guiutil.h"
#include "askpassphrasedialog.h"
#include "coincontrol.h"
#include "coincontroldialog.h"
#include "bitcoingui.h"

#include <QtWidgets/QMessageBox>
#include <QLocale>
#include <QTextDocument>
#include <QtWidgets/QScrollBar>
#include <QClipboard>
#include <QFileDialog>
#include <QStandardPaths>

#ifdef WIN32
#include "openssl/include/openssl/aes.h"
#include "openssl/include/openssl/evp.h"
#include "openssl/include/openssl/sha.h"
#else
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#endif

VaultDialog::VaultDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::VaultDialog),
    model(0)
{
    ui->setupUi(this);

    timer = new QTimer(this);

    // setup signal and slot
    connect(timer, SIGNAL(timeout()), this, SLOT(updateOnTimer()));

    // msec
    timer->start(1000);
}

void VaultDialog::setModel(WalletModel *model)
{
    this->model = model;
}

void VaultDialog::updateOnTimer()
{
    //
}

VaultDialog::~VaultDialog()
{
    delete ui;
}

void VaultDialog::clear()
{
    ui->labelDocFile->setText("");
    ui->labelDocHash->setText("");
    ui->textComment->setText("");
    ui->textDocAuthor->setText("");
    ui->textDocName->setText("");
    ui->textDocVersion->setText("");

    ui->publishButton->setDefault(true);
}

void VaultDialog::reject()
{
    //
}

void VaultDialog::accept()
{
    clear();
}

std::string hexStr(unsigned char* data, int len)
{
    std::stringstream ss;
    ss << std::hex;
    for(int i=0;i<len;++i)
        ss << std::setw(2) << std::setfill('0') << (int)data[i];
    return ss.str();
}

QString getHash(QString filename)
{
    try
    {
        unsigned char c[SHA512_DIGEST_LENGTH];
        FILE *inFile = fopen(filename.toStdString().c_str(), "rb");
        SHA512_CTX mdContext;
        int bytes;
        unsigned char data[1024];

        if (inFile == NULL) {
            return "I/O Error";
        }

        SHA512_Init(&mdContext);
        while ((bytes = fread (data, 1, 1024, inFile)) != 0)
        {
            SHA512_Update(&mdContext, data, bytes);
        }
        SHA512_Final(c, &mdContext);

        return (hexStr(c, SHA512_DIGEST_LENGTH).c_str());
    }
    catch (std::exception &ex)
    {
        return QString("Exception: ") + ex.what();
    }
    return "";
}


void VaultDialog::on_selectButton_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
           tr("Open Any File"), "",
           tr("All Files (*.*)"));

    if (!fileName.isEmpty())
    {
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly)) {
            QMessageBox::information(this, tr("Unable to open file"),
                file.errorString());
            return;
        }

        ui->labelDocFile->setText(fileName);
        ui->labelDocHash->setText(getHash(fileName));

        try
        {
            QFileInfo fileInfo(fileName);
            ui->textDocName->setText(fileInfo.fileName());
        }
        catch(std::exception&)
        {
        }
    }
}

void VaultDialog::on_infoButton_clicked()
{
    QMessageBox::information(this, tr("Scash Vault"),
        tr("TODO"),
        QMessageBox::Ok, QMessageBox::Ok);
}

void VaultDialog::on_clearButton_clicked()
{
    QMessageBox::StandardButton retval = QMessageBox::question(this,
                          tr("Scash Vault"),
                          tr("Are you sure you want to clear all data entered?"),
          QMessageBox::Yes|QMessageBox::Cancel,
          QMessageBox::Cancel);

    if(retval == QMessageBox::Yes)
    {
        clear();
    }
}

bool VaultDialog::sendVaultData(QString data)
{
    WalletModel::UnlockContext ctx(model->requestUnlock());
    if(!ctx.isValid())
    {
        // Unlock wallet was cancelled
        return false;
    }

    std::string newAddress = model->getNewAddress();
    if (newAddress.empty())
    {
        QMessageBox::critical(this, tr("Scash vault"),
            tr("Can not create new address"),
            QMessageBox::Ok, QMessageBox::Ok);
        return false;
    }

    WalletModel::SendCoinsReturn sendstatus;

    QList<SendCoinsRecipient> recipients;
    SendCoinsRecipient rec;
    rec.address = newAddress.c_str();
    rec.amount = 10 * COIN;
    rec.label = "";
    recipients.push_back(rec);

    if (!data.isEmpty())
    {
        std::string messageText = "";

        if (data.length() > SendMessageMaxChars)
        {
            QMessageBox::critical(this, tr("Scash vault"),
                tr("Message size is out of allowed constrains"),
                QMessageBox::Ok, QMessageBox::Ok);
            return false;
        }

        // Text will be filtered in sending kernel, at this stage allow everything
        messageText = data.toStdString();

        sendstatus = model->sendCoins(recipients, NULL, messageText);
    }

    switch(sendstatus.status)
    {
        case WalletModel::InvalidAddress:
            QMessageBox::warning(this, tr("Scash Vault"),
                tr("The recipient address is not valid, please recheck."),
                QMessageBox::Ok, QMessageBox::Ok);
            break;
        case WalletModel::WalletLocked:
            QMessageBox::warning(this, tr("Scash Vault"),
                tr("Wallet is locked. Unlock wallet before send."),
                QMessageBox::Ok, QMessageBox::Ok);
            break;
        case WalletModel::InvalidAmount:
            QMessageBox::warning(this, tr("Scash Vault"),
                tr("The amount to pay must be larger than 0."),
                QMessageBox::Ok, QMessageBox::Ok);
            break;
        case WalletModel::AmountExceedsBalance:
            QMessageBox::warning(this, tr("Scash Vault"),
                tr("The amount exceeds your balance."),
                QMessageBox::Ok, QMessageBox::Ok);
            break;
        case WalletModel::MessageTooLong:
            QMessageBox::warning(this, tr("Scash Vault"),
                tr("The attached message is too long."),
                QMessageBox::Ok, QMessageBox::Ok);
            break;
        case WalletModel::AmountWithFeeExceedsBalance:
            QMessageBox::warning(this, tr("Scash Vault"),
                tr("The total exceeds your balance when the %1 transaction fee is included.").
                arg(BitcoinUnits::formatWithUnit(BitcoinUnits::BTC, sendstatus.fee)),
                QMessageBox::Ok, QMessageBox::Ok);
            break;
        case WalletModel::DuplicateAddress:
            QMessageBox::warning(this, tr("Scash Vault"),
                tr("Duplicate address found, can only send to each address once per send operation."),
                QMessageBox::Ok, QMessageBox::Ok);
            break;
        case WalletModel::TransactionCreationFailed:
            QMessageBox::warning(this, tr("Scash Vault"),
                tr("Error: Transaction creation failed."),
                QMessageBox::Ok, QMessageBox::Ok);
            break;
        case WalletModel::TransactionCommitFailed:
            QMessageBox::warning(this, tr("Scash Vault"),
                tr("Error: The transaction was rejected. This might happen if some of the coins in your wallet were already spent, such as if you used a copy of wallet.dat and coins were spent in the copy but not marked as spent here."),
                QMessageBox::Ok, QMessageBox::Ok);
            break;
        case WalletModel::Aborted: // User aborted, nothing to do
            break;
        case WalletModel::OK:
            accept();

            BitcoinGUI::switchToTransactionPage();

            break;
    }
}

void VaultDialog::on_publishButton_clicked()
{
    if (ui->labelDocHash->text().isEmpty())
    {
        QMessageBox::critical(this, tr("Scash Vault"),
            tr("You have to select document first."),
            QMessageBox::Ok, QMessageBox::Ok);
        return;
    }

    if (!this->model)
    {
        QMessageBox::critical(this, tr("Scash Vault"),
            tr("Wallet is not attached."),
            QMessageBox::Ok, QMessageBox::Ok);
        return;
    }

    QString msg = "%VAULT\nHASH: ";
    msg += ui->labelDocHash->text();
    if (!ui->textDocName->text().isEmpty())
    {
        msg += "\nNAME: " + ui->textDocName->text();
    }
    if (!ui->textDocVersion->text().isEmpty())
    {
        msg += "\nVERSION: " + ui->textDocVersion->text();
    }
    if (!ui->textDocAuthor->text().isEmpty())
    {
        msg += "\nAUTHOR: " + ui->textDocAuthor->text();
    }
    if (!ui->textComment->toPlainText().isEmpty())
    {
        msg += "\nCOMMENT: " + ui->textComment->toPlainText();
    }

    QMessageBox::StandardButton retval = QMessageBox::question(this,
                          tr("Scash Vault"),
                          tr("Are you sure you want to publish this information onto the SpeedCash blockchain?\n\n%1")
                            .arg(msg),
          QMessageBox::Yes|QMessageBox::Cancel,
          QMessageBox::Cancel);

    if(retval == QMessageBox::Yes)
    {
        if (sendVaultData(msg))
        {
            //
        }
    }
}
