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

#include "ui_interface.h"

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

    // property ID selector
    propSelectorWidget = new QComboBox(this);
#ifdef Q_OS_MAC
    propSelectorWidget->setFixedWidth(121);
#else
    propSelectorWidget->setFixedWidth(120);
#endif

    propSelectorWidget->addItem(tr("Test1")); //, TransactionFilterProxy::ALL_TYPES);
    propSelectorWidget->addItem(tr("Test2")); //, TransactionFilterProxy::TYPE(TransactionRecord::RecvWithAddress) 
    propSelectorWidget->addItem(tr("Test3")); //, TransactionFilterProxy::TYPE(TransactionRecord::SendToAddress) 

    hlayout->addWidget(propSelectorWidget);

    QVBoxLayout *vlayout = new QVBoxLayout(this);
    vlayout->setContentsMargins(0,0,0,0);
    vlayout->setSpacing(0);

    QTableView *view = new QTableView(this);
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
