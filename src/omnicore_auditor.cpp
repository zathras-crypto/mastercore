// Omnicore auditor module

#include "omnicore_auditor.h"
#include "mastercore.h"
#include "mastercore_sp.h"
#include "main.h"

#include <boost/lexical_cast.hpp>

using namespace mastercore;

bool auditorEnabled = true; // dinable with --disableauditor startup param
int omni_debug_auditor = 1; // TO BE REMOVED WHEN DEXX's LOGGER CHANGES GO IN & REPLACE PRINTF's WITH LOGS
int omni_debug_auditor_verbose = 0; // TO BE REMOVED WHEN DEXX's LOGGER CHANGES GO IN & REPLACE PRINTF's WITH LOGS

std::map<uint32_t, int64_t> mapPropertyTotals;
uint32_t auditorPropertyCountMainEco = 0;
uint32_t auditorPropertyCountTestEco = 0;

/* This function initializes the auditor
 */
void mastercore::Auditor_Initialize()
{
    // Log auditor startup
    printf("Auditor initialized\n");

    // Initialize property totals map
    unsigned int propertyId;
    unsigned int nextPropIdMainEco = GetNextPropertyId(true);
    unsigned int nextPropIdTestEco = GetNextPropertyId(false);
    for (propertyId = 1; propertyId < nextPropIdMainEco; propertyId++) {
        mapPropertyTotals.insert(std::pair<uint32_t,int64_t>(propertyId,getTotalTokens(propertyId)));
    }
    for (propertyId = 2147483650; propertyId < nextPropIdTestEco; propertyId++) {
        mapPropertyTotals.insert(std::pair<uint32_t,int64_t>(propertyId,getTotalTokens(propertyId)));
    }

    // Initialize property counts
    auditorPropertyCountMainEco = nextPropIdMainEco - 1;
    auditorPropertyCountTestEco = nextPropIdTestEco - 1;
}

/* This function handles auditor functions for the beginning of a block
 */
void mastercore::Auditor_NotifyBlockStart(CBlockIndex const * pBlockIndex)
{
    // DEBUG : Log that auditor informed a new block
    if (omni_debug_auditor_verbose) printf("Auditor was notified block %d has begun processing\n", pBlockIndex->nHeight);
}

/* This function handles auditor functions for the end of a block
 */
void mastercore::Auditor_NotifyBlockFinish(CBlockIndex const * pBlockIndex)
{
    // DEBUG : Log that auditor informed block processing completed
    if (omni_debug_auditor_verbose) printf("Auditor was notified block %d has been processed\n", pBlockIndex->nHeight);

    // Compare the property totals from the state with the auditor property totals map
    int mismatch = ComparePropertyTotals();
    if (mismatch == 0) {
        if (omni_debug_auditor_verbose) printf("Auditor did not detect any inconsistencies in property token totals following block %d\n", pBlockIndex->nHeight);
    } else { // audit failure
        printf("Auditor has detected inconsistencies in the amount of tokens for property %u following block %d\n", mismatch, pBlockIndex->nHeight);
        if (!GetBoolArg("-overrideforcedshutdown", false)) AbortNode("Shutting down due to audit failure");
    }

    // Compare the number of properties in state with cache
    bool comparePropertyCount = ComparePropertyCounts();
    if (comparePropertyCount) {
        if (omni_debug_auditor_verbose) printf("Auditor did not detect any inconsistencies in the total number of properties following block %d\n", pBlockIndex->nHeight);
    } else { // audit failure
        printf("Auditor has detected inconsistencies in the total number of properties following block %d\n", pBlockIndex->nHeight);
        if (!GetBoolArg("-overrideforcedshutdown", false)) AbortNode("Shutting down due to audit failure");
    }
}

/* This function updates the property totals cache when an increase occurs
 */
void mastercore::Auditor_NotifyPropertyTotalChanged(bool increase, uint32_t propertyId, int64_t amount, std::string const& reasonStr)
{
    std::map<uint32_t,int64_t>::iterator it = mapPropertyTotals.find(propertyId);
    if(it != mapPropertyTotals.end()) {
        int64_t cachedValue = it->second;
        int64_t stateValue = getTotalTokens(propertyId);
        int64_t newValue;
        if (increase) { newValue = cachedValue + amount; } else { newValue = cachedValue - amount; }
        if (newValue == stateValue) { // additional sanity check - cached + amount increased should equal state total
            it->second = newValue;
            if ((omni_debug_auditor_verbose) || ((omni_debug_auditor) && (reasonStr.length() > 7) && (reasonStr.substr(0,7) != "Dev MSC"))) {
                printf("Auditor was notified of %s of %ld tokens for property %u due to %s\n", increase ? "an increase" : "a decrease", amount, propertyId, reasonStr.c_str());
            }
        } else { // audit failure - sanity check failed
            printf("Auditor has detected inconsistencies when attempting to update the total amount of tokens for property %u\n",propertyId);
            printf("Type:%s  Amount cached:%ld  Amount changed:%ld  Amount updated:%ld  Amount state:%ld\n", increase ? "increase" : "decrease", cachedValue, amount, newValue, stateValue);
            if (!GetBoolArg("-overrideforcedshutdown", false)) AbortNode("Shutting down due to audit failure");
        }
    } else { // audit failure - property not found
        printf("Auditor has detected inconsistencies when attempting to update the total amount of tokens for non-existent property %u\n",propertyId);
        if (!GetBoolArg("-overrideforcedshutdown", false)) AbortNode("Shutting down due to audit failure");
    }
}

/* This function adds a new property to the cached totals map
 */
void mastercore::Auditor_NotifyPropertyCreated(uint32_t propertyId)
{
    std::map<uint32_t,int64_t>::iterator it = mapPropertyTotals.find(propertyId);
    if((it == mapPropertyTotals.end()) && ((propertyId == auditorPropertyCountMainEco + 1) || (propertyId == auditorPropertyCountTestEco + 1))) { // check it does not already exist and is sequential
        mapPropertyTotals.insert(std::pair<uint32_t,int64_t>(propertyId,0));
        if (!isTestEcosystemProperty(propertyId)) { auditorPropertyCountMainEco += 1; } else { auditorPropertyCountTestEco += 1; }
        if (omni_debug_auditor) printf("Auditor was notified of a new property creation with ID %u\n", propertyId);
    } else { // audit failure - new property, it should not already exist in cached totals map
        printf("Auditor has detected a duplicated or non sequential property ID when attempting to insert property %u\n",propertyId);
        if (!GetBoolArg("-overrideforcedshutdown", false)) AbortNode("Shutting down due to audit failure");
    }
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
        if(it == mapPropertyTotals.end()) { return propertyId; } else { if (getTotalTokens(propertyId) != it->second) return propertyId; }
    }
    for (propertyId = 2147483650; propertyId < nextPropIdTestEco; propertyId++) {
        std::map<uint32_t,int64_t>::iterator it = mapPropertyTotals.find(propertyId);
        if(it == mapPropertyTotals.end()) { return propertyId; } else { if (getTotalTokens(propertyId) != it->second) return propertyId; }
    }
    return 0;
}
