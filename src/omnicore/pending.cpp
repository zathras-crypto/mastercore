#include "omnicore/pending.h"

#include "omnicore/log.h"
#include "omnicore/omnicore.h"
#include "omnicore/sp.h"

#include "uint256.h"

#include "json/json_spirit_value.h"
#include "json/json_spirit_writer_template.h"

#include <string>

using json_spirit::Object;
using json_spirit::Pair;
using json_spirit::Value;
using json_spirit::write_string;

namespace mastercore
{
//! Global map of pending transaction objects
PendingMap my_pending;

/**
 * Adds a transaction to the pending map using supplied parameters.
 */
void PendingAdd(const uint256& txid, const std::string& sendingAddress, const std::string& refAddress, uint16_t type, uint32_t propertyId, int64_t amount, uint32_t propertyIdDesired, int64_t amountDesired, int64_t action)
{
    Object txobj;
    std::string amountStr, amountDStr;
    bool divisible;
    bool divisibleDesired;
    txobj.push_back(Pair("txid", txid.GetHex()));
    txobj.push_back(Pair("sendingaddress", sendingAddress));
    if (!refAddress.empty()) txobj.push_back(Pair("referenceaddress", refAddress));
    txobj.push_back(Pair("confirmations", 0));
    txobj.push_back(Pair("type", c_strMasterProtocolTXType(type)));
    switch (type) {
        case MSC_TYPE_SIMPLE_SEND:
            txobj.push_back(Pair("propertyid", (uint64_t)propertyId));
            divisible = isPropertyDivisible(propertyId);
            txobj.push_back(Pair("divisible", divisible));
            if (divisible) { amountStr = FormatDivisibleMP(amount); } else { amountStr = FormatIndivisibleMP(amount); }
            txobj.push_back(Pair("amount", amountStr));
        break;
        case MSC_TYPE_SEND_TO_OWNERS:
            txobj.push_back(Pair("propertyid", (uint64_t)propertyId));
            divisible = isPropertyDivisible(propertyId);
            txobj.push_back(Pair("divisible", divisible));
            if (divisible) { amountStr = FormatDivisibleMP(amount); } else { amountStr = FormatIndivisibleMP(amount); }
            txobj.push_back(Pair("amount", amountStr));
        break;
        case MSC_TYPE_TRADE_OFFER:
            amountStr = FormatDivisibleMP(amount); // both amount for sale (either TMSC or MSC) and BTC desired are always divisible
            amountDStr = FormatDivisibleMP(amountDesired);
            txobj.push_back(Pair("amountoffered", amountStr));
            txobj.push_back(Pair("propertyidoffered", (uint64_t)propertyId));
            txobj.push_back(Pair("btcamountdesired", amountDStr));
            txobj.push_back(Pair("action", action));
        break;
        case MSC_TYPE_METADEX_TRADE:
        case MSC_TYPE_METADEX_CANCEL_PRICE:
        case MSC_TYPE_METADEX_CANCEL_PAIR:
        case MSC_TYPE_METADEX_CANCEL_ECOSYSTEM:
            divisible = isPropertyDivisible(propertyId);
            divisibleDesired = isPropertyDivisible(propertyIdDesired);
            if (divisible) { amountStr = FormatDivisibleMP(amount); } else { amountStr = FormatIndivisibleMP(amount); }
            if (divisibleDesired) { amountDStr = FormatDivisibleMP(amountDesired); } else { amountDStr = FormatIndivisibleMP(amountDesired); }
            txobj.push_back(Pair("amountoffered", amountStr));
            txobj.push_back(Pair("propertyidoffered", (uint64_t)propertyId));
            txobj.push_back(Pair("propertyidofferedisdivisible", divisible));
            txobj.push_back(Pair("amountdesired", amountDStr));
            txobj.push_back(Pair("propertyiddesired", (uint64_t)propertyIdDesired));
            txobj.push_back(Pair("propertyiddesiredisdivisible", divisibleDesired));
            txobj.push_back(Pair("action", action));
        break;
    }
    std::string txDesc = write_string(Value(txobj), true);
    if (msc_debug_pending) PrintToLog("%s(%s,%s,%s,%d,%u,%ld,%u,%ld,%d,%s)\n", __FUNCTION__, txid.GetHex(), sendingAddress, refAddress,
                                        type, propertyId, amount, propertyIdDesired, amountDesired, action, txDesc);
    if (update_tally_map(sendingAddress, propertyId, -amount, PENDING)) {
        CMPPending pending;
        pending.src = sendingAddress;
        pending.amount = amount;
        pending.prop = propertyId;
        pending.desc = txDesc;
        pending.type = type;
        my_pending.insert(std::make_pair(txid, pending));
    }
}

/**
 * Deletes a transaction from the pending map and credits the amount back to the pending tally for the address.
 *
 * NOTE: this is currently called for every bitcoin transaction prior to running through the parser.
 */
void PendingDelete(const uint256& txid)
{
    PendingMap::iterator it = my_pending.find(txid);
    if (it != my_pending.end()) {
        const CMPPending& pending = it->second;
        int64_t src_amount = getMPbalance(pending.src, pending.prop, PENDING);
        if (msc_debug_pending) PrintToLog("%s(%s): amount=%d\n", __FUNCTION__, txid.GetHex(), src_amount);
        if (src_amount) update_tally_map(pending.src, pending.prop, pending.amount, PENDING);
        my_pending.erase(it);
    }
}


} // namespace mastercore

/**
 * Prints information about a pending transaction object.
 */
void CMPPending::print(const uint256& txid) const
{
    PrintToConsole("%s : %s %d %d %d %s\n", txid.GetHex(), src, prop, amount, type, desc);
}

