// Copyright (c) 2011-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "txhistorydialog.h"
#include "ui_txhistorydialog.h"

#include "guiutil.h"
#include "optionsmodel.h"
#include "walletmodel.h"
#include "wallet.h"
#include "base58.h"
#include "ui_interface.h"

#include <boost/filesystem.hpp>

#include "leveldb/db.h"
#include "leveldb/write_batch.h"

// potentially overzealous includes here
#include "base58.h"
#include "rpcserver.h"
#include "init.h"
#include "util.h"
#include <fstream>
#include <algorithm>
#include <vector>
#include <utility>
#include <string>
#include <boost/assign/list_of.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/find.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/filesystem.hpp>
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"
#include "leveldb/db.h"
#include "leveldb/write_batch.h"
// end potentially overzealous includes

using namespace json_spirit;
#include "mastercore.h"
using namespace mastercore;

// potentially overzealous using here
using namespace std;
using namespace boost;
using namespace boost::assign;
using namespace leveldb;
// end potentially overzealous using

#include "mastercore_dex.h"
#include "mastercore_tx.h"
#include "mastercore_sp.h"
#include "mastercore_rpc.h"
#include "mastercore_parse_string.h"

#include <QDateTime>
#include <QMessageBox>
#include <QScrollBar>
#include <QTextDocument>
#include <QListWidget>
#include <QMenu>
#include <QTextEdit>

TXHistoryDialog::TXHistoryDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::txHistoryDialog),
    model(0)
{
    ui->setupUi(this);
    this->model = model;

    // setup
    ui->txHistoryTable->setColumnCount(6);
    ui->txHistoryTable->setHorizontalHeaderItem(0, new QTableWidgetItem(" "));
    ui->txHistoryTable->setHorizontalHeaderItem(1, new QTableWidgetItem("Date"));
    ui->txHistoryTable->setHorizontalHeaderItem(2, new QTableWidgetItem("Type"));
    ui->txHistoryTable->setHorizontalHeaderItem(3, new QTableWidgetItem("Address"));
    ui->txHistoryTable->setHorizontalHeaderItem(4, new QTableWidgetItem("Amount"));
    ui->txHistoryTable->verticalHeader()->setVisible(false);
//    ui->txHistoryTable->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
//    ui->txHistoryTable->setShowGrid(false);
    ui->txHistoryTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->txHistoryTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->txHistoryTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->txHistoryTable->horizontalHeader()->setResizeMode(3, QHeaderView::Stretch);
    ui->txHistoryTable->setColumnWidth(0, 23);
    ui->txHistoryTable->setColumnWidth(1, 150);
    ui->txHistoryTable->setColumnWidth(2, 130);
    ui->txHistoryTable->setColumnWidth(4, 200);
    ui->txHistoryTable->setColumnWidth(5, 0);

    // Always show scroll bar
    //ui->txHistoryTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    ui->txHistoryTable->setTabKeyNavigation(false);
    //view->setContextMenuPolicy(Qt::CustomContextMenu);

    ui->txHistoryTable->setContextMenuPolicy(Qt::CustomContextMenu);

    // Actions
    QAction *copyAddressAction = new QAction(tr("Copy address"), this);
    QAction *copyAmountAction = new QAction(tr("Copy amount"), this);
    QAction *copyTxIDAction = new QAction(tr("Copy transaction ID"), this);
    QAction *showDetailsAction = new QAction(tr("Show transaction details"), this);

    contextMenu = new QMenu();
    contextMenu->addAction(copyAddressAction);
    contextMenu->addAction(copyAmountAction);
    contextMenu->addAction(copyTxIDAction);
    contextMenu->addAction(showDetailsAction);

    // Connect actions
    connect(ui->txHistoryTable, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextualMenu(QPoint)));
    connect(ui->txHistoryTable, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(showDetails()));
    connect(copyAddressAction, SIGNAL(triggered()), this, SLOT(copyAddress()));
    connect(copyAmountAction, SIGNAL(triggered()), this, SLOT(copyAmount()));
    connect(copyTxIDAction, SIGNAL(triggered()), this, SLOT(copyTxID()));
    connect(showDetailsAction, SIGNAL(triggered()), this, SLOT(showDetails()));


    UpdateHistory();
}

