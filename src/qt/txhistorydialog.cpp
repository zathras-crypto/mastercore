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

void TXHistoryDialog::CreateRow(int rowcount, bool valid, bool bInbound, int confirmations, std::string txTimeStr, std::string displayType, std::string displayAddress, std::string displayAmount, std::string txidStr, bool fundsMoved)
{
    QIcon ic = QIcon(":/icons/transaction_0");
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
    ui->txHistoryTable->setRowCount(rowcount+1);
    QTableWidgetItem *dateCell = new QTableWidgetItem(QString::fromStdString(txTimeStr));
    QTableWidgetItem *typeCell = new QTableWidgetItem(QString::fromStdString(displayType));
    QTableWidgetItem *addressCell = new QTableWidgetItem(QString::fromStdString(displayAddress));
    QTableWidgetItem *amountCell = new QTableWidgetItem(QString::fromStdString(displayAmount));
    QTableWidgetItem *iconCell = new QTableWidgetItem;
    QTableWidgetItem *txidCell = new QTableWidgetItem(QString::fromStdString(txidStr));
    iconCell->setIcon(ic);
    addressCell->setTextAlignment(Qt::AlignLeft + Qt::AlignVCenter);
    addressCell->setForeground(QColor("#707070"));
    amountCell->setTextAlignment(Qt::AlignRight + Qt::AlignVCenter);
    amountCell->setForeground(QColor("#EE0000"));
    if (bInbound) amountCell->setForeground(QColor("#00AA00"));
    if (!fundsMoved) amountCell->setForeground(QColor("#404040"));
    if (rowcount % 2)
    {
        amountCell->setBackground(QColor("#F0F0F0"));
        addressCell->setBackground(QColor("#F0F0F0"));
        dateCell->setBackground(QColor("#F0F0F0"));
        typeCell->setBackground(QColor("#F0F0F0"));
        txidCell->setBackground(QColor("#F0F0F0"));
        iconCell->setBackground(QColor("#F0F0F0"));
    }
    ui->txHistoryTable->setItem(rowcount, 0, iconCell);
    ui->txHistoryTable->setItem(rowcount, 1, dateCell);
    ui->txHistoryTable->setItem(rowcount, 2, typeCell);
    ui->txHistoryTable->setItem(rowcount, 3, addressCell);
    ui->txHistoryTable->setItem(rowcount, 4, amountCell);
    ui->txHistoryTable->setItem(rowcount, 5, txidCell);
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
        string senderAddress = p_pending->src;
        uint64_t propertyId = p_pending->prop;
        bool divisible = isPropertyDivisible(propertyId);
        string displayAmount;
        int64_t amount = p_pending->amount;
        string displayValid;
        string displayAddress = senderAddress;
        int64_t type = p_pending->type;
        if (divisible) { displayAmount = FormatDivisibleShortMP(amount); } else { displayAmount = FormatIndivisibleMP(amount); }
        if (propertyId < 3)
        {
            if(propertyId == 1) { displayAmount += " MSC"; }
            if(propertyId == 2) { displayAmount += " TMSC"; }
        }
        else
        {
            string s = to_string(propertyId);
            displayAmount += " SPT#" + s;
        }
        QString txTimeStr = "Unconfirmed";
        string displayType;
        bool fundsMoved = false;
        if (type == 0) { displayType = "Send"; fundsMoved = true; }
        if (type == 21) displayType = "MetaDEx Trade";
        displayAmount = "-" + displayAmount; //all pending are outbound
        CreateRow(rowcount, true, false, 0, "Unconfirmed", displayType, displayAddress, displayAmount, txidStr, fundsMoved);
        rowcount += 1;
    }

    // wallet transactions
    CWallet *wallet = pwalletMain;
    string sAddress = "";
    string addressParam = "";

    int64_t nCount = 100; //don't display more than 100 historical transactions at the moment until we can move to a cached model
    int64_t nStartBlock = 0;
    int64_t nEndBlock = 999999;

    int chainHeight = GetHeight();

    Array response; //prep an array to hold our output

    // STO has no inbound transaction, so we need to use an insert methodology here
    // get STO receipts affecting me
    string mySTOReceipts = s_stolistdb->getMySTOReceipts(addressParam);
    std::vector<std::string> vecReceipts;
    boost::split(vecReceipts, mySTOReceipts, boost::is_any_of(","), token_compress_on);
    int64_t lastTXBlock = 999999;

    // try and fix intermittent freeze on startup and while running by only updating if we can get required locks
    // avoid hang waiting for locks
    TRY_LOCK(cs_main,lckMain);
    if (!lckMain) return;
    TRY_LOCK(wallet->cs_wallet, lckWallet);
    if (!lckWallet) return;

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

            // look for an STO receipt to see if we need to insert it
            for(uint32_t i = 0; i<vecReceipts.size(); i++)
            {
                std::vector<std::string> svstr;
                boost::split(svstr, vecReceipts[i], boost::is_any_of(":"), token_compress_on);
                if(4 == svstr.size()) // make sure expected num items
                {
                    if((atoi(svstr[1]) < lastTXBlock) && (atoi(svstr[1]) > blockHeight))
                    {
                        // STO receipt insert here - add STO receipt to response array
                        uint256 hash;
                        hash.SetHex(svstr[0]);
                        string displayAddress = svstr[2];
                        Object txobj;
                        uint64_t propertyId = 0;
                        try {
                            propertyId = boost::lexical_cast<uint64_t>(svstr[3]);
                        } catch (const boost::bad_lexical_cast &e) {
                            file_log("DEBUG STO - error in converting values from leveldb\n");
                            continue; //(something went wrong)
                        }
                        bool divisible = isPropertyDivisible(propertyId);
                        QDateTime txTime;
                        CBlockIndex* pBlkIdx = chainActive[atoi(svstr[1])];
                        txTime.setTime_t(pBlkIdx->GetBlockTime());
                        QString txTimeStr = txTime.toString(Qt::SystemLocaleShortDate);
                        string displayAmount;
                        Array receiveArray;
                        uint64_t total = 0;
                        uint64_t stoFee = 0;
                        s_stolistdb->getRecipients(hash, addressParam, &receiveArray, &total, &stoFee); // get matching receipts
                        int confirmations = 1 + chainHeight - pBlkIdx->nHeight;
                        if (divisible) { displayAmount = FormatDivisibleShortMP(total); } else { displayAmount = FormatIndivisibleMP(total); }
                        if (propertyId < 3) {
                            if(propertyId == 1) { displayAmount += " MSC"; } else { displayAmount += " TMSC"; }
                        } else {
                            string s = to_string(propertyId);
                            displayAmount += " SPT#" + s;
                        }
                        CreateRow(rowcount, true, true, confirmations, txTimeStr.toStdString(), "STO Receive", displayAddress, displayAmount, hash.GetHex(), true);
                        rowcount += 1;
                    }
                }
            }
            lastTXBlock = blockHeight;
            // check if the transaction exists in txlist
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
                string displayAmount;
                string displayToken;
                string displayValid;
                string displayAddress;
                string displayType;
                if (0 < parseRC) //positive RC means payment
                {
                    string tmpBuyer;
                    string tmpSeller;
                    uint64_t total = 0;
                    uint64_t tmpVout = 0;
                    uint64_t tmpNValue = 0;
                    uint64_t tmpPropertyId = 0;
                    p_txlistdb->getPurchaseDetails(hash,1,&tmpBuyer,&tmpSeller,&tmpVout,&tmpPropertyId,&tmpNValue);
                    senderAddress = tmpBuyer;
                    refAddress = tmpSeller;
                    bool bIsBuy = IsMyAddress(senderAddress);
                    if (!bIsBuy)
                    {
                        displayType = "DEx Sell";
                        displayAddress = refAddress;
                    }
                    else
                    {
                        displayType = "DEx Buy";
                        displayAddress = senderAddress;
                    }
                    // calculate total bought/sold
                    int numberOfPurchases=p_txlistdb->getNumberOfPurchases(hash);
                    if (0<numberOfPurchases)
                    {
                        for(int purchaseNumber = 1; purchaseNumber <= numberOfPurchases; purchaseNumber++)
                        {
                            p_txlistdb->getPurchaseDetails(hash,purchaseNumber,&tmpBuyer,&tmpSeller,&tmpVout,&tmpPropertyId,&tmpNValue);
                            total += tmpNValue;
                        }
                        displayAmount = FormatDivisibleShortMP(total);
                        if(tmpPropertyId == 1) { displayAmount += " MSC"; }
                        if(tmpPropertyId == 2) { displayAmount += " TMSC"; }
                        QDateTime txTime;
                        txTime.setTime_t(nTime);
                        QString txTimeStr = txTime.toString(Qt::SystemLocaleShortDate);
                        if (!bIsBuy) displayAmount = "-" + displayAmount;
                        int confirmations = 1 + chainHeight - pBlockIndex->nHeight;
                        CreateRow(rowcount, true, bIsBuy, confirmations, txTimeStr.toStdString(), displayType, displayAddress, displayAmount, hash.GetHex(), true);
                        rowcount += 1;
                    }
                }
                if (0 == parseRC) //negative RC means no MP content/badly encoded TX, we shouldn't see this if TX in levelDB but check for sanity
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
                            // special case for property creation (getProperty cannot get ID as createdID not stored in obj)
                            if (valid) // we only generate an ID for valid creates
                            {
                                if ((mp_obj.getType() == MSC_TYPE_CREATE_PROPERTY_FIXED) ||
                                    (mp_obj.getType() == MSC_TYPE_CREATE_PROPERTY_VARIABLE) ||
                                    (mp_obj.getType() == MSC_TYPE_CREATE_PROPERTY_MANUAL))
                                    {
                                        propertyId = _my_sps->findSPByTX(hash);
                                        if (mp_obj.getType() == MSC_TYPE_CREATE_PROPERTY_FIXED)
                                        { amount = getTotalTokens(propertyId); }
                                        else
                                        { amount = 0; }
                                    }
                            }
                            divisible = isPropertyDivisible(propertyId);
                        }
                    }
                    QListWidgetItem *qItem = new QListWidgetItem();
                    qItem->setData(Qt::DisplayRole, QString::fromStdString(hash.GetHex()));
                    bool fundsMoved = true;
                    // shrink tx type
                    string displayType = "Unknown";
                    switch (mp_obj.getType())
                    {
                        case MSC_TYPE_SIMPLE_SEND: displayType = "Send"; break;
                        case MSC_TYPE_RESTRICTED_SEND: displayType = "Rest. Send"; break;
                        case MSC_TYPE_SEND_TO_OWNERS: displayType = "Send To Owners"; break;
                        case MSC_TYPE_SAVINGS_MARK: displayType = "Mark Savings"; fundsMoved = false; break;
                        case MSC_TYPE_SAVINGS_COMPROMISED: ; displayType = "Lock Savings"; break;
                        case MSC_TYPE_RATELIMITED_MARK: displayType = "Rate Limit"; break;
                        case MSC_TYPE_AUTOMATIC_DISPENSARY: displayType = "Auto Dispense"; break;
                        case MSC_TYPE_TRADE_OFFER: displayType = "DEx Trade"; fundsMoved = false; break;
                        case MSC_TYPE_METADEX: displayType = "MetaDEx Trade"; fundsMoved = false; break;
                        case MSC_TYPE_ACCEPT_OFFER_BTC: displayType = "DEx Accept"; fundsMoved = false; break;
                        case MSC_TYPE_CREATE_PROPERTY_FIXED: displayType = "Create Property"; break;
                        case MSC_TYPE_CREATE_PROPERTY_VARIABLE: displayType = "Create Property"; break;
                        case MSC_TYPE_PROMOTE_PROPERTY: displayType = "Promo Property";
                        case MSC_TYPE_CLOSE_CROWDSALE: displayType = "Close Crowdsale";
                        case MSC_TYPE_CREATE_PROPERTY_MANUAL: displayType = "Create Property"; break;
                        case MSC_TYPE_GRANT_PROPERTY_TOKENS: displayType = "Grant Tokens"; break;
                        case MSC_TYPE_REVOKE_PROPERTY_TOKENS: displayType = "Revoke Tokens"; break;
                        case MSC_TYPE_CHANGE_ISSUER_ADDRESS: displayType = "Change Issuer"; fundsMoved = false; break;
                    }
                    if (IsMyAddress(senderAddress)) { displayAddress = senderAddress; } else { displayAddress = refAddress; }
                    if (divisible) { displayAmount = FormatDivisibleShortMP(amount); } else { displayAmount = FormatIndivisibleMP(amount); }
                    if (propertyId < 3)
                    {
                        if(propertyId == 1) { displayAmount += " MSC"; }
                        if(propertyId == 2) { displayAmount += " TMSC"; }
                    }
                    else
                    {
                        string s = to_string(propertyId);
                        displayAmount += " SPT#" + s;
                    }
                    if ((displayType == "Send") && (!IsMyAddress(senderAddress))) { displayType = "Receive"; }

                    QDateTime txTime;
                    txTime.setTime_t(nTime);
                    QString txTimeStr = txTime.toString(Qt::SystemLocaleShortDate);
                    bool inbound = true;
                    if (!valid) fundsMoved = false; // funds never move in invalid txs
                    // override/hide display amount for invalid creates and unknown transactions as we
                    // can't display amount and property as no prop exists
                    if ((mp_obj.getType() == MSC_TYPE_CREATE_PROPERTY_FIXED) ||
                        (mp_obj.getType() == MSC_TYPE_CREATE_PROPERTY_VARIABLE) ||
                        (mp_obj.getType() == MSC_TYPE_CREATE_PROPERTY_MANUAL) ||
                        (displayType == "Unknown" ))
                        {
                            if (!valid) { displayAmount = "N/A"; }
                        }
                    else
                        {
                            if ((fundsMoved) && (IsMyAddress(senderAddress)))
                                { displayAmount = "-" + displayAmount; inbound = false; }
                        }
                    int confirmations = 1 + chainHeight - pBlockIndex->nHeight;
                    CreateRow(rowcount, valid, inbound, confirmations, txTimeStr.toStdString(), displayType, displayAddress, displayAmount, hash.GetHex(), fundsMoved);
                    rowcount += 1;
                }
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
            // manipulate for STO if needed
            size_t pos = strTXText.find("Send To Owners");
            if (pos!=std::string::npos) {
                Array receiveArray;
                uint64_t tmpAmount = 0;
                uint64_t tmpSTOFee = 0;
                s_stolistdb->getRecipients(txid, "", &receiveArray, &tmpAmount, &tmpSTOFee);
                txobj.push_back(Pair("recipients", receiveArray));
                //rewrite string
                strTXText = write_string(Value(txobj), false) + "\n";
            }
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
        from = "[";
        to = "[\n";
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
