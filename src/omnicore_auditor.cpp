// Omnicore auditor module

#include "omnicore_auditor.h"
#include "mastercore.h"
#include "mastercore_sp.h"
#include "mastercore_dex.h"
#include "mastercore_log.h"
#include "main.h"

#include <boost/lexical_cast.hpp>
#include <map>

using namespace mastercore;

bool auditorEnabled = true; // disable with --disableauditor startup param

std::map<uint32_t, int64_t> mapPropertyTotals;
std::map<uint256, XDOUBLE> mapMetaDExUnitPrices;
uint32_t auditorPropertyCountMainEco = 0;
uint32_t auditorPropertyCountTestEco = 0;
int64_t lastBlockProcessed = -1;

/* This function initializes the auditor
 */
void mastercore::Auditor_Initialize()
{
    // Initialize property totals map
    unsigned int propertyId;
    unsigned int nextPropIdMainEco = GetNextPropertyId(true);
    unsigned int nextPropIdTestEco = GetNextPropertyId(false);
    for (propertyId = 1; propertyId < nextPropIdMainEco; propertyId++) {
        mapPropertyTotals.insert(std::pair<uint32_t,int64_t>(propertyId,SafeGetTotalTokens(propertyId)));
    }
    for (propertyId = 2147483650; propertyId < nextPropIdTestEco; propertyId++) {
        mapPropertyTotals.insert(std::pair<uint32_t,int64_t>(propertyId,SafeGetTotalTokens(propertyId)));
    }

    // Initialize MetaDEx unit prices cache
    for (md_PropertiesMap::iterator my_it = metadex.begin(); my_it != metadex.end(); ++my_it) {
        md_PricesMap & prices = my_it->second;
        for (md_PricesMap::iterator it = prices.begin(); it != prices.end(); ++it) {
            md_Set & indexes = (it->second);
            for (md_Set::iterator it = indexes.begin(); it != indexes.end(); ++it) {
                CMPMetaDEx obj = *it;
                mapMetaDExUnitPrices.insert(std::pair<uint256,XDOUBLE>(obj.getHash(),obj.effectivePrice()));
            }
        }
    }

    // Initialize property counts
    auditorPropertyCountMainEco = nextPropIdMainEco - 1;
    auditorPropertyCountTestEco = nextPropIdTestEco - 1;

    // Log auditor startup
    audit_log("Auditor initialized\n");
}

/* This function handles auditor functions for the beginning of a block
 */
void mastercore::Auditor_NotifyBlockStart(CBlockIndex const * pBlockIndex)
{
    // DEBUG : Log that auditor informed a new block
    if (omni_debug_auditor_verbose) audit_log("Auditor was notified block %d has begun processing\n", pBlockIndex->nHeight);

    // Is this the first block processed or is this block number as expected?
    if (lastBlockProcessed == -1) {
        lastBlockProcessed = pBlockIndex->nHeight;
    } else {
        if (lastBlockProcessed+1 == pBlockIndex->nHeight) {
            if (omni_debug_auditor_verbose) audit_log("Auditor did not detect processing of an out of order block (%d)\n", pBlockIndex->nHeight);
            lastBlockProcessed = pBlockIndex->nHeight;
        } else { // audit failure - case should never occur but safety check
            AuditFail(strprintf("Auditor has detected processing of a block in a non-sequential order (%d)\n", pBlockIndex->nHeight));
        }
    }
}

/* This function handles auditor functions for the end of a block
 */
void mastercore::Auditor_NotifyBlockFinish(CBlockIndex const * pBlockIndex)
{
    // DEBUG : Log that auditor informed block processing completed
    if (omni_debug_auditor_verbose) audit_log("Auditor was notified block %d has been processed\n", pBlockIndex->nHeight);

    // Verify that the finish notification is for the correct block (otherwise the auditor has somehow missed a/some block(s)
    if (pBlockIndex->nHeight != lastBlockProcessed) { // audit failure
        AuditFail(strprintf("Auditor has received unexpected notification of completion of block %d\n", pBlockIndex->nHeight));
    }

    // Compare the property totals from the state with the auditor property totals map
    int mismatch = ComparePropertyTotals();
    if (mismatch == 0) {
        if (omni_debug_auditor_verbose) audit_log("Auditor did not detect any inconsistencies in property token totals following block %d\n", pBlockIndex->nHeight);
    } else { // audit failure
        AuditFail(strprintf("Auditor has detected inconsistencies in the amount of tokens for property %u following block %d\n", mismatch, pBlockIndex->nHeight));
    }

    // Compare the number of properties in state with cache
    bool comparePropertyCount = ComparePropertyCounts();
    if (comparePropertyCount) {
        if (omni_debug_auditor_verbose) audit_log("Auditor did not detect any inconsistencies in the total number of properties following block %d\n", pBlockIndex->nHeight);
    } else { // audit failure
        AuditFail(strprintf("Auditor has detected inconsistencies in the total number of properties following block %d\n", pBlockIndex->nHeight));
    }

    // Check the MetaDEx does not have any bad trades present
    std::string reasonText;
    uint256 badTrade = SearchForBadTrades(reasonText);
    if (badTrade == 0) {
        if (omni_debug_auditor_verbose) audit_log("Auditor did not detect any problems in the MetaDEx maps following block %d\n", pBlockIndex->nHeight);
    } else { // audit failure
        AuditFail(strprintf("Auditor has detected an invalid trade (txid: %s) present in the MetaDEx following block %d\nReason: %s\n",
            badTrade.GetHex().c_str(), pBlockIndex->nHeight, reasonText.c_str()));
    }
}

