// Copyright (c) 2017-2018 Scash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "perfmondialog.h"
#include "ui_perfmondialog.h"

#include "bitcoinunits.h"
#include "guiconstants.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "chartdata.h"
#include "net.h"

#include <QPixmap>
#include <QUrl>
#include <QPainter>

#include <qrcodegen.h>

PerfMonDialog::PerfMonDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PerfMonDialog),
    model(0)
{
    ui->setupUi(this);

    timer = new QTimer(this);

    // setup signal and slot
    connect(timer, SIGNAL(timeout()), this, SLOT(updateOnTimer()));

    // msec
    timer->start(1000);
}

void PerfMonDialog::updateOnTimer()
{
    if (isVisible())
    {
        updateNow();
    }
}

PerfMonDialog::~PerfMonDialog()
{
    delete ui;
}

void PerfMonDialog::setModel(OptionsModel *model)
{
    this->model = model;
}

static void drawCharOnImage(QImage &img, ChartData& data, int divisor, QString unit)
{
    int imgWidth = img.width();
    int imgHeight = img.height();

    img.fill(0xffffff);

    static const int gridStep = 30;

    if (imgWidth > gridStep && imgHeight > gridStep)
    {
        for (int i = imgHeight - gridStep; i > 0; i -= gridStep)
        {
            for (int x = 0; x < imgWidth; x += 2)
            {
                img.setPixel(x, i, 0x77aa77);
            }
        }

        for (int i = imgWidth - gridStep; i > 0; i -= gridStep)
        {
            for (int y = 0; y < imgHeight; y += 2)
            {
                img.setPixel(i, y, 0x77aa77);
            }
        }
    }

    // min is always 0, now calc max
    int maximum = 0;
    int current = data.GetData(0);

    for (int i = 0; i < data.GetCount(); i++)
    {
        int r = data.GetData(i);
        if (r > maximum) maximum = r;
    }

    if (maximum > 0)
    {
        double scaleXinv = (double)imgWidth / (double)data.GetCount();
        if (scaleXinv < 1.0) scaleXinv = 1.0;
        double scaleY = (double)(imgHeight) / (double)(maximum - 0);

        int prevY = -1;
        for (int x = 0; x < imgWidth; x++)
        {
            int index = (int)((double)x / scaleXinv);
            if (index > 0 && index < data.GetCount())
            {
                int value = data.GetData(index);
                int y = (int)((double)value * scaleY);
                if (y < 0) y = 0;
                if (y >= imgHeight-1) y = imgHeight - 2;
                y = (imgHeight-1) - y;
                img.setPixel(x, y, 0xff00000);
                if (prevY != -1)
                {
                    for (int t = std::min(y, prevY); t < std::max(y, prevY); t++) img.setPixel(x, t, 0xcc4444);
                }

                prevY = y;
            }
        }
    }

    QPainter p;
    if (p.begin(&img))
    {
        p.setPen(QPen(Qt::darkBlue));
        p.setFont(QFont("Times", 12, QFont::Bold));
        p.drawText(img.rect(), Qt::AlignTop | Qt::AlignRight,
                   QString::fromLatin1("Max: %1 %2  ").arg(maximum / divisor).arg(unit));

        p.setPen(QPen(Qt::darkRed));
        p.drawText(img.rect(), Qt::AlignBottom | Qt::AlignRight,
                   QString::fromLatin1("Now: %1 %2  ").arg(current/ divisor).arg(unit));
    }

    for (int y = 0; y < imgHeight; y++)
    {
        img.setPixel(0, y, 0x0);
        img.setPixel(imgWidth-1, y, 0x0);
    }

    for (int x = 0; x < imgWidth; x++)
    {
        img.setPixel(x, 0, 0x0);
        img.setPixel(x, imgHeight-1, 0x0);
    }
}

static void prepareAndDrawChartData(QImage &img, QLabel &label, ChartData &data, int divisor, QString unit)
{
    int imgWidth = label.width();
    int imgHeight = label.height();

    if (img.width() != imgWidth || img.height() != imgHeight)
    {
        img = QImage(imgWidth, imgHeight, QImage::Format_RGB32);
    }

    drawCharOnImage(img, data, divisor, unit);
    if (label.text().length() > 0) label.setText("");
    label.setPixmap(QPixmap::fromImage(img));
}

void PerfMonDialog::updateNow()
{
    if (!fChartsEnabled) return;

    int inbound = 0, outbound = 0;
    CNetStatus::GetNodesStats(inbound, outbound);
    Charts::NetworkInConnections().AddData(inbound);
    Charts::NetworkOutConnections().AddData(outbound);

    prepareAndDrawChartData(imgIncomingTraffic, *(ui->labelIncomingTraffic),
                            Charts::NetworkInBytes(), 1024, "Kb/s");
    prepareAndDrawChartData(imgOutgoingTraffic, *(ui->labelOutgoingTraffic),
                            Charts::NetworkOutBytes(), 1024, "Kb/s");

    prepareAndDrawChartData(imgInConnections, *(ui->labelIncomingConnections),
                            Charts::NetworkInConnections(), 1, "nodes");
    prepareAndDrawChartData(imgOutConnections, *(ui->labelOutgoingConnections),
                            Charts::NetworkOutConnections(), 1, "nodes");

    prepareAndDrawChartData(imgBanCount, *(ui->labelBannedCount),
                            Charts::NetworkBannedConnections(), 1, "nodes/s");

    // Trick to scroll graphic
    Charts::NetworkInBytes().AddData(0);
    Charts::NetworkOutBytes().AddData(0);
    Charts::NetworkBannedConnections().AddData(0);
}

/*void PerfMonDialog::on_btnSaveAs_clicked()
{
    QString fn = GUIUtil::getSaveFileName(this, tr("Save Chart"), QString(), tr("PNG Images (*.png)"));
    if (!fn.isEmpty())
        myImage.scaled(EXPORT_IMAGE_SIZE, EXPORT_IMAGE_SIZE).save(fn);
}*/


