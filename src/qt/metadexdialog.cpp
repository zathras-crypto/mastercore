// Copyright (c) 2011-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "metadexdialog.h"
#include "ui_metadexdialog.h"

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

#include "mastercore.h"
using namespace mastercore;

// potentially overzealous using here
using namespace std;
using namespace boost;
using namespace boost::assign;
using namespace json_spirit;
using namespace leveldb;
// end potentially overzealous using

#include "mastercore_dex.h"
#include "mastercore_tx.h"
#include "mastercore_sp.h"

#include <QMessageBox>
#include <QScrollBar>
#include <QTextDocument>

MetaDExDialog::MetaDExDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MetaDExDialog),
    model(0)
{
    ui->setupUi(this);
    this->model = model;

//dummy data
//ui->buyList->addItem('TEST');
//ui->buyList->addItem('12345678901234567890');
//ui->buyList->addItem('1234567890  1234567890  1234567890  1234567890  ');
//ui->buyList->addItem('ABCDEFGHIJ');
//ui->buyList->addItem('ABCDEFGHIJ  IKLMNOPQRS  TVUWXKR333');
//ui->buyList->addItem('TEST');


ui->buyList->setColumnCount(3);
for(int i=0; i<5; ++i) {
    string strprice = "349.00000006";
    string strtotal = "123456.00000001";
    string strmsctotal = "123456.00000001";
//    QListWidgetItem *item=new QListWidgetItem(QIcon(":/images/Icon.png"),QString::fromStdString(strt));
//    ui->buyList->addItem(item);
QString pstr = QString::fromStdString(strprice);
QString tstr = QString::fromStdString(strtotal);
QString mstr = QString::fromStdString(strmsctotal);

    if (!ui->buyList) { printf("metadex dialog error\n"); return; }
 
    const int currentRow = ui->buyList->rowCount();  
    ui->buyList->setRowCount(currentRow + 1);
    ui->buyList->setItem(currentRow, 0, new QTableWidgetItem(pstr));
    ui->buyList->setItem(currentRow, 1, new QTableWidgetItem(tstr));
    ui->buyList->setItem(currentRow, 2, new QTableWidgetItem(mstr));

    ui->buyList->setHorizontalHeaderItem(0, new QTableWidgetItem("Unit Price"));
    ui->buyList->setHorizontalHeaderItem(1, new QTableWidgetItem("Total SP#3"));
    ui->buyList->setHorizontalHeaderItem(2, new QTableWidgetItem("Total MSC"));
    ui->buyList->verticalHeader()->setVisible(false);
    ui->buyList->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
    ui->buyList->setShowGrid(false);
    ui->buyList->setSelectionBehavior(QAbstractItemView::SelectRows);
}}
