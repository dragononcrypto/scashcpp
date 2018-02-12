/********************************************************************************
** Form generated from reading UI file 'messageentry.ui'
**
** Created by: Qt User Interface Compiler version 4.8.7
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MESSAGEENTRY_H
#define UI_MESSAGEENTRY_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPlainTextEdit>

QT_BEGIN_NAMESPACE

class Ui_MessageEntry
{
public:
    QGridLayout *gridLayoutMsg;
    QGridLayout *gridLayout;
    QPlainTextEdit *MessageEditor;
    QLabel *MessageDetailsLabel;

    void setupUi(QFrame *MessageEntry)
    {
        if (MessageEntry->objectName().isEmpty())
            MessageEntry->setObjectName(QString::fromUtf8("MessageEntry"));
        MessageEntry->resize(729, 136);
        MessageEntry->setFrameShape(QFrame::StyledPanel);
        MessageEntry->setFrameShadow(QFrame::Sunken);
        gridLayoutMsg = new QGridLayout(MessageEntry);
        gridLayoutMsg->setSpacing(12);
        gridLayoutMsg->setObjectName(QString::fromUtf8("gridLayoutMsg"));
        gridLayout = new QGridLayout();
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        MessageEditor = new QPlainTextEdit(MessageEntry);
        MessageEditor->setObjectName(QString::fromUtf8("MessageEditor"));

        gridLayout->addWidget(MessageEditor, 0, 0, 1, 1);


        gridLayoutMsg->addLayout(gridLayout, 0, 0, 1, 1);

        MessageDetailsLabel = new QLabel(MessageEntry);
        MessageDetailsLabel->setObjectName(QString::fromUtf8("MessageDetailsLabel"));

        gridLayoutMsg->addWidget(MessageDetailsLabel, 1, 0, 1, 1);


        retranslateUi(MessageEntry);

        QMetaObject::connectSlotsByName(MessageEntry);
    } // setupUi

    void retranslateUi(QFrame *MessageEntry)
    {
        MessageEntry->setWindowTitle(QApplication::translate("MessageEntry", "Form", 0));
        MessageDetailsLabel->setText(QApplication::translate("MessageEntry", "...", 0));
    } // retranslateUi

};

namespace Ui {
    class MessageEntry: public Ui_MessageEntry {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MESSAGEENTRY_H
