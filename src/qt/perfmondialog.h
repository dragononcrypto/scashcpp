// Copyright (c) 2017-2018 Scash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERFMONDIALOG_H
#define PERFMONDIALOG_H

#include <QDialog>
#include <QImage>
#include <QTimer>

namespace Ui
{
    class PerfMonDialog;
}

class OptionsModel;

class PerfMonDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PerfMonDialog(QWidget *parent = 0);
    ~PerfMonDialog();

    void setModel(OptionsModel *model);

    void updateNow();

public slots:
    void updateOnTimer();

private:
    Ui::PerfMonDialog *ui;
    OptionsModel *model;

    QTimer* timer;

    QImage imgIncomingTraffic;
    QImage imgOutgoingTraffic;
    QImage imgInConnections;
    QImage imgOutConnections;
    QImage imgBanCount;
};

#endif // PERFMONDIALOG_H