/* This function updates the property totals cache when an increase occurs
 */
void mastercore::Auditor_NotifyPropertyTotalChanged(bool increase, uint32_t propertyId, int64_t amount, std::string const& reasonStr)
{
    if (propertyId == 0) AuditFail("Auditor was notified of a property total change for property ID zero\n");
    if (amount <= 0) AuditFail(strprintf("Auditor was notified of a property total change with an invalid amount (%ld) for property %u due to %s\n", amount, propertyId, reasonStr.c_str()));
    std::map<uint32_t,int64_t>::iterator it = mapPropertyTotals.find(propertyId);
    if (it != mapPropertyTotals.end()) {
        int64_t cachedValue = it->second;
        int64_t stateValue = SafeGetTotalTokens(propertyId);
        int64_t newValue;
        if (increase) { newValue = cachedValue + amount; } else { newValue = cachedValue - amount; }
        if (newValue == stateValue) { // additional sanity check - cached + amount increased should equal state total
            it->second = newValue;
            if ((omni_debug_auditor_verbose) || ((omni_debug_auditor) && (reasonStr.length() > 7) && (reasonStr.substr(0,7) != "Dev MSC"))) {
                audit_log("Auditor was notified of %s of %ld tokens for property %u due to %s\n", increase ? "an increase" : "a decrease", amount, propertyId, reasonStr.c_str());
            }
        } else { // audit failure - sanity check failed
            AuditFail(strprintf("Auditor has detected inconsistencies when attempting to update the total amount of tokens for property %u\n"
                "Type:%s  Total cached:%ld, Amount changed:%ld, Total cached after update:%ld, Total from state:%ld\n",
                propertyId, increase ? "increase" : "decrease", cachedValue, amount, newValue, stateValue));
        }
    } else { // audit failure - property not found
        AuditFail(strprintf("Auditor has detected inconsistencies when attempting to update the total amount of tokens for non-existent property %u\n",propertyId));
    }
}

/* This function adds a new property to the cached totals map
 */
void mastercore::Auditor_NotifyPropertyCreated(uint32_t propertyId)
{
    if (propertyId == 0) AuditFail("Auditor was notified of a property creation with property ID zero\n");
    std::map<uint32_t,int64_t>::iterator it = mapPropertyTotals.find(propertyId);
    if((it == mapPropertyTotals.end()) && ((propertyId == auditorPropertyCountMainEco + 1) || (propertyId == auditorPropertyCountTestEco + 1))) { // check it does not already exist and is sequential
        mapPropertyTotals.insert(std::pair<uint32_t,int64_t>(propertyId,0));
        if (!isTestEcosystemProperty(propertyId)) { auditorPropertyCountMainEco += 1; } else { auditorPropertyCountTestEco += 1; }
        if (omni_debug_auditor) audit_log("Auditor was notified of a new property creation with ID %u\n", propertyId);
    } else { // audit failure - new property, it should not already exist in cached totals map
        AuditFail(strprintf("Auditor has detected a duplicated or non sequential property ID when attempting to insert property %u\n",propertyId));
    }
}

/* This function adds a new trade to the unit prices cache
 */
void mastercore::Auditor_NotifyTradeCreated(uint256 txid, XDOUBLE effectivePrice)
{
    if (txid == 0) AuditFail("Auditor was notified of a new trade with an invalid txid\n");
    if (effectivePrice <= 0) AuditFail(strprintf("Auditor was notified of a new trade with a zero or negative price: (%s) (txid: %s)\n", effectivePrice.str(DISPLAY_PRECISION_LEN,std::ios_base::fixed), txid.GetHex()));
    std::map<uint256,XDOUBLE>::iterator iter = mapMetaDExUnitPrices.find(txid);
    if (iter == mapMetaDExUnitPrices.end()) {
        mapMetaDExUnitPrices.insert(std::pair<uint256,XDOUBLE>(txid,effectivePrice));
        if (omni_debug_auditor) audit_log("Auditor was notified of a new trade with price %s (txid: %s)\n",
            effectivePrice.str(DISPLAY_PRECISION_LEN,std::ios_base::fixed),txid.GetHex());
    } else { // audit failure - the same new trade cannot be processed twice
        AuditFail(strprintf("Auditor has detected an attempt to add a new trade that has already been processed (txid: %s)\n",txid.GetHex()));
    }
}