void TXHistoryDialog::UpdateHistory()
{
    int rowcount = 0;

    // handle pending transactions first
    for(PendingMap::iterator it = my_pending.begin(); it != my_pending.end(); ++it)
    {
        CMPPending *p_pending = &(it->second);
        uint256 txid = it->first;
        string txidStr = txid.GetHex();
        //p_pending->print(txid);

        string senderAddress = p_pending->src;
        uint64_t propertyId = p_pending->prop;
        bool divisible = isPropertyDivisible(propertyId);
        string displayAmount;
        int64_t amount = p_pending->amount;
        string displayToken;
        string displayValid;
        string displayAddress = senderAddress;
        int64_t type = p_pending->type;

        if (divisible) { displayAmount = FormatDivisibleMP(amount); } else { displayAmount = FormatIndivisibleMP(amount); }
        // clean up trailing zeros - good for RPC not so much for UI
        displayAmount.erase ( displayAmount.find_last_not_of('0') + 1, std::string::npos );
        if (displayAmount.length() > 0) { std::string::iterator it = displayAmount.end() - 1; if (*it == '.') { displayAmount.erase(it); } } //get rid of trailing dot if non decimal

        if (propertyId < 3)
        {
            if(propertyId == 1) { displayToken = " MSC"; }
            if(propertyId == 2) { displayToken = " TMSC"; }
        }
        else
        {
            string s = to_string(propertyId);
            displayToken = " SPT#" + s;
        }
        QString txTimeStr = "Unconfirmed";
        string displayType;
        if (type == 0) displayType = "Send";
        if (type == 21) displayType = "MetaDEx Trade";
        displayAmount = "-" + displayAmount; //all pending are outbound
        //icon
        QIcon ic = QIcon(":/icons/transaction_0");
        // add to history
        ui->txHistoryTable->setRowCount(rowcount+1);
        QTableWidgetItem *dateCell = new QTableWidgetItem(txTimeStr);
        QTableWidgetItem *typeCell = new QTableWidgetItem(QString::fromStdString(displayType));
        QTableWidgetItem *addressCell = new QTableWidgetItem(QString::fromStdString(displayAddress));
        QTableWidgetItem *amountCell = new QTableWidgetItem(QString::fromStdString(displayAmount + displayToken));
        QTableWidgetItem *iconCell = new QTableWidgetItem;
        QTableWidgetItem *txidCell = new QTableWidgetItem(QString::fromStdString(txidStr)); //hash.GetHex()));
        iconCell->setIcon(ic);
        addressCell->setTextAlignment(Qt::AlignLeft + Qt::AlignVCenter);
        addressCell->setForeground(QColor("#707070"));
        amountCell->setTextAlignment(Qt::AlignRight + Qt::AlignVCenter);
        amountCell->setForeground(QColor("#EE0000"));
        if (rowcount % 2)
        {
            amountCell->setBackground(QColor("#F0F0F0"));
            addressCell->setBackground(QColor("#F0F0F0"));
            dateCell->setBackground(QColor("#F0F0F0"));
            typeCell->setBackground(QColor("#F0F0F0"));
            txidCell->setBackground(QColor("#F0F0F0"));
        }
        ui->txHistoryTable->setItem(rowcount, 0, iconCell);
        ui->txHistoryTable->setItem(rowcount, 1, dateCell);
        ui->txHistoryTable->setItem(rowcount, 2, typeCell);
        ui->txHistoryTable->setItem(rowcount, 3, addressCell);
        ui->txHistoryTable->setItem(rowcount, 4, amountCell);
        ui->txHistoryTable->setItem(rowcount, 5, txidCell);
        rowcount += 1;
    }

    // wallet transactions
    CWallet *wallet = pwalletMain;
    string sAddress = "";
    string addressParam = "";

    int64_t nCount = 50; //don't display more than 50 historical transactions at the moment until we can move to a cached model
    int64_t nStartBlock = 0;
    int64_t nEndBlock = 999999;

    Array response; //prep an array to hold our output

    // rewrite to use original listtransactions methodology from core
    LOCK(wallet->cs_wallet);
    std::list<CAccountingEntry> acentries;
    CWallet::TxItems txOrdered = pwalletMain->OrderedTxItems(acentries, "*");

    // iterate backwards 
    for (CWallet::TxItems::reverse_iterator it = txOrdered.rbegin(); it != txOrdered.rend(); ++it)
    {
        CWalletTx *const pwtx = (*it).second.first;
        if (pwtx != 0)
        {
            uint256 hash = pwtx->GetHash();
            CTransaction wtx;
            uint256 blockHash = 0;
            if (!GetTransaction(hash, wtx, blockHash, true)) continue;
            // get the time of the tx
            int64_t nTime = pwtx->GetTxTime();
            // get the height of the transaction and check it's within the chosen parameters
            blockHash = pwtx->hashBlock;
            if ((0 == blockHash) || (NULL == mapBlockIndex[blockHash])) continue;
            CBlockIndex* pBlockIndex = mapBlockIndex[blockHash];
            if (NULL == pBlockIndex) continue;
            int blockHeight = pBlockIndex->nHeight;
            if ((blockHeight < nStartBlock) || (blockHeight > nEndBlock)) continue; // ignore it if not within our range
            // check if the transaction exists in txlist, and if so is it correct type (21)
            if (p_txlistdb->exists(hash))
            {
                string statusText;
                unsigned int propertyId = 0;
                uint64_t amount = 0;
                string senderAddress;
                string refAddress;
                bool divisible = false;
                bool valid = false;
                string MPTxType;

                CMPTransaction mp_obj;
                int parseRC = parseTransaction(true, wtx, blockHeight, 0, &mp_obj);
                if (0 <= parseRC) //negative RC means no MP content/badly encoded TX, we shouldn't see this if TX in levelDB but check for sanity
                {
                    if (0<=mp_obj.step1())
                    {
                        MPTxType = mp_obj.getTypeString();
                        senderAddress = mp_obj.getSender();
                        refAddress = mp_obj.getReceiver();
                        int tmpblock=0;
                        uint32_t tmptype=0;
                        uint64_t amountNew=0;
                        valid=getValidMPTX(hash, &tmpblock, &tmptype, &amountNew);

                        if (0 == mp_obj.step2_Value())
                        {
                            propertyId = mp_obj.getProperty();
                            amount = mp_obj.getAmount();
                            divisible = isPropertyDivisible(propertyId);
                        }
                    }
                }
                QListWidgetItem *qItem = new QListWidgetItem();
                qItem->setData(Qt::DisplayRole, QString::fromStdString(hash.GetHex()));
                // shrink tx type
                string displayType = "Unknown";
                switch (mp_obj.getType())
                {
                    case MSC_TYPE_SIMPLE_SEND: displayType = "Send"; break;
                    case MSC_TYPE_RESTRICTED_SEND: displayType = "Rest. Send"; break;
                    case MSC_TYPE_SEND_TO_OWNERS: displayType = "Send To Owners"; break;
                    case MSC_TYPE_SAVINGS_MARK: displayType = "Mark Savings"; break;
                    case MSC_TYPE_SAVINGS_COMPROMISED: ; displayType = "Lock Savings"; break;
                    case MSC_TYPE_RATELIMITED_MARK: displayType = "Rate Limit"; break;
                    case MSC_TYPE_AUTOMATIC_DISPENSARY: displayType = "Auto Dispense"; break; 
                    case MSC_TYPE_TRADE_OFFER: displayType = "DEx Trade"; break;
                    case MSC_TYPE_METADEX: displayType = "MetaDEx Trade"; break;
                    case MSC_TYPE_ACCEPT_OFFER_BTC: displayType = "DEx Accept"; break;
                    case MSC_TYPE_CREATE_PROPERTY_FIXED: displayType = "Create Property"; break;
                    case MSC_TYPE_CREATE_PROPERTY_VARIABLE: displayType = "Create Property"; break;
                    case MSC_TYPE_PROMOTE_PROPERTY: displayType = "Promo Property";
                    case MSC_TYPE_CLOSE_CROWDSALE: displayType = "Close Crowdsale";
                    case MSC_TYPE_CREATE_PROPERTY_MANUAL: displayType = "Create Property"; break;
                    case MSC_TYPE_GRANT_PROPERTY_TOKENS: displayType = "Grant Tokens"; break;
                    case MSC_TYPE_REVOKE_PROPERTY_TOKENS: displayType = "Revoke Tokens"; break;
                    case MSC_TYPE_CHANGE_ISSUER_ADDRESS: displayType = "Change Issuer"; break;
                }

                string displayAmount;
                string displayToken;
                string displayValid;
                string displayAddress;
                if (IsMyAddress(senderAddress)) { displayAddress = senderAddress; } else { displayAddress = refAddress; }
                if (divisible) { displayAmount = FormatDivisibleMP(amount); } else { displayAmount = FormatIndivisibleMP(amount); }
                // clean up trailing zeros - good for RPC not so much for UI
                displayAmount.erase ( displayAmount.find_last_not_of('0') + 1, std::string::npos );
                if (displayAmount.length() > 0) { std::string::iterator it = displayAmount.end() - 1; if (*it == '.') { displayAmount.erase(it); } } //get rid of trailing dot if non decimal
                if (valid) { displayValid = "valid"; } else { displayValid = "invalid"; }
                if (propertyId < 3)
                {
                    if(propertyId == 1) { displayToken = " MSC"; }
                    if(propertyId == 2) { displayToken = " TMSC"; }
                }
                else
                {
                    string s = to_string(propertyId);
                    displayToken = " SPT#" + s;
                }
                string displayDirection = "out";
                if ((displayType == "Send") && (!IsMyAddress(senderAddress))) { displayType = "Receive"; }

                QDateTime txTime;
                txTime.setTime_t(nTime);
                QString txTimeStr = txTime.toString(Qt::SystemLocaleShortDate);
                if (IsMyAddress(senderAddress)) displayAmount = "-" + displayAmount;
                //icon
                QIcon ic = QIcon(":/icons/transaction_0");
                int confirmations =  1 + GetHeight() - pBlockIndex->nHeight;
                switch(confirmations)
                {
                     case 1: ic = QIcon(":/icons/transaction_1"); break;
                     case 2: ic = QIcon(":/icons/transaction_2"); break;
                     case 3: ic = QIcon(":/icons/transaction_3"); break;
                     case 4: ic = QIcon(":/icons/transaction_4"); break;
                     case 5: ic = QIcon(":/icons/transaction_5"); break;
                }
                if (confirmations > 5) ic = QIcon(":/icons/transaction_confirmed");
                if (!valid) ic = QIcon(":/icons/transaction_invalid");

                // add to history
                ui->txHistoryTable->setRowCount(rowcount+1);
                QTableWidgetItem *dateCell = new QTableWidgetItem(txTimeStr);
                QTableWidgetItem *typeCell = new QTableWidgetItem(QString::fromStdString(displayType));
                QTableWidgetItem *addressCell = new QTableWidgetItem(QString::fromStdString(displayAddress));
                QTableWidgetItem *amountCell = new QTableWidgetItem(QString::fromStdString(displayAmount + displayToken));
                QTableWidgetItem *iconCell = new QTableWidgetItem;
                QTableWidgetItem *txidCell = new QTableWidgetItem(QString::fromStdString(hash.GetHex()));
                iconCell->setIcon(ic);
                addressCell->setTextAlignment(Qt::AlignLeft + Qt::AlignVCenter);
                addressCell->setForeground(QColor("#707070"));
                amountCell->setTextAlignment(Qt::AlignRight + Qt::AlignVCenter);
                amountCell->setForeground(QColor("#00AA00"));
                if (IsMyAddress(senderAddress)) amountCell->setForeground(QColor("#EE0000"));
                if (rowcount % 2)
                {
                    amountCell->setBackground(QColor("#F0F0F0"));
                    addressCell->setBackground(QColor("#F0F0F0"));
                    dateCell->setBackground(QColor("#F0F0F0"));
                    typeCell->setBackground(QColor("#F0F0F0"));
                    txidCell->setBackground(QColor("#F0F0F0"));
                }
                ui->txHistoryTable->setItem(rowcount, 0, iconCell);
                ui->txHistoryTable->setItem(rowcount, 1, dateCell);
                ui->txHistoryTable->setItem(rowcount, 2, typeCell);
                ui->txHistoryTable->setItem(rowcount, 3, addressCell);
                ui->txHistoryTable->setItem(rowcount, 4, amountCell);
                ui->txHistoryTable->setItem(rowcount, 5, txidCell);
                rowcount += 1;
            }
        }
    // don't burn time doing more work than we need to
    if (rowcount > nCount) break;
    }
}

