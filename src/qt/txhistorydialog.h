// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TXHISTORYDIALOG_H
#define TXHISTORYDIALOG_H

#include "walletmodel.h"

#include <QDialog>
#include <QString>
#include <QTableWidget>
class OptionsModel;

QT_BEGIN_NAMESPACE
class QUrl;
QT_END_NAMESPACE

namespace Ui {
    class txHistoryDialog;
}

/** Dialog for looking up Master Protocol tokens */
class TXHistoryDialog : public QDialog
{
    Q_OBJECT

public:
    //void FullRefresh();
    explicit TXHistoryDialog(QWidget *parent = 0);
    void setModel(WalletModel *model);
    void UpdateHistory();
    /** Set up the tab chain manually, as Qt messes up the tab chain by default in some cases (issue https://bugreports.qt-project.org/browse/QTBUG-10907).
     */
    QWidget *setupTabChain(QWidget *prev);
    QTableWidgetItem *iconCell;
    QTableWidgetItem *dateCell;
    QTableWidgetItem *typeCell;
    QTableWidgetItem *amountCell;
    QTableWidgetItem *addressCell;
    QTableWidgetItem *txidCell;

public slots:
    //void switchButtonClicked();

private:
    Ui::txHistoryDialog *ui;
    WalletModel *model;
    QMenu *contextMenu;

private slots:
    void contextualMenu(const QPoint &);
    void showDetails();
    void copyAddress();
    void copyAmount();
    void copyTxID();

signals:
    void doubleClicked(const QModelIndex&);
    // Fired when a message should be reported to the user
    void message(const QString &title, const QString &message, unsigned int style);
};

#endif // ORDERHISTORYDIALOG_H
