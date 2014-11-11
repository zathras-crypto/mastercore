// Copyright (c) 2011-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "orderhistorydialog.h"
#include "ui_orderhistorydialog.h"

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

#include "orderlistdelegate.h"

OrderHistoryDialog::OrderHistoryDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::orderHistoryDialog),
    model(0)
{
    ui->setupUi(this);
    this->model = model;

    ui->orderHistoryLW->setItemDelegate(new ListDelegate(ui->orderHistoryLW));

    QListWidgetItem *item = new QListWidgetItem();
    item->setData(Qt::DisplayRole, "6525ca23bb51022086d06d80d91243548d2d1ff546369fcfb187a18fd006df59");
    item->setData(Qt::UserRole + 1, "Sell 10.12345678 MSC for 12.4566774 SPT #3");
    item->setData(Qt::UserRole + 2, "99999.12345678 SPT #3");
    item->setData(Qt::UserRole + 3, "1234.12345678 MSC");
    item->setData(Qt::UserRole + 4, "Awaiting Confirmation");
    ui->orderHistoryLW->addItem(item);

    QListWidgetItem *item2 = new QListWidgetItem();
    item2->setData(Qt::DisplayRole, "6525ca23bb51022086d06d80d91243548d2d1ff546369fcfb187a18fd006df59");
    item2->setData(Qt::UserRole + 1, "Sell 10.12345678 MSC for 12.4566774 SPT #3");
    item2->setData(Qt::UserRole + 2, "99999.12345678 SPT #3");
    item2->setData(Qt::UserRole + 3, "1234.12345678 MSC");
    item2->setData(Qt::UserRole + 4, "Awaiting Confirmation");
    ui->orderHistoryLW->addItem(item2);

    QListWidgetItem *item3 = new QListWidgetItem();
    item3->setData(Qt::DisplayRole, "6525ca23bb51022086d06d80d91243548d2d1ff546369fcfb187a18fd006df59");
    item3->setData(Qt::UserRole + 1, "Sell 10.12345678 MSC for 12.4566774 SPT #3");
    item3->setData(Qt::UserRole + 2, "99999.12345678 SPT #3");
    item3->setData(Qt::UserRole + 3, "1234.12345678 MSC");
    item3->setData(Qt::UserRole + 4, "Awaiting Confirmation");
    ui->orderHistoryLW->addItem(item3);

    QListWidgetItem *item4 = new QListWidgetItem();
    item4->setData(Qt::DisplayRole, "6525ca23bb51022086d06d80d91243548d2d1ff546369fcfb187a18fd006df59");
    item4->setData(Qt::UserRole + 1, "Sell 10.12345678 MSC for 12.4566774 SPT #3");
    item4->setData(Qt::UserRole + 2, "99999.12345678 SPT #3");
    item4->setData(Qt::UserRole + 3, "1234.12345678 MSC");
    item4->setData(Qt::UserRole + 4, "Awaiting Confirmation");
    ui->orderHistoryLW->addItem(item4);

    QListWidgetItem *item5 = new QListWidgetItem();
    item5->setData(Qt::DisplayRole, "6525ca23bb51022086d06d80d91243548d2d1ff546369fcfb187a18fd006df59");
    item5->setData(Qt::UserRole + 1, "Sell 10.12345678 MSC for 12.4566774 SPT #3");
    item5->setData(Qt::UserRole + 2, "99999.12345678 SPT #3");
    item5->setData(Qt::UserRole + 3, "1234.12345678 MSC");
    item5->setData(Qt::UserRole + 4, "Awaiting Confirmation");
    ui->orderHistoryLW->addItem(item5);

    QListWidgetItem *item6 = new QListWidgetItem();
    item6->setData(Qt::DisplayRole, "6525ca23bb51022086d06d80d91243548d2d1ff546369fcfb187a18fd006df59");
    item6->setData(Qt::UserRole + 1, "Sell 10.12345678 MSC for 12.4566774 SPT #3");
    item6->setData(Qt::UserRole + 2, "99999.12345678 SPT #3");
    item6->setData(Qt::UserRole + 3, "1234.12345678 MSC");
    item6->setData(Qt::UserRole + 4, "Awaiting Confirmation");
    ui->orderHistoryLW->addItem(item6);

    QListWidgetItem *item7 = new QListWidgetItem();
    item7->setData(Qt::DisplayRole, "6525ca23bb51022086d06d80d91243548d2d1ff546369fcfb187a18fd006df59");
    item7->setData(Qt::UserRole + 1, "Sell 10.12345678 MSC for 12.4566774 SPT #3");
    item7->setData(Qt::UserRole + 2, "99999.12345678 SPT #3");
    item7->setData(Qt::UserRole + 3, "1234.12345678 MSC");
    item7->setData(Qt::UserRole + 4, "Awaiting Confirmation");
    ui->orderHistoryLW->addItem(item7);

    QListWidgetItem *item8 = new QListWidgetItem();
    item8->setData(Qt::DisplayRole, "6525ca23bb51022086d06d80d91243548d2d1ff546369fcfb187a18fd006df59");
    item8->setData(Qt::UserRole + 1, "Sell 10.12345678 MSC for 12.4566774 SPT #3");
    item8->setData(Qt::UserRole + 2, "99999.12345678 SPT #3");
    item8->setData(Qt::UserRole + 3, "1234.12345678 MSC");
    item8->setData(Qt::UserRole + 4, "Awaiting Confirmation");
    ui->orderHistoryLW->addItem(item8);


}

void OrderHistoryDialog::setModel(WalletModel *model)
{
    this->model = model;
    //connect(model, SIGNAL(balanceChanged(qint64, qint64, qint64)), this, SLOT(OrderRefresh()));
}

