#ifndef MESSAGEENTRY_H
#define MESSAGEENTRY_H

#include <QFrame>

namespace Ui {
    class MessageEntry;
}

class WalletModel;
class SendCoinsRecipient;

/** A single entry in the dialog for sending bitcoins. */
class MessageEntry : public QFrame
{
    Q_OBJECT

public:
    explicit MessageEntry(QWidget *parent = 0);
    ~MessageEntry();

    void setModel(WalletModel *model);
    bool validate();

    void setFocus();

    int getTextCharactersCount();
    double getFeeEstimation();
    QString getText();

    bool isMessageValid();

public slots:
    void clear();

signals:

private slots:
    void on_MessageEditor_textChanged();

private:
    Ui::MessageEntry *ui;
    WalletModel *model;
};
#endif // MESSAGEENTRY_H
