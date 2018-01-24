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

#include <unistd.h>
#include <ios>
#include <iostream>
#include <fstream>

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

int64 getMemUsage()
{
    // 'file' stat seems to give the most reliable results
    std::ifstream stat_stream("/proc/self/stat", std::ifstream::ios_base::in);

    // dummy vars for leading entries in stat that we don't care about
    std::string pid, comm, state, ppid, pgrp, session, tty_nr;
    std::string tpgid, flags, minflt, cminflt, majflt, cmajflt;
    std::string utime, stime, cutime, cstime, priority, nice;
    std::string O_, itrealvalue, starttime;

    // the two fields we want
    //
    unsigned long vsize;
    long rss;

    stat_stream >> pid >> comm >> state >> ppid >> pgrp >> session >> tty_nr
              >> tpgid >> flags >> minflt >> cminflt >> majflt >> cmajflt
              >> utime >> stime >> cutime >> cstime >> priority >> nice
              >> O_ >> itrealvalue >> starttime >> vsize >> rss; // don't care about the rest

    stat_stream.close();

    // unsigned long vm_usage     = vsize / 1024.0;

    long page_size_kb = sysconf(_SC_PAGE_SIZE) / 1024; // in case x86-64 is configured to use 2MB pages
     unsigned long resident_set = rss * page_size_kb;

    return resident_set;
}

void PerfMonDialog::updateNow()
{
    if (!fChartsEnabled) return;

    int inbound = 0, outbound = 0;
    CNetStatus::GetNodesStats(inbound, outbound);
    Charts::NetworkInConnections().AddData(inbound);
    Charts::NetworkOutConnections().AddData(outbound);
    Charts::MemoryUtilization().AddData(getMemUsage());

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

    prepareAndDrawChartData(imgMemUtil, *(ui->labelMemoryUsage),
                            Charts::MemoryUtilization(), 1024, "MB");

    prepareAndDrawChartData(imgDatabaseQueries, *(ui->labelDatabaseQueries),
                            Charts::DatabaseQueries(), 1, "q/s");

    prepareAndDrawChartData(imgDatabaseTimes, *(ui->labelDatabaseAvgTime),
                            Charts::DatabaseAvgTime(), 1, "ms");

    prepareAndDrawChartData(imgBlocksAdded, *(ui->labelBlocksAdded),
                            Charts::BlocksAdded(), 1, "blocks/s");

    prepareAndDrawChartData(imgBlocksRejected, *(ui->labelBlocksRejected),
                            Charts::BlocksRejected(), 1, "blocks/s");

    // Trick to scroll graphic
    Charts::NetworkInBytes().AddData(0);
    Charts::NetworkOutBytes().AddData(0);
    Charts::NetworkBannedConnections().AddData(0);
    Charts::DatabaseQueries().AddData(0);
    Charts::BlocksAdded().AddData(0);
    Charts::BlocksRejected().AddData(0);
}



