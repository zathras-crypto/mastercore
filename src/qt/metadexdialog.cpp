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
    ui->sellList->setColumnCount(3);
    ui->openOrders->setColumnCount(5);
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
            ui->sellList->setRowCount(currentRow + 1);
            ui->sellList->setItem(currentRow, 0, new QTableWidgetItem(pstr));
            ui->sellList->setItem(currentRow, 1, new QTableWidgetItem(tstr));
            ui->sellList->setItem(currentRow, 2, new QTableWidgetItem(mstr));
        }
        //dummy orders
        const int currentRow = ui->openOrders->rowCount();
        ui->openOrders->setRowCount(currentRow + 1);
        ui->openOrders->setItem(currentRow, 0, new QTableWidgetItem("1FakeBitcoinAddressDoNotSend"));
        ui->openOrders->setItem(currentRow, 1, new QTableWidgetItem("Sell"));
        ui->openOrders->setItem(currentRow, 2, new QTableWidgetItem("0.00004565"));
        ui->openOrders->setItem(currentRow, 3, new QTableWidgetItem("345.45643222"));
        ui->openOrders->setItem(currentRow, 4, new QTableWidgetItem("0.015770081"));

    ui->openOrders->setHorizontalHeaderItem(0, new QTableWidgetItem("Address"));
    ui->openOrders->setHorizontalHeaderItem(1, new QTableWidgetItem("Type"));
    ui->openOrders->setHorizontalHeaderItem(2, new QTableWidgetItem("Unit Price"));
    ui->openOrders->verticalHeader()->setVisible(false);
    #if QT_VERSION < 0x050000
       ui->openOrders->horizontalHeader()->setResizeMode(0, QHeaderView::Stretch);
    #else
       ui->openOrders->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    #endif
    ui->openOrders->horizontalHeader()->resizeSection(1, 60);
    ui->openOrders->horizontalHeader()->resizeSection(2, 140);
    ui->openOrders->horizontalHeader()->resizeSection(3, 140);
    ui->openOrders->horizontalHeader()->resizeSection(4, 140);
    ui->openOrders->setShowGrid(false);
    ui->openOrders->setSelectionBehavior(QAbstractItemView::SelectRows);

    ui->buyList->setHorizontalHeaderItem(0, new QTableWidgetItem("Unit Price"));
    ui->buyList->setHorizontalHeaderItem(1, new QTableWidgetItem("Total SP#3"));
    ui->buyList->setHorizontalHeaderItem(2, new QTableWidgetItem("Total MSC"));
    ui->buyList->verticalHeader()->setVisible(false);
    ui->buyList->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
    ui->buyList->setShowGrid(false);
    ui->buyList->setSelectionBehavior(QAbstractItemView::SelectRows);

    ui->sellList->setHorizontalHeaderItem(0, new QTableWidgetItem("Unit Price"));
    ui->sellList->setHorizontalHeaderItem(1, new QTableWidgetItem("Total SP#3"));
    ui->sellList->setHorizontalHeaderItem(2, new QTableWidgetItem("Total MSC"));
    ui->sellList->verticalHeader()->setVisible(false);
    ui->sellList->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
    ui->sellList->setShowGrid(false);
    ui->sellList->setSelectionBehavior(QAbstractItemView::SelectRows);

    connect(ui->switchButton, SIGNAL(clicked()), this, SLOT(switchButtonClicked()));
    connect(ui->sellAddressCombo, SIGNAL(activated(int)), this, SLOT(sellAddressComboBoxChanged(int)));
    connect(ui->buyAddressCombo, SIGNAL(activated(int)), this, SLOT(buyAddressComboBoxChanged(int)));

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
        // check if trying to trade against self
        if ((searchPropertyId == 1) || (searchPropertyId == 2))
        {
            //todo add property cannot be traded against self messgevox
            return;
        }
        // check if property exists
        bool spExists = _my_sps->hasSP(searchPropertyId);
        if (!spExists)
        {
            //todo add property not found messagebox
            return;
        }
        else
        {
            global_metadex_market = searchPropertyId;
            FullRefresh();
        }
    }
}

void MetaDExDialog::UpdateSellAddress()
{
    unsigned int propertyId = global_metadex_market;
    bool divisible = isPropertyDivisible(propertyId);
    QString currentSetSellAddress = ui->sellAddressCombo->currentText();
    int64_t balanceAvailable = getUserAvailableMPbalance(currentSetSellAddress.toStdString(), propertyId);
    string labStr;
    if (divisible)
    {
        labStr = "Your balance: " + FormatDivisibleMP(balanceAvailable) + " SPT";
    }
    else
    {
        labStr = "Your balance: " + FormatIndivisibleMP(balanceAvailable) + " SPT";
    }
    QString qLabStr = QString::fromStdString(labStr);
    ui->yourSellBalanceLabel->setText(qLabStr);
}

