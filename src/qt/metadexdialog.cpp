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

    //open
    global_metadex_market = 3;

    //prep lists
    ui->buyList->setColumnCount(3);
        //dummy data
        for(int i=0; i<5; ++i)
        {
            string strprice = "349.00000006";
            string strtotal = "123456.00000001";
            string strmsctotal = "123456.00000001";
            QString pstr = QString::fromStdString(strprice);
            QString tstr = QString::fromStdString(strtotal);
            QString mstr = QString::fromStdString(strmsctotal);
            if (!ui->buyList) { printf("metadex dialog error\n"); return; }
            const int currentRow = ui->buyList->rowCount();
            ui->buyList->setRowCount(currentRow + 1);
            ui->buyList->setItem(currentRow, 0, new QTableWidgetItem(pstr));
            ui->buyList->setItem(currentRow, 1, new QTableWidgetItem(tstr));
            ui->buyList->setItem(currentRow, 2, new QTableWidgetItem(mstr));
        }
    ui->buyList->setHorizontalHeaderItem(0, new QTableWidgetItem("Unit Price"));
    ui->buyList->setHorizontalHeaderItem(1, new QTableWidgetItem("Total SP#3"));
    ui->buyList->setHorizontalHeaderItem(2, new QTableWidgetItem("Total MSC"));
    ui->buyList->verticalHeader()->setVisible(false);
    ui->buyList->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
    ui->buyList->setShowGrid(false);
    ui->buyList->setSelectionBehavior(QAbstractItemView::SelectRows);

    connect(ui->switchButton, SIGNAL(clicked()), this, SLOT(switchButtonClicked()));

    FullRefresh();

}

void MetaDExDialog::SwitchMarket()
{
    uint64_t searchPropertyId = 0;
    // first let's check if we have a searchText, if not do nothing
    string searchText = ui->switchLineEdit->text().toStdString();
    if (searchText.empty()) return;

    // try seeing if we have a numerical search string, if so treat it as a property ID search
    try
    {
        searchPropertyId = boost::lexical_cast<int64_t>(searchText);
    }
    catch(const boost::bad_lexical_cast &e) { return; } // bad cast to number

    if ((searchPropertyId > 0) && (searchPropertyId < 4294967290)) // sanity check
    {
        // check if property exists
        bool spExists = _my_sps->hasSP(searchPropertyId);
        if (!spExists)
        {
            return;
        }
        else
        {
            global_metadex_market = searchPropertyId;
            FullRefresh();
        }
    }
}

void MetaDExDialog::FullRefresh()
{
    // populate from address selector
    unsigned int propertyId = global_metadex_market;
    bool testeco = false;
    if (propertyId > TEST_ECO_PROPERTY_1) testeco = true;
    LOCK(cs_tally);

    // update form labels
    if (testeco)
    {
        ui->exchangeLabel->setText("Exchange - SP#" + QString::fromStdString(FormatIndivisibleMP(propertyId)) + "/TMSC");
    }
    else
    {
        ui->exchangeLabel->setText("Exchange - SP#" + QString::fromStdString(FormatIndivisibleMP(propertyId)) + "/MSC");
    }
    ui->buyMarketLabel->setText("BUY SP#" + QString::fromStdString(FormatIndivisibleMP(propertyId)));
    ui->sellMarketLabel->setText("SELL SP#" + QString::fromStdString(FormatIndivisibleMP(propertyId)));

    // sell addresses
    for(map<string, CMPTally>::iterator my_it = mp_tally_map.begin(); my_it != mp_tally_map.end(); ++my_it)
    {
        string address = (my_it->first).c_str();
        unsigned int id;
        bool includeAddress=false;
        (my_it->second).init();
        while (0 != (id = (my_it->second).next()))
        {
            if(id==propertyId) { includeAddress=true; break; }
        }
        if (!includeAddress) continue; //ignore this address, has never transacted in this propertyId
        if (!IsMyAddress(address)) continue; //ignore this address, it's not ours
        ui->sellAddressCombo->addItem((my_it->first).c_str());
    }
    // buy addresses
    for(map<string, CMPTally>::iterator my_it = mp_tally_map.begin(); my_it != mp_tally_map.end(); ++my_it)
    {
        string address = (my_it->first).c_str();
        unsigned int id;
        bool includeAddress=false;
        (my_it->second).init();
        while (0 != (id = (my_it->second).next()))
        {
            if((id==MASTERCOIN_CURRENCY_MSC) && (!testeco)) { includeAddress=true; break; }
            if((id==MASTERCOIN_CURRENCY_TMSC) && (testeco)) { includeAddress=true; break; }
        }
        if (!includeAddress) continue; //ignore this address, has never transacted in this propertyId
        if (!IsMyAddress(address)) continue; //ignore this address, it's not ours
        ui->buyAddressCombo->addItem((my_it->first).c_str());
    }
}

void MetaDExDialog::switchButtonClicked()
{
    SwitchMarket();
}

