// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TXHISTORYDIALOG_H
#define TXHISTORYDIALOG_H

#include "guiutil.h"

#include <QDialog>
#include <QString>
#include <QTableWidget>
#include <QTextEdit>
#include <QDialogButtonBox>
#include <QPushButton>

class ClientModel;
class OptionsModel;
class WalletModel;

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
    void setClientModel(ClientModel *model);
    void setWalletModel(WalletModel *model);

    void CreateRow(int rowcount, bool valid, bool bInbound, int confirmations, std::string txTimeStr, std::string displayType, std::string displayAddress, std::string displayAmount, std::string txidStr, bool fundsMoved);
    void accept();
    /** Set up the tab chain manually, as Qt messes up the tab chain by default in some cases (issue https://bugreports.qt-project.org/browse/QTBUG-10907).
     */
    QDialog *txDlg;
    QWidget *setupTabChain(QWidget *prev);
    QTableWidgetItem *iconCell;
    QTableWidgetItem *dateCell;
    QTableWidgetItem *typeCell;
    QTableWidgetItem *amountCell;
    QTableWidgetItem *addressCell;
    QTableWidgetItem *txidCell;
    QLayout *dlgLayout;
    QTextEdit *dlgTextEdit;
    QDialogButtonBox *buttonBox;
    QPushButton *closeButton;

    GUIUtil::TableViewLastColumnResizingFixer *borrowedColumnResizingFixer;

    virtual void resizeEvent(QResizeEvent* event);

public slots:
    //void switchButtonClicked();

private:
    Ui::txHistoryDialog *ui;
    ClientModel *clientModel;
    WalletModel *walletModel;
    QMenu *contextMenu;

private slots:
    void contextualMenu(const QPoint &);
    void showDetails();
    void copyAddress();
    void copyAmount();
    void copyTxID();
    void UpdateHistory();

signals:
    void doubleClicked(const QModelIndex&);
    // Fired when a message should be reported to the user
    void message(const QString &title, const QString &message, unsigned int style);
};

#endif // TXHISTORYDIALOG_H
