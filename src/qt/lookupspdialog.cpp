// Copyright (c) 2011-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "lookupspdialog.h"
#include "ui_lookupspdialog.h"

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

LookupSPDialog::LookupSPDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LookupSPDialog),
    model(0)
{
    ui->setupUi(this);
    this->model = model;

    // populate placeholder text
    ui->searchLineEdit->setPlaceholderText("Search property ID, issuer or property name");

    // connect actions
    connect(ui->matchingComboBox, SIGNAL(activated(int)), this, SLOT(matchingComboBoxChanged(int)));
    connect(ui->searchButton, SIGNAL(clicked()), this, SLOT(searchButtonClicked()));
}

void LookupSPDialog::searchSP()
{
    // search function to lookup properties, we want this search function to be as capable as possible to
    // help users find the property they're looking for via search terms they may want to use
printf("search\n");
    int searchParamType = 0;
    string searchText = ui->searchLineEdit->text().toStdString();
    unsigned int searchPropertyId = 0;

    // first let's check if we have a searchText, if not do nothing
    if (searchText.empty()) return;

    // try seeing if we have a numerical search string, if so treat it as a property ID search
    try
    {
        searchPropertyId = boost::lexical_cast<int64_t>(searchText);
        searchParamType = 1; // search by propertyId
    }
    catch(const boost::bad_lexical_cast &e) { }
    if (searchParamType == 1 && 0 >= searchPropertyId) searchParamType = 0; // we got a number but it's <=0

    // next if not positive numerical, lets see if the string is a valid bitcoin address
    if (searchParamType == 0)
    {
        CBitcoinAddress address;
        address.SetString(searchText); // no null check on searchText required we've already checked it's not empty above
        if (address.IsValid()) searchParamType = 2; // search by address;
    }

    // if we still don't have a param we'll search against free text in the name
    if (searchParamType == 0) searchParamType = 3; // search by free text
printf("selected search type is %d\n", searchParamType);

    // clear matching results combo
    ui->matchingComboBox->clear();
    bool spExists;
    unsigned int tmpPropertyId;
    unsigned int nextSPID;
    unsigned int nextTestSPID;
    unsigned int propertyId;
    QString strId;
    switch(searchParamType)
    {
        case 1: //search by property Id
           // convert search string to ID
           strId = QString::fromStdString(searchText);
           propertyId = strId.toUInt();
           // check if this property ID exists, if not no match to populate and just return
           spExists = _my_sps->hasSP(propertyId);
           if (spExists)
           {
               addSPToMatchingResults(propertyId);
               updateDisplayedProperty();
           }
           else
           {
               return;
           }
        break;
        case 2: //search by address
           // iterate through my_sps looking for the issuer address and add any properties issued by said address to matchingcombo
           // talk with @Michael @Bart to see if perhaps a more efficient way to do this, but not major issue as only run on user request
           nextSPID = _my_sps->peekNextSPID(1);
           nextTestSPID = _my_sps->peekNextSPID(2);
           for (tmpPropertyId = 1; tmpPropertyId<nextSPID; tmpPropertyId++)
           {
               CMPSPInfo::Entry sp;
               if (false != _my_sps->getSP(tmpPropertyId, sp))
               {
                   if (sp.issuer == searchText)
                   {
                       addSPToMatchingResults(tmpPropertyId);
                   }
               }
           }
           for (tmpPropertyId = TEST_ECO_PROPERTY_1; tmpPropertyId<nextTestSPID; tmpPropertyId++)
           {
               CMPSPInfo::Entry sp;
               if (false != _my_sps->getSP(tmpPropertyId, sp))
               {
                   if (sp.issuer == searchText)
                   {
                       addSPToMatchingResults(tmpPropertyId);
                   }
               }
           }
        break;
        case 3: //search by freetext
           // iterate through my_sps and see if property name contains the search text
        break;
    }

}

void LookupSPDialog::addSPToMatchingResults(unsigned int propertyId)
{
    // verify the supplied property exists (sanity check) then populate the matching results combo box
    bool spExists = _my_sps->hasSP(propertyId);
    if (spExists)
    {
        string spName;
        spName = getPropertyName(propertyId).c_str();
        if(spName.size()>30) spName=spName.substr(0,30)+"...";
        string spId = static_cast<ostringstream*>( &(ostringstream() << propertyId) )->str();
        spName += " (#" + spId + ")";
        ui->matchingComboBox->addItem(spName.c_str(),spId.c_str());
    }
    else
    {
        return;
    }
}

void LookupSPDialog::updateDisplayedProperty()
{
    QString strId = ui->matchingComboBox->itemData(ui->matchingComboBox->currentIndex()).toString();
    // protect against an empty matchedComboBox
    if (strId.toStdString().empty()) return;

    // map property Id
    unsigned int propertyId = strId.toUInt();
    CMPSPInfo::Entry sp;
    if (false == _my_sps->getSP(propertyId, sp)) { return; } // something has gone wrong, don't attempt to display non-existent property

    // populate the fields
    bool divisible=sp.isDivisible();
    if (divisible) { ui->divisibleLabel->setText("Yes"); } else { ui->divisibleLabel->setText("No"); }
    if (propertyId>2147483647) { ui->ecosystemLabel->setText("Test"); } else { ui->ecosystemLabel->setText("Production"); }
    ui->propertyIDLabel->setText(QString::fromStdString(FormatIndivisibleMP(propertyId)));
    ui->nameLabel->setText(QString::fromStdString(sp.name));
    ui->categoryLabel->setText(QString::fromStdString(sp.category));
    ui->subcategoryLabel->setText(QString::fromStdString(sp.subcategory));
    ui->dataLabel->setText(QString::fromStdString(sp.data));
    ui->urlLabel->setText(QString::fromStdString(sp.url));
    string strTotalTokens;
    string strWalletTokens;
    int64_t totalTokens = getTotalTokens(propertyId);
    int64_t walletTokens = 0;
    if (propertyId<2147483648)
    { walletTokens = global_balance_money_maineco[propertyId]; }
    else
    { walletTokens = global_balance_money_testeco[propertyId-2147483647]; }
    string tokenLabel;
    if (propertyId > 2)
    {
       tokenLabel = " SPT";
    }
    else
    {
       if (propertyId == 1) { tokenLabel = " MSC"; } else { tokenLabel = " TMSC"; }
    }
    if (divisible) { strTotalTokens = FormatDivisibleMP(totalTokens); } else { strTotalTokens = FormatIndivisibleMP(totalTokens); }
    if (divisible) { strWalletTokens = FormatDivisibleMP(walletTokens); } else { strWalletTokens = FormatIndivisibleMP(walletTokens); }
    ui->totalTokensLabel->setText(QString::fromStdString(strTotalTokens + tokenLabel));
    ui->walletBalanceLabel->setText(QString::fromStdString(strWalletTokens + tokenLabel));
    ui->issuerLabel->setText(QString::fromStdString(sp.issuer));
    // issuances are no longer just fixed or variable, this needs further code changes to sp.fixed
    //bool fixedIssuance = sp.fixed;
}

void LookupSPDialog::searchButtonClicked()
{
    searchSP();
}

void LookupSPDialog::matchingComboBoxChanged(int idx)
{
    updateDisplayedProperty();
}
