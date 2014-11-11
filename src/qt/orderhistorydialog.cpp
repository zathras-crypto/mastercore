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

OrderHistoryDialog::OrderHistoryDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::orderHistoryDialog),
    model(0)
{
    ui->setupUi(this);
    this->model = model;
}

void OrderHistoryDialog::setModel(WalletModel *model)
{
    this->model = model;
    //connect(model, SIGNAL(balanceChanged(qint64, qint64, qint64)), this, SLOT(OrderRefresh()));
}

