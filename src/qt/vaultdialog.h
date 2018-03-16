#ifndef VAULTDIALOG_H
#define VAULTDIALOG_H

#include <QtWidgets/QDialog>
#include <QString>
#include <QTimer>

namespace Ui {
    class VaultDialog;
}
class WalletModel;
class SendCoinsEntry;
class SendCoinsRecipient;

QT_BEGIN_NAMESPACE
class QUrl;
QT_END_NAMESPACE

/** Dialog for sending bitcoins */
class VaultDialog : public QDialog
{
    Q_OBJECT

public:
    explicit VaultDialog(QWidget *parent = 0);
    ~VaultDialog();

    void setModel(WalletModel *model);


public slots:
    void clear();
    void reject();
    void accept();

    void updateOnTimer();

private slots:
    void on_selectButton_clicked();

    void on_infoButton_clicked();

    void on_clearButton_clicked();

    void on_publishButton_clicked();

private:
    Ui::VaultDialog *ui;
    WalletModel *model;
    QTimer* timer;

    bool sendVaultData(QString data);

};

#endif // VAULTDIALOG_H