void MetaDExDialog::UpdateBuyAddress()
{
    unsigned int propertyId = global_metadex_market;
    bool testeco = false;
    if (propertyId >= TEST_ECO_PROPERTY_1) testeco = true;
    QString currentSetBuyAddress = ui->buyAddressCombo->currentText();
    int64_t balanceAvailable;
    string tokenStr;
    if (testeco)
    {
        balanceAvailable = getUserAvailableMPbalance(currentSetBuyAddress.toStdString(), MASTERCOIN_CURRENCY_TMSC);
        tokenStr = " TMSC";
    }
    else
    {
        balanceAvailable = getUserAvailableMPbalance(currentSetBuyAddress.toStdString(), MASTERCOIN_CURRENCY_MSC);
        tokenStr = " MSC";

    }
    string labStr = "Your balance: " + FormatDivisibleMP(balanceAvailable) + tokenStr;
    QString qLabStr = QString::fromStdString(labStr);
    ui->yourBuyBalanceLabel->setText(qLabStr);
}

void MetaDExDialog::FullRefresh()
{
    // populate from address selector
    unsigned int propertyId = global_metadex_market;
    bool testeco = false;
    if (propertyId >= TEST_ECO_PROPERTY_1) testeco = true;
    LOCK(cs_tally);

    // get currently selected addresses
    QString currentSetBuyAddress = ui->buyAddressCombo->currentText();
    QString currentSetSellAddress = ui->sellAddressCombo->currentText();

    // clear address selectors
    ui->buyAddressCombo->clear();
    ui->sellAddressCombo->clear();

    // update form labels
    if (testeco)
    {
        ui->exchangeLabel->setText("Exchange - SP#" + QString::fromStdString(FormatIndivisibleMP(propertyId)) + "/TMSC");
        ui->buyList->setHorizontalHeaderItem(2, new QTableWidgetItem("TMSC"));
        ui->sellList->setHorizontalHeaderItem(2, new QTableWidgetItem("TMSC"));
        ui->buyTotalLabel->setText("0.00000000 TMSC");
        ui->sellTotalLabel->setText("0.00000000 TMSC");
        ui->openOrders->setHorizontalHeaderItem(3, new QTableWidgetItem("SP#" + QString::fromStdString(FormatIndivisibleMP(propertyId))));
        ui->openOrders->setHorizontalHeaderItem(4, new QTableWidgetItem("TMSC"));
    }
    else
    {
        ui->exchangeLabel->setText("Exchange - SP#" + QString::fromStdString(FormatIndivisibleMP(propertyId)) + "/MSC");
        ui->buyList->setHorizontalHeaderItem(2, new QTableWidgetItem("MSC"));
        ui->sellList->setHorizontalHeaderItem(2, new QTableWidgetItem("MSC"));
        ui->buyTotalLabel->setText("0.00000000 MSC");
        ui->sellTotalLabel->setText("0.00000000 MSC");
        ui->openOrders->setHorizontalHeaderItem(3, new QTableWidgetItem("SP#" + QString::fromStdString(FormatIndivisibleMP(propertyId))));
        ui->openOrders->setHorizontalHeaderItem(4, new QTableWidgetItem("MSC"));
    }

    ui->buyMarketLabel->setText("BUY SP#" + QString::fromStdString(FormatIndivisibleMP(propertyId)));
    ui->sellMarketLabel->setText("SELL SP#" + QString::fromStdString(FormatIndivisibleMP(propertyId)));
    ui->buyList->setHorizontalHeaderItem(1, new QTableWidgetItem("SP#" + QString::fromStdString(FormatIndivisibleMP(propertyId))));
    ui->sellList->setHorizontalHeaderItem(1, new QTableWidgetItem("SP#" + QString::fromStdString(FormatIndivisibleMP(propertyId))));
    ui->sellButton->setText("Sell SP#" + QString::fromStdString(FormatIndivisibleMP(propertyId)));
    ui->buyButton->setText("Buy SP#" + QString::fromStdString(FormatIndivisibleMP(propertyId)));

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

    // attempt to set buy and sell addresses back to values before refresh
    int sellIdx = ui->sellAddressCombo->findText(currentSetSellAddress);
    if (sellIdx != -1) { ui->sellAddressCombo->setCurrentIndex(sellIdx); } // -1 means the new prop doesn't have the previously selected address
    int buyIdx = ui->buyAddressCombo->findText(currentSetBuyAddress);
    if (buyIdx != -1) { ui->buyAddressCombo->setCurrentIndex(buyIdx); } // -1 means the new prop doesn't have the previously selected address

    // update the balances
    UpdateSellAddress();
    UpdateBuyAddress();

    // silly sizing
    QRect rect = ui->openOrders->geometry();
    int tableHeight = 2 + ui->openOrders->horizontalHeader()->height();
    for(int i = 0; i < ui->openOrders->rowCount(); i++){
        tableHeight += ui->openOrders->rowHeight(i);
    }
    rect.setHeight(tableHeight);
    ui->openOrders->setGeometry(rect);
}

void MetaDExDialog::buyAddressComboBoxChanged(int idx)
{
    UpdateBuyAddress();
}

void MetaDExDialog::sellAddressComboBoxChanged(int idx)
{
    UpdateSellAddress();
}

void MetaDExDialog::switchButtonClicked()
{
    SwitchMarket();
}

