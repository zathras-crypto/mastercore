// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "balancesview.h"

#include "addresstablemodel.h"
#include "bitcoinunits.h"
#include "csvmodelwriter.h"
#include "editaddressdialog.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "transactiondescdialog.h"
#include "transactionfilterproxy.h"
#include "transactionrecord.h"
#include "transactiontablemodel.h"
#include "walletmodel.h"
#include "wallet.h"

#include "ui_interface.h"

#include <boost/filesystem.hpp>

#include "leveldb/db.h"
#include "leveldb/write_batch.h"

#include "mastercore.h"

#include <QComboBox>
#include <QDateTimeEdit>
#include <QDesktopServices>
#include <QDoubleValidator>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPoint>
#include <QScrollBar>
#include <QSignalMapper>
#include <QTableView>
#include <QUrl>
#include <QVBoxLayout>

BalancesView::BalancesView(QWidget *parent) :
    QWidget(parent), model(0), balancesView(0)
{
    // Build filter row
    setContentsMargins(0,0,0,0);

    QHBoxLayout *hlayout = new QHBoxLayout();
    hlayout->setContentsMargins(0,0,0,0);
#ifdef Q_OS_MAC
    hlayout->setSpacing(5);
    hlayout->addSpacing(26);
#else
    hlayout->setSpacing(0);
    hlayout->addSpacing(23);
#endif
    hlayout->addStretch();
    // property ID selector
    propSelLabel = new QLabel("Show Balances For: ");
    hlayout->addWidget(propSelLabel);

    propSelectorWidget = new QComboBox(this);
#ifdef Q_OS_MAC
    propSelectorWidget->setFixedWidth(301);
#else
    propSelectorWidget->setFixedWidth(300);
#endif
    propSelectorWidget->addItem(tr("Wallet Totals (Summary)"));
    // trigger update of global balances
    set_wallet_totals();
    // populate property selector
    for (unsigned int propertyId = 1; propertyId<100000; propertyId++)
    {
        if ((global_balance_money_maineco[propertyId] > 0) || (global_balance_reserved_maineco[propertyId] > 0))
        {
            string spName;
            spName = getPropertyName(propertyId).c_str();
            if(spName.size()>20) spName=spName.substr(0,20)+"...";
            spName += " (#" + static_cast<ostringstream*>( &(ostringstream() << propertyId) )->str() + ")";
            propSelectorWidget->addItem(tr(spName.c_str()));
        }
    }
    for (unsigned int propertyId = 1; propertyId<100000; propertyId++)
    {
        if ((global_balance_money_testeco[propertyId] > 0) || (global_balance_reserved_testeco[propertyId] > 0))
        {
            string spName;
            spName = getPropertyName(propertyId+2147483647).c_str();
            if(spName.size()>20) spName=spName.substr(0,20)+"...";
            spName += " (#" + static_cast<ostringstream*>( &(ostringstream() << propertyId+2147483647) )->str() + ")";
            propSelectorWidget->addItem(tr(spName.c_str()));
        }
    }
    //add the selector to the layout
    hlayout->addWidget(propSelectorWidget);

    QVBoxLayout *vlayout = new QVBoxLayout(this);
    vlayout->setContentsMargins(0,0,0,0);
    vlayout->setSpacing(0);

    //populate
    //prep matrix
    const int numRows = 3000;
    const int numColumns = 3;
    uint matrix[numRows][numColumns];
//        QVBoxLayout *mscvbox = new QVBoxLayout();
    MatrixModel *mmp = NULL;
    QTableView *view = NULL;
    //create matrix
    for (int i = 0; i < numRows; ++i)
         for (int j = 0; j < numColumns; ++j)
              matrix[i][j] = (i+1) * (j+1);
    //create a model which adapts the data (the matrix) to the view.
    mmp = new MatrixModel(numRows, numColumns, (uint*)matrix, 1);
    view = new QTableView(this);
    view->setModel(mmp);
    //adjust sizing
    view->horizontalHeader()->resizeSection(0, 160);
    #if QT_VERSION < 0x050000
       view->horizontalHeader()->setResizeMode(1, QHeaderView::Stretch);
    #else
       view->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    #endif
    view->horizontalHeader()->resizeSection(2, 140);
    view->horizontalHeader()->resizeSection(3, 140);
    vlayout->addLayout(hlayout);
    vlayout->addWidget(view);
    vlayout->setSpacing(0);
    int width = view->verticalScrollBar()->sizeHint().width();
    // Cover scroll bar width with spacing
#ifdef Q_OS_MAC
    hlayout->addSpacing(width+2);
#else
    hlayout->addSpacing(width);
#endif
    // Always show scroll bar
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    view->setTabKeyNavigation(false);
    view->setContextMenuPolicy(Qt::CustomContextMenu);

    balancesView = view;

    // Actions
    QAction *balancesCopyAddressAction = new QAction(tr("Copy address"), this);
    QAction *balancesCopyLabelAction = new QAction(tr("Copy label"), this);
    QAction *balancesCopyAmountAction = new QAction(tr("Copy amount"), this);

    contextMenu = new QMenu();
    contextMenu->addAction(balancesCopyAddressAction);
    contextMenu->addAction(balancesCopyLabelAction);
    contextMenu->addAction(balancesCopyAmountAction);

    mapperThirdPartyTxUrls = new QSignalMapper(this);

    // Connect actions
    connect(propSelectorWidget, SIGNAL(activated(int)), this, SLOT(propSelectorChanged(int)));

    connect(balancesCopyAddressAction, SIGNAL(triggered()), this, SLOT(balancesCopyAddress()));
    connect(balancesCopyLabelAction, SIGNAL(triggered()), this, SLOT(balancesCopyLabel()));
    connect(balancesCopyAmountAction, SIGNAL(triggered()), this, SLOT(balancesCopyAmount()));
}

void BalancesView::propSelectorChanged(int idx)
{
//redisplay balances with new selection
}

void BalancesView::contextualMenu(const QPoint &point)
{
    QModelIndex index = balancesView->indexAt(point);
    if(index.isValid())
    {
//        contextMenu->exec(QCursor::pos());
    }
}

void BalancesView::balancesCopyAddress()
{
//    GUIUtil::copyEntryData(transactionView, 0, TransactionTableModel::AddressRole);
}

void BalancesView::balancesCopyLabel()
{
//    GUIUtil::copyEntryData(transactionView, 0, TransactionTableModel::LabelRole);
}

void BalancesView::balancesCopyAmount()
{
//    GUIUtil::copyEntryData(transactionView, 0, TransactionTableModel::FormattedAmountRole);
}

// We override the virtual resizeEvent of the QWidget to adjust tables column
// sizes as the tables width is proportional to the dialogs width.
void BalancesView::resizeEvent(QResizeEvent* event)
{
//    QWidget::resizeEvent(event);
//    columnResizingFixer->stretchColumnWidth(TransactionTableModel::ToAddress);
}