void TXHistoryDialog::setModel(WalletModel *model)
{
    this->model = model;
    connect(model, SIGNAL(balanceChanged(qint64, qint64, qint64)), this, SLOT(UpdateHistory()));
}

void TXHistoryDialog::contextualMenu(const QPoint &point)
{
    QModelIndex index = ui->txHistoryTable->indexAt(point);
    if(index.isValid())
    {
        contextMenu->exec(QCursor::pos());
    }
}

void TXHistoryDialog::copyAddress()
{
    GUIUtil::setClipboard(ui->txHistoryTable->item(ui->txHistoryTable->currentRow(),3)->text());
}

void TXHistoryDialog::copyAmount()
{
    GUIUtil::setClipboard(ui->txHistoryTable->item(ui->txHistoryTable->currentRow(),4)->text());
}

void TXHistoryDialog::copyTxID()
{
    GUIUtil::setClipboard(ui->txHistoryTable->item(ui->txHistoryTable->currentRow(),5)->text());
}

void TXHistoryDialog::showDetails()
{
    Object txobj;
    uint256 txid;
    txid.SetHex(ui->txHistoryTable->item(ui->txHistoryTable->currentRow(),5)->text().toStdString());
    std::string strTXText;

    // first of all check if the TX is a pending tx, if so grab details from pending map
    PendingMap::iterator it = my_pending.find(txid);
    if (it != my_pending.end())
    {
        CMPPending *p_pending = &(it->second);
        strTXText = "*** THIS TRANSACTION IS UNCONFIRMED ***\n" + p_pending->desc;
    }
    else
    {
        // grab details usual way
        int pop = populateRPCTransactionObject(txid, &txobj, "");
        if (0<=pop)
        {
            strTXText = write_string(Value(txobj), false) + "\n";
        }
    }

    if (!strTXText.empty())
    {
        // clean up
        string from = ",";
        string to = ",\n    ";
        size_t start_pos = 0;
        while((start_pos = strTXText.find(from, start_pos)) != std::string::npos)
        {
            strTXText.replace(start_pos, from.length(), to);
            start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
        }
        from = ":";
        to = "   :   ";
        start_pos = 0;
        while((start_pos = strTXText.find(from, start_pos)) != std::string::npos)
        {
            strTXText.replace(start_pos, from.length(), to);
            start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
        }
        from = "{";
        to = "{\n    ";
        start_pos = 0;
        while((start_pos = strTXText.find(from, start_pos)) != std::string::npos)
        {
            strTXText.replace(start_pos, from.length(), to);
            start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
        }
        from = "}";
        to = "\n}";
        start_pos = 0;
        while((start_pos = strTXText.find(from, start_pos)) != std::string::npos)
        {
            strTXText.replace(start_pos, from.length(), to);
            start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
        }

        QString txText = QString::fromStdString(strTXText);
        QDialog *txDlg = new QDialog;
        QLayout *dlgLayout = new QVBoxLayout;
        dlgLayout->setSpacing(12);
        dlgLayout->setMargin(12);
        QTextEdit *dlgTextEdit = new QTextEdit;
        dlgTextEdit->setText(txText);
        dlgTextEdit->setStatusTip("Transaction Information");
        dlgLayout->addWidget(dlgTextEdit);
        txDlg->setWindowTitle("Transaction Information");
        QPushButton *closeButton = new QPushButton(tr("&Close"));
        closeButton->setDefault(true);
        QDialogButtonBox *buttonBox = new QDialogButtonBox;
        buttonBox->addButton(closeButton, QDialogButtonBox::AcceptRole);
        dlgLayout->addWidget(buttonBox);
        txDlg->setLayout(dlgLayout);
        txDlg->resize(700, 360);
        connect(buttonBox, SIGNAL(accepted()), txDlg, SLOT(accept()));
        txDlg->setAttribute(Qt::WA_DeleteOnClose); //delete once it's closed
        if (txDlg->exec() == QDialog::Accepted) { } else { } //do nothing but close
    }
}

void TXHistoryDialog::accept()
{

}
