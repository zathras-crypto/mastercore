// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef ORDERHISTORYDIALOG_H
#define ORDERHISTORYDIALOG_H

#include "walletmodel.h"

#include <QDialog>
#include <QString>

class OptionsModel;

QT_BEGIN_NAMESPACE
class QUrl;
QT_END_NAMESPACE

namespace Ui {
    class orderHistoryDialog;
}

/** Dialog for looking up Master Protocol tokens */
class OrderHistoryDialog : public QDialog
{
    Q_OBJECT

public:
    //void FullRefresh();
    explicit OrderHistoryDialog(QWidget *parent = 0);
    void setModel(WalletModel *model);

    /** Set up the tab chain manually, as Qt messes up the tab chain by default in some cases (issue https://bugreports.qt-project.org/browse/QTBUG-10907).
     */
    QWidget *setupTabChain(QWidget *prev);


public slots:
    //void switchButtonClicked();

private:
    Ui::orderHistoryDialog *ui;
    WalletModel *model;

private slots:
    //void buyRecalc();

signals:
    // Fired when a message should be reported to the user
    void message(const QString &title, const QString &message, unsigned int style);
};

#endif // ORDERHISTORYDIALOG_H
