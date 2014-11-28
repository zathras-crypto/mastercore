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
#include "mastercore_parse_string.h"

#include <QDateTime>
#include <QMessageBox>
#include <QScrollBar>
#include <QTextDocument>

#include "txlistdelegate.h"

TXHistoryDialog::TXHistoryDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::txHistoryDialog),
    model(0)
{
    ui->setupUi(this);
    this->model = model;
    ui->txHistoryLW->setItemDelegate(new TXListDelegate(ui->txHistoryLW));

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
                string address;
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
                        address = mp_obj.getSender();

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
                string displayType = MPTxType;
                string displayAmount;
                string displayToken;
                string displayValid;
                string displayAddress = address;
                if (divisible) { displayAmount = FormatDivisibleMP(amount); } else { displayAmount = FormatIndivisibleMP(amount); }
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
                QDateTime txTime;
                txTime.setTime_t(nTime);
                QString txTimeStr = txTime.toString(Qt::SystemLocaleShortDate);
                qItem->setData(Qt::UserRole + 1, QString::fromStdString(displayType));
                qItem->setData(Qt::UserRole + 2, QString::fromStdString(displayAmount + " " + displayToken));
                qItem->setData(Qt::UserRole + 3, QString::fromStdString(displayDirection));
                qItem->setData(Qt::UserRole + 4, QString::fromStdString(displayAddress));
                qItem->setData(Qt::UserRole + 5, txTimeStr);
                qItem->setData(Qt::UserRole + 6, QString::fromStdString(displayValid));
                ui->txHistoryLW->addItem(qItem);
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

