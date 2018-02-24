// Copyright (c) 2017-2018 Scash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "messageentry.h"
#include "ui_messageentry.h"
#include "guiutil.h"
#include "bitcoinunits.h"
#include "addressbookpage.h"
#include "walletmodel.h"
#include "optionsmodel.h"
#include "addresstablemodel.h"
#include "main.h"

#include <QApplication>
#include <QClipboard>

MessageEntry::MessageEntry(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::MessageEntry),
    model(0)
{
    ui->setupUi(this);

#ifdef Q_OS_MAC
    //ui->payToLayout->setSpacing(4);
#endif

    setFocusPolicy(Qt::TabFocus);
    setFocusProxy(ui->MessageEditor);

    ui->MessageEditor->clear();
    ui->MessageDetailsLabel->setText(tr("Enter message you want to send among with transaction in the field above."));
}

QString MessageEntry::getText()
{
    return ui->MessageEditor->toPlainText();
}

int MessageEntry::getTextCharactersCount()
{
    return getText().length();
}

double MessageEntry::getFeeEstimation()
{
    return (double)(getTextCharactersCount() / 100) *
            (double)SendMessageCostPerChar /
            (double)COIN * 100;
}

MessageEntry::~MessageEntry()
{
    delete ui;
}

bool MessageEntry::isMessageValid()
{
    int chars = getTextCharactersCount();
    return chars > 0 && chars < SendMessageMaxChars;
}

void MessageEntry::on_MessageEditor_textChanged()
{
    int chars = getTextCharactersCount();
    if (chars > 0)
    {
        if (chars <= SendMessageMaxChars)
        {
            ui->MessageDetailsLabel->setText(
                        tr("Message length: %1 chars; additional fee estimate is %2 SCS")
                        .arg(chars).arg(getFeeEstimation()));
        }
        else
        {
            ui->MessageDetailsLabel->setText(tr("Message is too large to send in single transaction, please split"));
        }
    }
    else
    {
        ui->MessageDetailsLabel->setText(tr("Message cleared and detached from transaction"));
    }
}

void MessageEntry::setModel(WalletModel *model)
{
    // just ignore
}

bool MessageEntry::validate()
{
    return true;
}

void MessageEntry::clear()
{
    ui->MessageEditor->clear();
}

void MessageEntry::setFocus()
{
    ui->MessageEditor->setFocus();
}