/* This function handles an audit failure
 */
void AuditFail(const std::string& msg)
{
    audit_log("%s\n", msg);
    if (!GetBoolArg("-overrideforcedshutdown", false))
        AbortOmniNode("Shutting down due to audit failure.  Please check mastercore.log for details");
}

/* This function searches the MetaDEx maps for any invalid trades
 *
 * NOTE: Invalid trades in the context of the auditor are defined as:
 *       - Trades with zero amounts for sale or desired
 *       - Trades with price or effective prices of zero
 *       - Trades where the price or effective price has been modified
 */
uint256 SearchForBadTrades(std::string &reasonText)
{
    for (md_PropertiesMap::iterator my_it = metadex.begin(); my_it != metadex.end(); ++my_it) {
        md_PricesMap & prices = my_it->second;
        for (md_PricesMap::iterator it = prices.begin(); it != prices.end(); ++it) {
            XDOUBLE price = (it->first);
            md_Set & indexes = (it->second);
            for (md_Set::iterator it = indexes.begin(); it != indexes.end(); ++it) {
                CMPMetaDEx obj = *it;
                if (0 >= obj.getAmountDesired()) { reasonText = "Amount desired equals zero"; return obj.getHash(); }
                if (0 >= obj.getAmountForSale()) { reasonText = "Amount forsale equals zero"; return obj.getHash(); }
                if (0 >= price) { reasonText = "Price index equals zero"; return obj.getHash(); }
                if (0 >= obj.effectivePrice()) { reasonText = "Effective price equals zero"; return obj.getHash(); }
                std::map<uint256,XDOUBLE>::iterator iter = mapMetaDExUnitPrices.find(obj.getHash());
                if (iter==mapMetaDExUnitPrices.end()) {
                    reasonText = "Auditor was not notified of this trade";
                    return obj.getHash();
                } else {
                    if (obj.effectivePrice()!=iter->second) {
                    reasonText = "Effective price has changed since original trade\nOriginal price:";
                    reasonText += iter->second.str(125,std::ios_base::fixed) + "\n   State price:";
                    reasonText += obj.effectivePrice().str(125,std::ios_base::fixed);
                    return obj.getHash();
                    }
                }
            }
        }
    }
    return 0;
}

/* This function compares the cached number of properties with the state
 */
bool ComparePropertyCounts()
{
    unsigned int nextPropIdMainEco = GetNextPropertyId(true);
    unsigned int nextPropIdTestEco = GetNextPropertyId(false);
    if ((nextPropIdMainEco == auditorPropertyCountMainEco + 1) && (nextPropIdTestEco == auditorPropertyCountTestEco + 1)) {
         return true;
    } else {
         return false;
    }
}

/* This function compares the current state with the cached property totals map
 */
uint32_t ComparePropertyTotals()
{
    unsigned int propertyId;
    unsigned int nextPropIdMainEco = GetNextPropertyId(true);
    unsigned int nextPropIdTestEco = GetNextPropertyId(false);
    for (propertyId = 1; propertyId < nextPropIdMainEco; propertyId++) {
        std::map<uint32_t,int64_t>::iterator it = mapPropertyTotals.find(propertyId);
        if(it == mapPropertyTotals.end()) { return propertyId; } else { if (SafeGetTotalTokens(propertyId) != it->second) return propertyId; }
    }
    for (propertyId = 2147483650; propertyId < nextPropIdTestEco; propertyId++) {
        std::map<uint32_t,int64_t>::iterator it = mapPropertyTotals.find(propertyId);
        if(it == mapPropertyTotals.end()) { return propertyId; } else { if (SafeGetTotalTokens(propertyId) != it->second) return propertyId; }
    }
    return 0;
}

/* This function obtains the total number of tokens for propertyId from the balances tally.
 *
 * Note: similar to getTotalTokens, however getTotalTokens utilizes a shortcut to request the
 * number of tokens originally issued from the SP object for a fixed issuance property since the
 * number of tokens should never change.  However for auditing purposes we want to verify how many
 * tokens exist *now* so must avoid using such a shortcut.
 */
int64_t SafeGetTotalTokens(uint32_t propertyId)
{
  LOCK(cs_tally);
  int64_t totalTokens = 0;
  for(std::map<string, CMPTally>::iterator my_it = mp_tally_map.begin(); my_it != mp_tally_map.end(); ++my_it) {
      string address = (my_it->first).c_str();
      totalTokens += getMPbalance(address, propertyId, BALANCE);
      totalTokens += getMPbalance(address, propertyId, SELLOFFER_RESERVE);
      totalTokens += getMPbalance(address, propertyId, METADEX_RESERVE);
      if (propertyId<3) totalTokens += getMPbalance(address, propertyId, ACCEPT_RESERVE);
  }
  return totalTokens;
}

