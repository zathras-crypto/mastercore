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
    connect(ui->txHistoryTable, SIGNAL(doubleClicked(QModelIndex)), this, SIGNAL(doubleClicked(QModelIndex)));
    connect(copyAddressAction, SIGNAL(triggered()), this, SLOT(copyAddress()));
    connect(copyAmountAction, SIGNAL(triggered()), this, SLOT(copyAmount()));
    connect(copyTxIDAction, SIGNAL(triggered()), this, SLOT(copyTxID()));
    connect(showDetailsAction, SIGNAL(triggered()), this, SLOT(showDetails()));


    UpdateHistory();
}

void TXHistoryDialog::UpdateHistory()
{
    CWallet *wallet = pwalletMain;
    string sAddress = "";
    string addressParam = "";

    int64_t nCount = 10;
    int64_t nFrom = 0;
    int64_t nStartBlock = 0;
    int64_t nEndBlock = 999999;

    Array response; //prep an array to hold our output

    // rewrite to use original listtransactions methodology from core
    LOCK(wallet->cs_wallet);
    std::list<CAccountingEntry> acentries;
    CWallet::TxItems txOrdered = pwalletMain->OrderedTxItems(acentries, "*");

    // iterate backwards 
    int rowcount = 0;
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
                     case 1: ic = QIcon(":/icons/transaction_1");
                     case 2: ic = QIcon(":/icons/transaction_2");
                     case 3: ic = QIcon(":/icons/transaction_3");
                     case 4: ic = QIcon(":/icons/transaction_4");
                     case 5: ic = QIcon(":/icons/transaction_5");
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
/*
                if(pending)
                {
                    QFont font;
                    font.setBold(true);
                    ui->sellList->item(rowcount, 0)->setFont(font);
                    ui->sellList->item(rowcount, 1)->setFont(font);
                    ui->sellList->item(rowcount, 2)->setFont(font);
                }
*/
                rowcount += 1;

/*


                qItem->setData(Qt::UserRole + 1, QString::fromStdString(displayType));
                qItem->setData(Qt::UserRole + 2, QString::fromStdString(displayAmount + displayToken));
                qItem->setData(Qt::UserRole + 3, QString::fromStdString(displayDirection));
                qItem->setData(Qt::UserRole + 4, QString::fromStdString(displayAddress));
                qItem->setData(Qt::UserRole + 5, txTimeStr);
                qItem->setData(Qt::UserRole + 6, QString::fromStdString(displayValid));
*/
//                ui->txHistoryLW->addItem(qItem);
            }
        }
    }
            // don't burn time doing more work than we need to
//            if ((int)response.size() >= (nCount+nFrom)) break;
    // sort array here and cut on nFrom and nCount
//    if (nFrom > (int)response.size())
//        nFrom = response.size();
//    if ((nFrom + nCount) > (int)response.size())
//        nCount = response.size() - nFrom;
//    Array::iterator first = response.begin();
//    std::advance(first, nFrom);
//    Array::iterator last = response.begin();
//    std::advance(last, nFrom+nCount);

//    if (last != response.end()) response.erase(last, response.end());
//    if (first != response.begin()) response.erase(response.begin(), first);

//    std::reverse(response.begin(), response.end()); // return oldest to newest?
 //   return response;   // return response array for JSON serialization

}

void TXHistoryDialog::setModel(WalletModel *model)
{
    this->model = model;
    //connect(model, SIGNAL(balanceChanged(qint64, qint64, qint64)), this, SLOT(OrderRefresh()));
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
    int pop = populateRPCTransactionObject(txid, &txobj, "");
    std::string strTXText = write_string(Value(txobj), false) + "\n";
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
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
    dlgLayout->addWidget(buttonBox);
    txDlg->setLayout(dlgLayout);
    txDlg->resize(700, 360);
    connect(buttonBox, SIGNAL(accepted()), txDlg, SLOT(accept()));
    txDlg->setAttribute(Qt::WA_DeleteOnClose); //delete once it's closed
    if (txDlg->exec() == QDialog::Accepted) { } else { } //do nothing but close
}

void TXHistoryDialog::accept()
{

}
