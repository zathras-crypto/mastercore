// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SENDMPDIALOG_H
#define SENDMPDIALOG_H

#include "walletmodel.h"

#include <QDialog>
#include <QString>

class OptionsModel;
class SendMPEntry;
class SendCoinsRecipient;

QT_BEGIN_NAMESPACE
class QUrl;
QT_END_NAMESPACE

namespace Ui {
    class SendMPDialog;
}

/** Dialog for sending bitcoins */
class SendMPDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SendMPDialog(QWidget *parent = 0);
    ~SendMPDialog();

    void setModel(WalletModel *model);

    /** Set up the tab chain manually, as Qt messes up the tab chain by default in some cases (issue https://bugreports.qt-project.org/browse/QTBUG-10907).
     */
    QWidget *setupTabChain(QWidget *prev);

    void setAddress(const QString &address);
    void pasteEntry(const SendCoinsRecipient &rv);
    bool handlePaymentRequest(const SendCoinsRecipient &recipient);

public slots:
    void clear();
    void reject();
    void accept();
    SendMPEntry *addEntry();
    void updateTabsAndLabels();
    void setBalance(qint64 balance, qint64 unconfirmedBalance, qint64 immatureBalance);

private:
    Ui::SendMPDialog *ui;
    WalletModel *model;
    bool fNewRecipientAllowed;

    // Process WalletModel::SendCoinsReturn and generate a pair consisting
    // of a message and message flags for use in emit message().
    // Additional parameter msgArg can be used via .arg(msgArg).
    void processSendMPReturn(const WalletModel::SendCoinsReturn &SendCoinsReturn, const QString &msgArg = QString());

private slots:
    void on_sendButton_clicked();
    void removeEntry(SendMPEntry* entry);
    void updateDisplayUnit();
    void coinControlFeatureChanged(bool);
    void coinControlButtonClicked();
    void coinControlChangeChecked(int);
    void coinControlChangeEdited(const QString &);
    void coinControlUpdateLabels();
    void coinControlClipboardQuantity();
    void coinControlClipboardAmount();
    void coinControlClipboardFee();
    void coinControlClipboardAfterFee();
    void coinControlClipboardBytes();
    void coinControlClipboardPriority();
    void coinControlClipboardLowOutput();
    void coinControlClipboardChange();

signals:
    // Fired when a message should be reported to the user
    void message(const QString &title, const QString &message, unsigned int style);
};

#endif // SENDMPDIALOG_H
