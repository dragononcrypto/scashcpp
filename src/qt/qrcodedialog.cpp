// Copyright (c) 2017-2018 Scash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "qrcodedialog.h"
#include "ui_qrcodedialog.h"

#include "bitcoinunits.h"
#include "guiconstants.h"
#include "guiutil.h"
#include "optionsmodel.h"

#include <QPixmap>
#include <QUrl>

#include <qrcodegen.h>

QRCodeDialog::QRCodeDialog(const QString &addr, const QString &label, bool enableReq, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QRCodeDialog),
    model(0),
    address(addr)
{
    ui->setupUi(this);

    setWindowTitle(QString("%1").arg(address));

    ui->chkReqPayment->setVisible(enableReq);
    ui->lblAmount->setVisible(enableReq);
    ui->lnReqAmount->setVisible(enableReq);

    ui->lnLabel->setText(label);

    ui->btnSaveAs->setEnabled(false);

    genCode();
}

QRCodeDialog::~QRCodeDialog()
{
    delete ui;
}

void QRCodeDialog::setModel(OptionsModel *model)
{
    this->model = model;

    if (model)
        connect(model, SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));

    // update the display unit, to not use the default ("BTC")
    updateDisplayUnit();
}

void QRCodeDialog::genCode()
{
    QString uri = getURI();

    if (uri != "")
    {
        uint8_t qrcode[qrcodegen_BUFFER_LEN_MAX];
        uint8_t tempBuffer[qrcodegen_BUFFER_LEN_MAX];
        bool ok = qrcodegen_encodeText(getURI().toStdString().c_str(), tempBuffer, qrcode, qrcodegen_Ecc_HIGH,
              qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX, qrcodegen_Mask_AUTO, true);

        if (!ok)
        {
            ui->lblQRCode->setText(tr("Error encoding URI into QR Code."));
            return;
        }

        // 4 is border
        int size = qrcodegen_getSize(qrcode);
        myImage = QImage(size + 8, size + 8, QImage::Format_RGB32);
        myImage.fill(0xffffff);
        for (int y = 0; y < size; y++)
        {
            for (int x = 0; x < size; x++)
            {
                myImage.setPixel(x + 4, y + 4, (qrcodegen_getModule(qrcode, x, y) ? 0x0 : 0xffffff));
            }
        }

        ui->lblQRCode->setPixmap(QPixmap::fromImage(myImage).scaled(300, 300));
        ui->outUri->setPlainText(uri);
    }
}

QString QRCodeDialog::getURI()
{
    QString ret = QString("scash:%1").arg(address);
    int paramCount = 0;

    ui->outUri->clear();

    if (ui->chkReqPayment->isChecked())
    {
        if (ui->lnReqAmount->validate())
        {
            // even if we allow a non BTC unit input in lnReqAmount, we generate the URI with BTC as unit (as defined in BIP21)
            ret += QString("?amount=%1").arg(BitcoinUnits::format(BitcoinUnits::BTC, ui->lnReqAmount->value()));
            paramCount++;
        }
        else
        {
            ui->btnSaveAs->setEnabled(false);
            ui->lblQRCode->setText(tr("The entered amount is invalid, please check."));
            return QString("");
        }
    }

    if (!ui->lnLabel->text().isEmpty())
    {
        QString lbl(QUrl::toPercentEncoding(ui->lnLabel->text()));
        ret += QString("%1label=%2").arg(paramCount == 0 ? "?" : "&").arg(lbl);
        paramCount++;
    }

    if (!ui->lnMessage->text().isEmpty())
    {
        QString msg(QUrl::toPercentEncoding(ui->lnMessage->text()));
        ret += QString("%1message=%2").arg(paramCount == 0 ? "?" : "&").arg(msg);
        paramCount++;
    }

    // limit URI length to prevent a DoS against the QR-Code dialog
    if (ret.length() > MAX_URI_LENGTH)
    {
        ui->btnSaveAs->setEnabled(false);
        ui->lblQRCode->setText(tr("Resulting URI too long, try to reduce the text for label / message."));
        return QString("");
    }

    ui->btnSaveAs->setEnabled(true);

    return ret;
}

void QRCodeDialog::on_lnReqAmount_textChanged()
{
    genCode();
}

void QRCodeDialog::on_lnLabel_textChanged()
{
    genCode();
}

void QRCodeDialog::on_lnMessage_textChanged()
{
    genCode();
}

void QRCodeDialog::on_btnSaveAs_clicked()
{
    QString fn = GUIUtil::getSaveFileName(this, tr("Save QR Code"), QString(), tr("PNG Images (*.png)"));
    if (!fn.isEmpty())
        myImage.scaled(EXPORT_IMAGE_SIZE, EXPORT_IMAGE_SIZE).save(fn);
}

void QRCodeDialog::on_chkReqPayment_toggled(bool fChecked)
{
    if (!fChecked)
    {
        // if chkReqPayment is not active, don't display lnReqAmount as invalid
        ui->lnReqAmount->setValid(true);
    }

    genCode();
}

void QRCodeDialog::updateDisplayUnit()
{
    if (model)
    {
        // Update lnReqAmount with the current unit
        ui->lnReqAmount->setDisplayUnit(model->getDisplayUnit());
    }
}
