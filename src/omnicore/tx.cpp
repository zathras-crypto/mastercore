// Master Protocol transaction code

#include "omnicore/tx.h"

#include "omnicore/convert.h"
#include "omnicore/dex.h"
#include "omnicore/log.h"
#include "omnicore/mdex.h"
#include "omnicore/notifications.h"
#include "omnicore/omnicore.h"
#include "omnicore/sp.h"
#include "omnicore/sto.h"

#include "alert.h"
#include "amount.h"
#include "main.h"
#include "sync.h"
#include "utiltime.h"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <stdio.h>
#include <string.h>

#include <algorithm>
#include <vector>

using boost::algorithm::token_compress_on;

using namespace mastercore;

// --------------------- DEPRECHIATED ------------------------
// Please use CMPTransaction::interpret_Transaction() instead.
// -----------------------------------------------------------

// initial packet interpret step
int CMPTransaction::step1()
{
  if (MIN_PAYLOAD_SIZE > pkt_size)  // class C packets could now be as small as 8 bytes
  {
    PrintToLog("%s() ERROR PACKET TOO SMALL; size = %d, line %d, file: %s\n", __FUNCTION__, pkt_size, __LINE__, __FILE__);
    return -(PKT_ERROR -1);
  }

  // collect version
  memcpy(&version, &pkt[0], 2);
  swapByteOrder16(version);

  // blank out version bytes in the packet
  pkt[0]=0; pkt[1]=0;
  
  memcpy(&type, &pkt[0], 4);

  swapByteOrder32(type);

  string classType;
  switch(multi) {
      case 0:
          classType = "A";
      break;
      case 1:
          classType = "B";
      break;
      case 2:
          classType = "C";
      break;
  }
  PrintToLog("\t         version: %d, Class %s\n", version, classType);
  PrintToLog("\t            type: %u (%s)\n", type, c_strMasterProtocolTXType(type));

  return (type);
}

// extract alert info for alert packets
int CMPTransaction::step2_Alert(std::string *new_global_alert_message)
{
  const char *p = 4 + (char *)&pkt;
  std::vector<std::string>spstr;
  char alertString[SP_STRING_FIELD_LEN];

  // is sender authorized?
  bool authorized = CheckAlertAuthorization(sender);

  if(!authorized)
  {
      // not authorized, ignore alert
      PrintToLog("\t      alert auth: false\n");
      return (PKT_ERROR -912);
  }
  else
  {
      // authorized, decode and make sure there are 4 tokens, then replace global_alert_message
      spstr.push_back(std::string(p));
      memcpy(alertString, spstr[0].c_str(), std::min(spstr[0].length(),sizeof(alertString)-1));

      std::vector<std::string> vstr;
      boost::split(vstr, alertString, boost::is_any_of(":"), token_compress_on);
      PrintToLog("\t      alert auth: true\n");
      PrintToLog("\t    alert sender: %s\n", sender);

      if (5 != vstr.size())
      {
          // there are not 5 tokens in the alert, badly formed alert and must discard
          PrintToLog("\t    packet error: badly formed alert != 5 tokens\n");
          return (PKT_ERROR -911);
      }
      else
      {
          int32_t alertType;
          uint64_t expiryValue;
          uint32_t typeCheck;
          uint32_t verCheck;
          string alertMessage;
          try
          {
              alertType = boost::lexical_cast<int32_t>(vstr[0]);
              expiryValue = boost::lexical_cast<uint64_t>(vstr[1]);
              typeCheck = boost::lexical_cast<uint32_t>(vstr[2]);
              verCheck = boost::lexical_cast<uint32_t>(vstr[3]);
          } catch (const boost::bad_lexical_cast &e)
            {
                  PrintToLog("DEBUG ALERT - error in converting values from global alert string\n");
                  return (PKT_ERROR -910); //(something went wrong)
            }
          alertMessage = vstr[4];
          PrintToLog("\t    message type: %llu\n",alertType);
          PrintToLog("\t    expiry value: %llu\n",expiryValue);
          PrintToLog("\t      type check: %llu\n",typeCheck);
          PrintToLog("\t       ver check: %llu\n",verCheck);
          PrintToLog("\t   alert message: %s\n", alertMessage);
          // copy the alert string into the global_alert_message and return a 0 rc
          string message(alertString);
          *new_global_alert_message=message;
          // we have a new alert, fire a notify event if needed
          CAlert::Notify(alertMessage, true);
          return 0;
      }
  }
}

// extract Value for certain types of packets
int CMPTransaction::step2_Value()
{
  memcpy(&nValue, &pkt[8], 8);
  swapByteOrder64(nValue);

  // here we are copying nValue into nNewValue to be stored into our leveldb later: MP_txlist
  nNewValue = nValue;

  memcpy(&property, &pkt[4], 4);
  swapByteOrder32(property);

  PrintToLog("\t        property: %u (%s)\n", property, strMPProperty(property));
  PrintToLog("\t           value: %s\n", FormatMP(property, nValue));

  if (MAX_INT_8_BYTES < nValue)
  {
    return (PKT_ERROR -801);  // out of range
  }

  return 0;
}

// extract Smart Property data
// RETURNS: the pointer to the next piece to be parsed
// ERROR is returns NULL and/or sets the error_code
const char *CMPTransaction::step2_SmartProperty(int &error_code)
{
const char *p = 11 + (char *)&pkt;
std::vector<std::string>spstr;
unsigned int i;
unsigned int prop_id;

  error_code = 0;

  memcpy(&ecosystem, &pkt[4], 1);
  PrintToLog("\t       Ecosystem: %u\n", ecosystem);

  // valid values are 1 & 2
  if ((OMNI_PROPERTY_MSC != ecosystem) && (OMNI_PROPERTY_TMSC != ecosystem))
  {
    error_code = (PKT_ERROR_SP -501);
    return NULL;
  }

  prop_id = _my_sps->peekNextSPID(ecosystem);

  memcpy(&prop_type, &pkt[5], 2);
  swapByteOrder16(prop_type);

  memcpy(&prev_prop_id, &pkt[7], 4);
  swapByteOrder32(prev_prop_id);

  PrintToLog("\t     Property ID: %u (%s)\n", prop_id, strMPProperty(prop_id));
  PrintToLog("\t   Property type: %u (%s)\n", prop_type, c_strPropertyType(prop_type));
  PrintToLog("\tPrev Property ID: %u\n", prev_prop_id);

  // only 1 & 2 are valid right now
  if ((MSC_PROPERTY_TYPE_INDIVISIBLE != prop_type) && (MSC_PROPERTY_TYPE_DIVISIBLE != prop_type))
  {
    error_code = (PKT_ERROR_SP -502);
    return NULL;
  }

  for (i = 0; i<5; i++)
  {
    spstr.push_back(std::string(p));
    p += spstr.back().size() + 1;
  }

  i = 0;

  memcpy(category, spstr[i].c_str(), std::min(spstr[i].length(),sizeof(category)-1)); i++;
  memcpy(subcategory, spstr[i].c_str(), std::min(spstr[i].length(),sizeof(subcategory)-1)); i++;
  memcpy(name, spstr[i].c_str(), std::min(spstr[i].length(),sizeof(name)-1)); i++;
  memcpy(url, spstr[i].c_str(), std::min(spstr[i].length(),sizeof(url)-1)); i++;
  memcpy(data, spstr[i].c_str(), std::min(spstr[i].length(),sizeof(data)-1)); i++;

  PrintToLog("\t        Category: %s\n", category);
  PrintToLog("\t     Subcategory: %s\n", subcategory);
  PrintToLog("\t            Name: %s\n", name);
  PrintToLog("\t             URL: %s\n", url);
  PrintToLog("\t            Data: %s\n", data);

  if (!IsTransactionTypeAllowed(block, prop_id, type, version))
  {
    error_code = (PKT_ERROR_SP -503);
    return NULL;
  }

  // name can not be NULL
  if ('\0' == name[0])
  {
    error_code = (PKT_ERROR_SP -505);
    return NULL;
  }

  if (!p) error_code = (PKT_ERROR_SP -510);

  if (isOverrun(p, __LINE__))
  {
    error_code = (PKT_ERROR_SP -800);
    return NULL;
  }

  return p;
}

int CMPTransaction::step3_sp_fixed(const char *p)
{
  if (!p) return (PKT_ERROR_SP -1);

  memcpy(&nValue, p, 8);
  swapByteOrder64(nValue);
  p += 8;

  // here we are copying nValue into nNewValue to be stored into our leveldb later: MP_txlist
  nNewValue = nValue;

  if (MSC_PROPERTY_TYPE_INDIVISIBLE == prop_type)
  {
    PrintToLog("\t           value: %lu\n", nValue);
    if (0 == nValue) return (PKT_ERROR_SP -101);
  }
  else
  if (MSC_PROPERTY_TYPE_DIVISIBLE == prop_type)
  {
    PrintToLog("\t           value: %lu.%08lu\n", nValue/COIN, nValue%COIN);
    if (0 == nValue) return (PKT_ERROR_SP -102);
  }

  if (MAX_INT_8_BYTES < nValue)
  {
    return (PKT_ERROR -802);  // out of range
  }

  if (isOverrun(p, __LINE__)) return (PKT_ERROR_SP -900);

  return 0;
}

int CMPTransaction::step3_sp_variable(const char *p)
{
  if (!p) return (PKT_ERROR_SP -1);

  memcpy(&property, p, 4);  // property desired
  swapByteOrder32(property);
  p += 4;

  PrintToLog("\t        property: %u (%s)\n", property, strMPProperty(property));

  memcpy(&nValue, p, 8);
  swapByteOrder64(nValue);
  p += 8;

  // here we are copying nValue into nNewValue to be stored into our leveldb later: MP_txlist
  nNewValue = nValue;

  if (MSC_PROPERTY_TYPE_INDIVISIBLE == prop_type)
  {
    PrintToLog("\t           value: %lu\n", nValue);
    if (0 == nValue) return (PKT_ERROR_SP -201);
  }
  else
  if (MSC_PROPERTY_TYPE_DIVISIBLE == prop_type)
  {
    PrintToLog("\t           value: %lu.%08lu\n", nValue/COIN, nValue%COIN);
    if (0 == nValue) return (PKT_ERROR_SP -202);
  }

  if (MAX_INT_8_BYTES < nValue)
  {
    return (PKT_ERROR -803);  // out of range
  }

  memcpy(&deadline, p, 8);
  swapByteOrder64(deadline);
  p += 8;
  PrintToLog("\t        Deadline: %s (%lX)\n", DateTimeStrFormat("%Y-%m-%d %H:%M:%S", deadline), deadline);

  if (!deadline) return (PKT_ERROR_SP -203);  // deadline cannot be 0

  // deadline can not be smaller than the timestamp of the current block
  if (deadline < (uint64_t)blockTime) return (PKT_ERROR_SP -204);

  memcpy(&early_bird, p++, 1);
  PrintToLog("\tEarly Bird Bonus: %u\n", early_bird);

  memcpy(&percentage, p++, 1);
  PrintToLog("\t      Percentage: %u\n", percentage);

  if (isOverrun(p, __LINE__)) return (PKT_ERROR_SP -765);

  return 0;
}

// -----------------------------------------------------------

/** Checks whether a pointer to the payload is past it's last position. */
bool CMPTransaction::isOverrun(const char *p, unsigned int line)
{
int now = (char *)p - (char *)&pkt;
bool bRet = (now > pkt_size);

    if (bRet) PrintToLog("%s(%sline=%u):now= %u, pkt_size= %u\n", __FUNCTION__, bRet ? "OVERRUN !!! ":"", line, now, pkt_size);

    return bRet;
}

char *mastercore::c_strMasterProtocolTXType(int i)
{
  switch (i)
  {
    case MSC_TYPE_SIMPLE_SEND: return ((char *)"Simple Send");
    case MSC_TYPE_RESTRICTED_SEND: return ((char *)"Restricted Send");
    case MSC_TYPE_SEND_TO_OWNERS: return ((char *)"Send To Owners");
    case MSC_TYPE_SAVINGS_MARK: return ((char *)"Savings");
    case MSC_TYPE_SAVINGS_COMPROMISED: return ((char *)"Savings COMPROMISED");
    case MSC_TYPE_RATELIMITED_MARK: return ((char *)"Rate-Limiting");
    case MSC_TYPE_AUTOMATIC_DISPENSARY: return ((char *)"Automatic Dispensary");
    case MSC_TYPE_TRADE_OFFER: return ((char *)"DEx Sell Offer");
    case MSC_TYPE_METADEX_TRADE: return ((char *)"MetaDEx trade");
    case MSC_TYPE_METADEX_CANCEL_PRICE: return ((char *)"MetaDEx cancel-price");
    case MSC_TYPE_METADEX_CANCEL_PAIR: return ((char *)"MetaDEx cancel-pair");
    case MSC_TYPE_METADEX_CANCEL_ECOSYSTEM: return ((char *)"MetaDEx cancel-ecosystem");
    case MSC_TYPE_ACCEPT_OFFER_BTC: return ((char *)"DEx Accept Offer");
    case MSC_TYPE_CREATE_PROPERTY_FIXED: return ((char *)"Create Property - Fixed");
    case MSC_TYPE_CREATE_PROPERTY_VARIABLE: return ((char *)"Create Property - Variable");
    case MSC_TYPE_PROMOTE_PROPERTY: return ((char *)"Promote Property");
    case MSC_TYPE_CLOSE_CROWDSALE: return ((char *)"Close Crowdsale");
    case MSC_TYPE_CREATE_PROPERTY_MANUAL: return ((char *)"Create Property - Manual");
    case MSC_TYPE_GRANT_PROPERTY_TOKENS: return ((char *)"Grant Property Tokens");
    case MSC_TYPE_REVOKE_PROPERTY_TOKENS: return ((char *)"Revoke Property Tokens");
    case MSC_TYPE_CHANGE_ISSUER_ADDRESS: return ((char *)"Change Issuer Address");
    case MSC_TYPE_NOTIFICATION: return ((char *)"Notification");
    case OMNICORE_MESSAGE_TYPE_ALERT: return ((char *)"ALERT");

    default: return ((char *)"* unknown type *");
  }
}

// -------------------- PACKET PARSING -----------------------

/** Parses the packet or payload. */
bool CMPTransaction::interpret_Transaction()
{
    if (!interpret_TransactionType()) {
        PrintToLog("Failed to interpret type and version\n");
        return false;
    }

    switch (type) {
        case MSC_TYPE_SIMPLE_SEND:
            return interpret_SimpleSend();

        case MSC_TYPE_SEND_TO_OWNERS:
            return interpret_SendToOwners();

        case MSC_TYPE_TRADE_OFFER:
            return interpret_TradeOffer();

        case MSC_TYPE_ACCEPT_OFFER_BTC:
            return interpret_AcceptOfferBTC();

        case MSC_TYPE_METADEX_TRADE:
            return interpret_MetaDExTrade();

        case MSC_TYPE_METADEX_CANCEL_PRICE:
            return interpret_MetaDExCancelPrice();

        case MSC_TYPE_METADEX_CANCEL_PAIR:
            return interpret_MetaDExCancelPair();

        case MSC_TYPE_METADEX_CANCEL_ECOSYSTEM:
            return interpret_MetaDExCancelEcosystem();

        case MSC_TYPE_CREATE_PROPERTY_FIXED:
            return interpret_CreatePropertyFixed();

        case MSC_TYPE_CREATE_PROPERTY_VARIABLE:
            return interpret_CreatePropertyVariable();

        case MSC_TYPE_CLOSE_CROWDSALE:
            return interpret_CloseCrowdsale();

        case MSC_TYPE_CREATE_PROPERTY_MANUAL:
            return interpret_CreatePropertyMananged();

        case MSC_TYPE_GRANT_PROPERTY_TOKENS:
            return interpret_GrantTokens();

        case MSC_TYPE_REVOKE_PROPERTY_TOKENS:
            return interpret_RevokeTokens();

        case MSC_TYPE_CHANGE_ISSUER_ADDRESS:
            return interpret_ChangeIssuer();

        case OMNICORE_MESSAGE_TYPE_ALERT:
            return interpret_Alert();
    }

    return false;
}

/** Helper to convert class number to string. */
static std::string intToClass(int multi)
{
    switch (multi) {
        case 1:
            return "B";
        case 2:
            return "C";
    }
    return "A";
}

/** Version and type */
bool CMPTransaction::interpret_TransactionType()
{
    if (pkt_size < 4) {
        return false;
    }
    uint16_t txVersion = 0;
    uint16_t txType = 0;
    memcpy(&txVersion, &pkt[0], 2);
    swapByteOrder16(txVersion);
    memcpy(&txType, &pkt[2], 2);
    swapByteOrder16(txType);
    version = txVersion;
    type = txType;

    if (msc_debug_packets) {
        PrintToLog("\t------------------------------\n");
        PrintToLog("\t         version: %d, class %s\n", txVersion, intToClass(multi));
        PrintToLog("\t            type: %d (%s)\n", txType, c_strMasterProtocolTXType(txType));
    }

    return true;
}

/** Tx 1 */
bool CMPTransaction::interpret_SimpleSend()
{
    if (pkt_size < 16) {
        return false;
    }
    memcpy(&property, &pkt[4], 4);
    swapByteOrder32(property);
    memcpy(&nValue, &pkt[8], 8);
    swapByteOrder64(nValue);
    nNewValue = nValue;

    if (msc_debug_packets) {
        PrintToLog("\t        property: %d (%s)\n", property, strMPProperty(property));
        PrintToLog("\t           value: %s\n", FormatMP(property, nValue));
    }

    return true;
}

/** Tx 3 */
bool CMPTransaction::interpret_SendToOwners()
{
    if (pkt_size < 16) {
        return false;
    }
    memcpy(&property, &pkt[4], 4);
    swapByteOrder32(property);
    memcpy(&nValue, &pkt[8], 8);
    swapByteOrder64(nValue);
    nNewValue = nValue;

    if (msc_debug_packets) {
        PrintToLog("\t        property: %d (%s)\n", property, strMPProperty(property));
        PrintToLog("\t           value: %s\n", FormatMP(property, nValue));
    }

    return true;
}

/** Tx 20 */
bool CMPTransaction::interpret_TradeOffer()
{
    if (pkt_size < 34) {
        return false;
    }
    memcpy(&property, &pkt[4], 4);
    swapByteOrder32(property);
    memcpy(&nValue, &pkt[8], 8);
    swapByteOrder64(nValue);
    nNewValue = nValue;
    memcpy(&amount_desired, &pkt[16], 8);
    memcpy(&blocktimelimit, &pkt[24], 1);
    memcpy(&min_fee, &pkt[25], 8);
    memcpy(&subaction, &pkt[33], 1);
    swapByteOrder64(amount_desired);
    swapByteOrder64(min_fee);

    if (msc_debug_packets) {
        PrintToLog("\t        property: %d (%s)\n", property, strMPProperty(property));
        PrintToLog("\t           value: %s\n", FormatMP(property, nValue));
        PrintToLog("\t  amount desired: %s\n", FormatDivisibleMP(amount_desired));
        PrintToLog("\tblock time limit: %d\n", blocktimelimit);
        PrintToLog("\t         min fee: %s\n", FormatDivisibleMP(min_fee));
        PrintToLog("\t      sub-action: %d\n", subaction);
    }

    return true;
}

/** Tx 22 */
bool CMPTransaction::interpret_AcceptOfferBTC()
{
    if (pkt_size < 16) {
        return false;
    }
    memcpy(&property, &pkt[4], 4);
    swapByteOrder32(property);
    memcpy(&nValue, &pkt[8], 8);
    swapByteOrder64(nValue);
    nNewValue = nValue;

    if (msc_debug_packets) {
        PrintToLog("\t        property: %d (%s)\n", property, strMPProperty(property));
        PrintToLog("\t           value: %s\n", FormatMP(property, nValue));
    }

    return true;
}

/** Tx 25 */
bool CMPTransaction::interpret_MetaDExTrade()
{
    if (pkt_size < 28) {
        return false;
    }
    memcpy(&property, &pkt[4], 4);
    swapByteOrder32(property);
    memcpy(&nValue, &pkt[8], 8);
    swapByteOrder64(nValue);
    nNewValue = nValue;
    memcpy(&desired_property, &pkt[16], 4);
    swapByteOrder32(desired_property);
    memcpy(&desired_value, &pkt[20], 8);
    swapByteOrder64(desired_value);

    action = CMPTransaction::ADD; // depreciated

    if (msc_debug_packets) {
        PrintToLog("\t        property: %d (%s)\n", property, strMPProperty(property));
        PrintToLog("\t           value: %s\n", FormatMP(property, nValue));
        PrintToLog("\tdesired property: %d (%s)\n", desired_property, strMPProperty(desired_property));
        PrintToLog("\t   desired value: %s\n", FormatMP(desired_property, desired_value));
    }

    return true;
}

/** Tx 26 */
bool CMPTransaction::interpret_MetaDExCancelPrice()
{
    if (pkt_size < 28) {
        return false;
    }
    memcpy(&property, &pkt[4], 4);
    swapByteOrder32(property);
    memcpy(&nValue, &pkt[8], 8);
    swapByteOrder64(nValue);
    nNewValue = nValue;
    memcpy(&desired_property, &pkt[16], 4);
    swapByteOrder32(desired_property);
    memcpy(&desired_value, &pkt[20], 8);
    swapByteOrder64(desired_value);

    action = CMPTransaction::CANCEL_AT_PRICE; // depreciated

    if (msc_debug_packets) {
        PrintToLog("\t        property: %d (%s)\n", property, strMPProperty(property));
        PrintToLog("\t           value: %s\n", FormatMP(property, nValue));
        PrintToLog("\tdesired property: %d (%s)\n", desired_property, strMPProperty(desired_property));
        PrintToLog("\t   desired value: %s\n", FormatMP(desired_property, desired_value));
    }

    return true;
}

/** Tx 27 */
bool CMPTransaction::interpret_MetaDExCancelPair()
{
    if (pkt_size < 12) {
        return false;
    }
    memcpy(&property, &pkt[4], 4);
    swapByteOrder32(property);
    memcpy(&desired_property, &pkt[8], 4);
    swapByteOrder32(desired_property);

    nValue = 0; // depreciated
    nNewValue = nValue; // depreciated
    desired_value = 0; // depreciated
    action = CMPTransaction::CANCEL_ALL_FOR_PAIR; // depreciated

    if (msc_debug_packets) {
        PrintToLog("\t        property: %d (%s)\n", property, strMPProperty(property));
        PrintToLog("\tdesired property: %d (%s)\n", desired_property, strMPProperty(desired_property));
    }

    return true;
}

/** Tx 28 */
bool CMPTransaction::interpret_MetaDExCancelEcosystem()
{
    if (pkt_size < 5) {
        return false;
    }
    memcpy(&ecosystem, &pkt[4], 1);

    property = ecosystem; // depreciated
    desired_property = ecosystem; // depreciated
    nValue = 0; // depreciated
    nNewValue = nValue; // depreciated
    desired_value = 0; // depreciated
    action = CMPTransaction::CANCEL_EVERYTHING; // depreciated

    if (msc_debug_packets) {
        PrintToLog("\t       ecosystem: %d\n", (int)ecosystem);
    }

    return true;
}

/** Tx 50 */
bool CMPTransaction::interpret_CreatePropertyFixed()
{
    if (pkt_size < 25) {
        return false;
    }
    const char* p = 11 + (char*) &pkt;
    std::vector<std::string> spstr;
    memcpy(&ecosystem, &pkt[4], 1);
    memcpy(&prop_type, &pkt[5], 2);
    swapByteOrder16(prop_type);
    memcpy(&prev_prop_id, &pkt[7], 4);
    swapByteOrder32(prev_prop_id);
    for (int i = 0; i < 5; i++) {
        spstr.push_back(std::string(p));
        p += spstr.back().size() + 1;
    }
    int i = 0;
    memcpy(category, spstr[i].c_str(), std::min(spstr[i].length(), sizeof(category)-1)); i++;
    memcpy(subcategory, spstr[i].c_str(), std::min(spstr[i].length(), sizeof(subcategory)-1)); i++;
    memcpy(name, spstr[i].c_str(), std::min(spstr[i].length(), sizeof(name)-1)); i++;
    memcpy(url, spstr[i].c_str(), std::min(spstr[i].length(), sizeof(url)-1)); i++;
    memcpy(data, spstr[i].c_str(), std::min(spstr[i].length(), sizeof(data)-1)); i++;
    memcpy(&nValue, p, 8);
    swapByteOrder64(nValue);
    p += 8;
    nNewValue = nValue;

    if (msc_debug_packets) {
        PrintToLog("\t       ecosystem: %d\n", ecosystem);
        PrintToLog("\t   property type: %d (%s)\n", prop_type, c_strPropertyType(prop_type));
        PrintToLog("\tprev property id: %d\n", prev_prop_id);
        PrintToLog("\t        category: %s\n", category);
        PrintToLog("\t     subcategory: %s\n", subcategory);
        PrintToLog("\t            name: %s\n", name);
        PrintToLog("\t             url: %s\n", url);
        PrintToLog("\t            data: %s\n", data);
        PrintToLog("\t           value: %s\n", prop_type == 1 ? FormatIndivisibleMP(nValue) : FormatDivisibleMP(nValue));
    }

    if (isOverrun(p, __LINE__)) {
        return false;
    }

    return true;
}

/** Tx 51 */
bool CMPTransaction::interpret_CreatePropertyVariable()
{
    if (pkt_size < 39) {
        return false;
    }
    const char* p = 11 + (char*) &pkt;
    std::vector<std::string> spstr;
    memcpy(&ecosystem, &pkt[4], 1);
    memcpy(&prop_type, &pkt[5], 2);
    swapByteOrder16(prop_type);
    memcpy(&prev_prop_id, &pkt[7], 4);
    swapByteOrder32(prev_prop_id);
    for (int i = 0; i < 5; i++) {
        spstr.push_back(std::string(p));
        p += spstr.back().size() + 1;
    }
    int i = 0;
    memcpy(category, spstr[i].c_str(), std::min(spstr[i].length(), sizeof(category)-1)); i++;
    memcpy(subcategory, spstr[i].c_str(), std::min(spstr[i].length(), sizeof(subcategory)-1)); i++;
    memcpy(name, spstr[i].c_str(), std::min(spstr[i].length(), sizeof(name)-1)); i++;
    memcpy(url, spstr[i].c_str(), std::min(spstr[i].length(), sizeof(url)-1)); i++;
    memcpy(data, spstr[i].c_str(), std::min(spstr[i].length(), sizeof(data)-1)); i++;
    memcpy(&property, p, 4);
    swapByteOrder32(property);
    p += 4;
    memcpy(&nValue, p, 8);
    swapByteOrder64(nValue);
    p += 8;
    nNewValue = nValue;
    memcpy(&deadline, p, 8);
    swapByteOrder64(deadline);
    p += 8;
    memcpy(&early_bird, p++, 1);
    memcpy(&percentage, p++, 1);

    if (msc_debug_packets) {
        PrintToLog("\t       ecosystem: %d\n", ecosystem);
        PrintToLog("\t   property type: %d (%s)\n", prop_type, c_strPropertyType(prop_type));
        PrintToLog("\tprev property id: %d\n", prev_prop_id);
        PrintToLog("\t        category: %s\n", category);
        PrintToLog("\t     subcategory: %s\n", subcategory);
        PrintToLog("\t            name: %s\n", name);
        PrintToLog("\t             url: %s\n", url);
        PrintToLog("\t            data: %s\n", data);
        PrintToLog("\tproperty desired: %d (%s)\n", property, strMPProperty(property));
        PrintToLog("\t tokens per unit: %s\n", prop_type == 1 ? FormatIndivisibleMP(nValue) : FormatDivisibleMP(nValue));
        PrintToLog("\t        deadline: %s (%x)\n", DateTimeStrFormat("%Y-%m-%d %H:%M:%S", deadline), deadline);
        PrintToLog("\tearly bird bonus: %d\n", early_bird);
        PrintToLog("\t    issuer bonus: %d\n", percentage);
    }

    if (isOverrun(p, __LINE__)) {
        return false;
    }

    return true;
}

/** Tx 53 */
bool CMPTransaction::interpret_CloseCrowdsale()
{
    if (pkt_size < 8) {
        return false;
    }
    memcpy(&property, &pkt[4], 4);
    swapByteOrder32(property);

    if (msc_debug_packets) {
        PrintToLog("\t        property: %d (%s)\n", property, strMPProperty(property));
    }

    return true;
}

/** Tx 54 */
bool CMPTransaction::interpret_CreatePropertyMananged()
{
    if (pkt_size < 17) {
        return false;
    }
    const char* p = 11 + (char*) &pkt;
    std::vector<std::string> spstr;
    memcpy(&ecosystem, &pkt[4], 1);
    memcpy(&prop_type, &pkt[5], 2);
    swapByteOrder16(prop_type);
    memcpy(&prev_prop_id, &pkt[7], 4);
    swapByteOrder32(prev_prop_id);
    for (int i = 0; i < 5; i++) {
        spstr.push_back(std::string(p));
        p += spstr.back().size() + 1;
    }
    int i = 0;
    memcpy(category, spstr[i].c_str(), std::min(spstr[i].length(), sizeof(category)-1)); i++;
    memcpy(subcategory, spstr[i].c_str(), std::min(spstr[i].length(), sizeof(subcategory)-1)); i++;
    memcpy(name, spstr[i].c_str(), std::min(spstr[i].length(), sizeof(name)-1)); i++;
    memcpy(url, spstr[i].c_str(), std::min(spstr[i].length(), sizeof(url)-1)); i++;
    memcpy(data, spstr[i].c_str(), std::min(spstr[i].length(), sizeof(data)-1)); i++;

    if (msc_debug_packets) {
        PrintToLog("\t       ecosystem: %d\n", ecosystem);
        PrintToLog("\t   property type: %d (%s)\n", prop_type, c_strPropertyType(prop_type));
        PrintToLog("\tprev property id: %d\n", prev_prop_id);
        PrintToLog("\t        category: %s\n", category);
        PrintToLog("\t     subcategory: %s\n", subcategory);
        PrintToLog("\t            name: %s\n", name);
        PrintToLog("\t             url: %s\n", url);
        PrintToLog("\t            data: %s\n", data);
    }

    if (isOverrun(p, __LINE__)) {
        return false;
    }

    return true;
}

/** Tx 55 */
bool CMPTransaction::interpret_GrantTokens()
{
    if (pkt_size < 16) {
        return false;
    }
    memcpy(&property, &pkt[4], 4);
    swapByteOrder32(property);
    memcpy(&nValue, &pkt[8], 8);
    swapByteOrder64(nValue);
    nNewValue = nValue;

    if (msc_debug_packets) {
        PrintToLog("\t        property: %d (%s)\n", property, strMPProperty(property));
        PrintToLog("\t           value: %s\n", FormatMP(property, nValue));
    }

    return true;
}

/** Tx 56 */
bool CMPTransaction::interpret_RevokeTokens()
{
    if (pkt_size < 16) {
        return false;
    }
    memcpy(&property, &pkt[4], 4);
    swapByteOrder32(property);
    memcpy(&nValue, &pkt[8], 8);
    swapByteOrder64(nValue);
    nNewValue = nValue;

    if (msc_debug_packets) {
        PrintToLog("\t        property: %d (%s)\n", property, strMPProperty(property));
        PrintToLog("\t           value: %s\n", FormatMP(property, nValue));
    }

    return true;
}

/** Tx 70 */
bool CMPTransaction::interpret_ChangeIssuer()
{
    if (pkt_size < 8) {
        return false;
    }
    memcpy(&property, &pkt[4], 4);
    swapByteOrder32(property);

    if (msc_debug_packets) {
        PrintToLog("\t        property: %d (%s)\n", property, strMPProperty(property));
    }

    return true;
}

/** Tx 65535 */
bool CMPTransaction::interpret_Alert()
{
    if (pkt_size < 5) {
        return false;
    }
    const char* p = 4 + (char*) &pkt;
    std::string spstr(p);
    memcpy(alertString, spstr.c_str(), std::min(spstr.length(), sizeof(alertString)-1));

    if (msc_debug_packets) {
        PrintToLog("\t           alert: %s\n", alertString);
    }

    if (isOverrun(p, __LINE__)) {
        return false;
    }

    return true;
}

// ---------------------- CORE LOGIC -------------------------

/** Tx 3 */
int CMPTransaction::logicMath_SendToOwners()
{
    if (!IsTransactionTypeAllowed(block, property, type, version)) {
        return (PKT_ERROR_STO -22);
    }

    if (nValue <= 0 || MAX_INT_8_BYTES < nValue) {
        return (PKT_ERROR_STO -23);
    }

    if (!_my_sps->hasSP(property)) {
        return (PKT_ERROR_STO -24);
    }

    if (getMPbalance(sender, property, BALANCE) < (int64_t)nValue) {
        return (PKT_ERROR_STO -25);
    }

    // ------------------------------------------

    OwnerAddrType receiversSet = STO_GetReceivers(sender, property, nValue);
    uint64_t numberOfReceivers = receiversSet.size();

    // make sure we found some owners
    if (numberOfReceivers <= 0) {
        return (PKT_ERROR_STO -26);
    }

    // determine which property the fee will be paid in
    uint32_t feeProperty = isTestEcosystemProperty(property) ? OMNI_PROPERTY_TMSC : OMNI_PROPERTY_MSC;

    int64_t transferFee = TRANSFER_FEE_PER_OWNER * numberOfReceivers;
    PrintToLog("\t    Transfer fee: %s %s\n", FormatDivisibleMP(transferFee), strMPProperty(feeProperty));

    // enough coins to pay the fee?
    if (feeProperty != property) {
        if (getMPbalance(sender, feeProperty, BALANCE) < transferFee) {
            return (PKT_ERROR_STO -27);
        }
    } else {
        // special case check, only if distributing MSC or TMSC -- the property the fee will be paid in
        if (getMPbalance(sender, feeProperty, BALANCE) < ((int64_t)nValue + transferFee)) {
            return (PKT_ERROR_STO -28);
        }
    }

    // ------------------------------------------

    // burn MSC or TMSC here: take the transfer fee away from the sender
    assert(update_tally_map(sender, feeProperty, -transferFee, BALANCE));

    // split up what was taken and distribute between all holders
    int64_t sent_so_far = 0;
    for (OwnerAddrType::reverse_iterator it = receiversSet.rbegin(); it != receiversSet.rend(); ++it) {
        const std::string& address = it->second;

        int64_t will_really_receive = it->first;
        sent_so_far += will_really_receive;

        // real execution of the loop
        assert(update_tally_map(sender, property, -will_really_receive, BALANCE));
        assert(update_tally_map(address, property, will_really_receive, BALANCE));

        // add to stodb
        s_stolistdb->recordSTOReceive(address, txid, block, property, will_really_receive);

        if (sent_so_far != (int64_t)nValue) {
            PrintToLog("sent_so_far= %14d, nValue= %14d, n_owners= %d\n", sent_so_far, nValue, numberOfReceivers);
        } else {
            PrintToLog("SendToOwners: DONE HERE\n");
        }
    }

    // sent_so_far must equal nValue here
    assert(sent_so_far == (int64_t)nValue);

    return 0;
}

/** Tx 20 */
int CMPTransaction::logicMath_TradeOffer(CMPOffer *obj_o)
{
    if (MAX_INT_8_BYTES < nValue) {
        return (PKT_ERROR -801);  // out of range
    }
    // ------------------------------------------

int rc = PKT_ERROR_TRADEOFFER;

      if ((OMNI_PROPERTY_TMSC != property) && (OMNI_PROPERTY_MSC != property))
      {
        PrintToLog("No smart properties allowed on the DeX...\n");
        return PKT_ERROR_TRADEOFFER -72;
      }

      // block height checks, for instance DEX is only available on MSC starting with block 290630
      if (!IsTransactionTypeAllowed(block, property, type, version)) return -88888;

      if (obj_o)
      {
        obj_o->Set(amount_desired, min_fee, blocktimelimit, subaction);
        return PKT_RETURNED_OBJECT;
      }

      // figure out which Action this is based on amount for sale, version & etc.
      switch (version)
      {
        case MP_TX_PKT_V0:
          if (0 != nValue)
          {
            if (!DEx_offerExists(sender, property))
            {
              rc = DEx_offerCreate(sender, property, nValue, block, amount_desired, min_fee, blocktimelimit, txid, &nNewValue);
            }
            else
            {
              rc = DEx_offerUpdate(sender, property, nValue, block, amount_desired, min_fee, blocktimelimit, txid, &nNewValue);
            }
          }
          else
          // what happens if nValue is 0 for V0 ?  ANSWER: check if exists and it does -- cancel, otherwise invalid
          {
            if (DEx_offerExists(sender, property))
            {
              rc = DEx_offerDestroy(sender, property);
            }
          }

          break;

        case MP_TX_PKT_V1:
        {
          if (DEx_offerExists(sender, property))
          {
            if ((CANCEL != subaction) && (UPDATE != subaction))
            {
              PrintToLog("%s() INVALID SELL OFFER -- ONE ALREADY EXISTS\n", __FUNCTION__);
              rc = PKT_ERROR_TRADEOFFER -11;
              break;
            }
          }
          else
          {
            // Offer does not exist
            if ((NEW != subaction))
            {
              PrintToLog("%s() INVALID SELL OFFER -- UPDATE OR CANCEL ACTION WHEN NONE IS POSSIBLE\n", __FUNCTION__);
              rc = PKT_ERROR_TRADEOFFER -12;
              break;
            }
          }
 
          switch (subaction)
          {
            case NEW:
              rc = DEx_offerCreate(sender, property, nValue, block, amount_desired, min_fee, blocktimelimit, txid, &nNewValue);
              break;

            case UPDATE:
              rc = DEx_offerUpdate(sender, property, nValue, block, amount_desired, min_fee, blocktimelimit, txid, &nNewValue);
              break;

            case CANCEL:
              rc = DEx_offerDestroy(sender, property);
              break;

            default:
              rc = (PKT_ERROR -999);
              break;
          }
          break;
        }

        default:
          rc = (PKT_ERROR -500);  // neither V0 nor V1
          break;
      };

    return rc;
}

/** Tx 22 */
int CMPTransaction::logicMath_AcceptOffer_BTC()
{
    if (MAX_INT_8_BYTES < nValue) {
        return (PKT_ERROR -801);  // out of range
    }
    // ------------------------------------------

    int rc = DEX_ERROR_ACCEPT;

    // the min fee spec requirement is checked in the following function
    rc = DEx_acceptCreate(sender, receiver, property, nValue, block, tx_fee_paid, &nNewValue);

    return rc;
}

// TODO: depreciate
int CMPTransaction::logicMath_MetaDEx(CMPMetaDEx* mdex_o)
{
    if (MAX_INT_8_BYTES < nValue) {
        return (PKT_ERROR -801); // out of range
    }
    // ------------------------------------------

    int rc = PKT_ERROR_METADEX -100;

    if (mdex_o) {
        mdex_o->Set(sender, block, property, nValue, desired_property, desired_value, txid, tx_idx, action);
        return PKT_RETURNED_OBJECT;
    }

    return rc;
}

/** Tx 25 */
int CMPTransaction::logicMath_MetaDExTrade()
{
    if (MAX_INT_8_BYTES < nValue) {
        return (PKT_ERROR - 801); // out of range
    }
    // ------------------------------------------

    int rc = PKT_ERROR_METADEX - 100;

    if (!IsTransactionTypeAllowed(block, property, type, version)) return (PKT_ERROR_METADEX -889);

    // ensure we are not trading same property for itself
    if (property == desired_property) return (PKT_ERROR_METADEX -5);

    // ensure no cross-over of properties from test to main ecosystem
    if (isTestEcosystemProperty(property) != isTestEcosystemProperty(desired_property)) return (PKT_ERROR_METADEX -4);

    // ensure the desired property exists in our universe
    if (!_my_sps->hasSP(desired_property)) return (PKT_ERROR_METADEX -30);

    // ensure offered and desired values are positive
    if (0 >= static_cast<int64_t>(nNewValue)) return (PKT_ERROR_METADEX -11);
    if (0 >= static_cast<int64_t>(desired_value)) return (PKT_ERROR_METADEX -12);

    // ensure that one side of the trade is MSC/TMSC (phase 1 check)
    if ((property != OMNI_PROPERTY_MSC) && (desired_property != OMNI_PROPERTY_MSC) &&
        (property != OMNI_PROPERTY_TMSC) && (desired_property != OMNI_PROPERTY_TMSC)) return (PKT_ERROR_METADEX -800);

    // ensure sufficient balance is available to offer
    if (getMPbalance(sender, property, BALANCE) < static_cast<int64_t>(nNewValue)) return (PKT_ERROR_METADEX -567);

    t_tradelistdb->recordNewTrade(txid, sender, property, desired_property, block, tx_idx);
    rc = MetaDEx_ADD(sender, property, nNewValue, block, desired_property, desired_value, txid, tx_idx);
    return rc;
}

/** Tx 26 */
int CMPTransaction::logicMath_MetaDExCancelPrice()
{
    if (MAX_INT_8_BYTES < nValue) {
        return (PKT_ERROR - 801); // out of range
    }
    // ------------------------------------------

    int rc = PKT_ERROR_METADEX -100;

    if (!IsTransactionTypeAllowed(block, property, type, version)) return (PKT_ERROR_METADEX -890);

    // ensure we are not trading same property for itself
    if (property == desired_property) return (PKT_ERROR_METADEX -5);

    // ensure no cross-over of properties from test to main ecosystem
    if (isTestEcosystemProperty(property) != isTestEcosystemProperty(desired_property)) return (PKT_ERROR_METADEX -4);

    // ensure the desired property exists in our universe
    if (!_my_sps->hasSP(desired_property)) return (PKT_ERROR_METADEX -30);

    // ensure offered and desired values are positive
    if (0 >= static_cast<int64_t> (nNewValue)) return (PKT_ERROR_METADEX -11);
    if (0 >= static_cast<int64_t> (desired_value)) return (PKT_ERROR_METADEX -12);

    rc = MetaDEx_CANCEL_AT_PRICE(txid, block, sender, property, nNewValue, desired_property, desired_value);

    return rc;
}

/** Tx 27 */
int CMPTransaction::logicMath_MetaDExCancelPair()
{
    int rc = PKT_ERROR_METADEX -100;

    if (!IsTransactionTypeAllowed(block, property, type, version)) return (PKT_ERROR_METADEX -891);

    // ensure we are not trading same property for itself
    if (property == desired_property) return (PKT_ERROR_METADEX - 5);

    // ensure no cross-over of properties from test to main ecosystem
    if (isTestEcosystemProperty(property) != isTestEcosystemProperty(desired_property)) return (PKT_ERROR_METADEX -4);

    // ensure the desired property exists in our universe
    if (!_my_sps->hasSP(desired_property)) return (PKT_ERROR_METADEX -30);

    rc = MetaDEx_CANCEL_ALL_FOR_PAIR(txid, block, sender, property, desired_property);

    return rc;
}

/** Tx 28 */
int CMPTransaction::logicMath_MetaDExCancelEcosystem()
{
    int rc = PKT_ERROR_METADEX -101;

    if (!IsTransactionTypeAllowed(block, ecosystem, type, version, true)) return (PKT_ERROR_METADEX -892);

    rc = MetaDEx_CANCEL_EVERYTHING(txid, block, sender, ecosystem);

    return rc;
}

/** Tx 50 */
int CMPTransaction::logicMath_CreatePropertyFixed()
{
    if (OMNI_PROPERTY_MSC != ecosystem && OMNI_PROPERTY_TMSC != ecosystem) {
        return (PKT_ERROR_SP -501);
    }
    if (MSC_PROPERTY_TYPE_INDIVISIBLE != prop_type && MSC_PROPERTY_TYPE_DIVISIBLE != prop_type) {
        return (PKT_ERROR_SP -502);
    }
    uint32_t prop_id = _my_sps->peekNextSPID(ecosystem);
    if (!IsTransactionTypeAllowed(block, prop_id, type, version)) {
        return (PKT_ERROR_SP -503);
    }
    if ('\0' == name[0]) {
        return (PKT_ERROR_SP -505);
    }
    // ------------------------------------------
    if (0 == nValue) {
        return (PKT_ERROR_SP -101);
    }
    if (MAX_INT_8_BYTES < nValue) {
        return (PKT_ERROR -802); // out of range
    }
    // ------------------------------------------

    int rc = -1;

    CMPSPInfo::Entry newSP;
    newSP.issuer = sender;
    newSP.txid = txid;
    newSP.prop_type = prop_type;
    newSP.num_tokens = nValue;
    newSP.category.assign(category);
    newSP.subcategory.assign(subcategory);
    newSP.name.assign(name);
    newSP.url.assign(url);
    newSP.data.assign(data);
    newSP.fixed = true;
    newSP.creation_block = newSP.update_block = chainActive[block]->GetBlockHash();

    const uint32_t id = _my_sps->putSP(ecosystem, newSP);
    update_tally_map(sender, id, nValue, BALANCE);

    rc = 0;
    return rc;
}

/** Tx 51 */
int CMPTransaction::logicMath_CreatePropertyVariable()
{
    if (OMNI_PROPERTY_MSC != ecosystem && OMNI_PROPERTY_TMSC != ecosystem) {
        return (PKT_ERROR_SP -501);
    }
    if (MSC_PROPERTY_TYPE_INDIVISIBLE != prop_type && MSC_PROPERTY_TYPE_DIVISIBLE != prop_type) {
        return (PKT_ERROR_SP -502);
    }
    uint32_t prop_id = _my_sps->peekNextSPID(ecosystem);
    if (!IsTransactionTypeAllowed(block, prop_id, type, version)) {
        return (PKT_ERROR_SP -503);
    }
    if ('\0' == name[0]) {
        return (PKT_ERROR_SP -505);
    }
    // ------------------------------------------
    if (0 == nValue) {
        return (PKT_ERROR_SP - 201);
    }
    if (MAX_INT_8_BYTES < nValue) {
        return (PKT_ERROR - 803); // out of range
    }
    if (!deadline) return (PKT_ERROR_SP - 203); // deadline cannot be 0
    // deadline can not be smaller than the timestamp of the current block
    if ((int64_t) deadline < blockTime) return (PKT_ERROR_SP - 204);
    // ------------------------------------------

    int rc = -1;

    // check if one exists for this address already !
    if (NULL != getCrowd(sender)) return (PKT_ERROR_SP -20);

    // must check that the desired property exists in our universe
    if (false == _my_sps->hasSP(property)) return (PKT_ERROR_SP -30);

    CMPSPInfo::Entry newSP;
    newSP.issuer = sender;
    newSP.txid = txid;
    newSP.prop_type = prop_type;
    newSP.num_tokens = nValue;
    newSP.category.assign(category);
    newSP.subcategory.assign(subcategory);
    newSP.name.assign(name);
    newSP.url.assign(url);
    newSP.data.assign(data);
    newSP.fixed = false;
    newSP.property_desired = property;
    newSP.deadline = deadline;
    newSP.early_bird = early_bird;
    newSP.percentage = percentage;
    newSP.creation_block = newSP.update_block = chainActive[block]->GetBlockHash();

    const uint32_t id = _my_sps->putSP(ecosystem, newSP);
    my_crowds.insert(std::make_pair(sender, CMPCrowd(id, nValue, property, deadline, early_bird, percentage, 0, 0)));
    PrintToLog("CREATED CROWDSALE id: %u value: %lu property: %u\n", id, nValue, property);

    rc = 0;
    return rc;
}

/** Tx 53 */
int CMPTransaction::logicMath_CloseCrowdsale()
{
    int rc = -1;

    CrowdMap::iterator it = my_crowds.find(sender);

    if (it == my_crowds.end()) {
        return (PKT_ERROR_SP -605);
    }

    if (msc_debug_sp) PrintToLog("%s() trying to ERASE CROWDSALE for propid= %u=%X\n", __FUNCTION__, property, property);

    // ensure we are closing the crowdsale which we opened by checking the property
    if ((it->second).getPropertyId() != property) {
        return (PKT_ERROR_SP -606);
    }

    dumpCrowdsaleInfo(it->first, it->second);

    // Begin calculate Fractional
    CMPCrowd &crowd = it->second;

    CMPSPInfo::Entry sp;
    _my_sps->getSP(crowd.getPropertyId(), sp);

    double missedTokens = calculateFractional(sp.prop_type,
            sp.early_bird,
            sp.deadline,
            sp.num_tokens,
            sp.percentage,
            crowd.getDatabase(),
            crowd.getIssuerCreated());

    sp.historicalData = crowd.getDatabase();
    sp.update_block = chainActive[block]->GetBlockHash();
    sp.close_early = 1;
    sp.timeclosed = blockTime;
    sp.txid_close = txid;
    sp.missedTokens = (int64_t) missedTokens;
    _my_sps->updateSP(crowd.getPropertyId(), sp);

    update_tally_map(sp.issuer, crowd.getPropertyId(), missedTokens, BALANCE);
    //End

    my_crowds.erase(it);

    rc = 0;
    return rc;
}

/** Tx 54 */
int CMPTransaction::logicMath_CreatePropertyMananged()
{
    if (OMNI_PROPERTY_MSC != ecosystem && OMNI_PROPERTY_TMSC != ecosystem) {
        return (PKT_ERROR_SP -501);
    }
    if (MSC_PROPERTY_TYPE_INDIVISIBLE != prop_type && MSC_PROPERTY_TYPE_DIVISIBLE != prop_type) {
        return (PKT_ERROR_SP -502);
    }
    uint32_t prop_id = _my_sps->peekNextSPID(ecosystem);
    if (!IsTransactionTypeAllowed(block, prop_id, type, version)) {
        return (PKT_ERROR_SP -503);
    }
    if ('\0' == name[0]) {
        return (PKT_ERROR_SP -505);
    }
    // ------------------------------------------

    int rc = -1;

    CMPSPInfo::Entry newSP;
    newSP.issuer = sender;
    newSP.txid = txid;
    newSP.prop_type = prop_type;
    newSP.category.assign(category);
    newSP.subcategory.assign(subcategory);
    newSP.name.assign(name);
    newSP.url.assign(url);
    newSP.data.assign(data);
    newSP.fixed = false;
    newSP.manual = true;

    uint32_t id = _my_sps->putSP(ecosystem, newSP);
    PrintToLog("CREATED MANUAL PROPERTY id: %u admin: %s \n", id, sender);

    rc = 0;
    return rc;
}

/** Tx 55 */
int CMPTransaction::logicMath_GrantTokens()
{
    if (MAX_INT_8_BYTES < nValue) {
        return (PKT_ERROR -801);  // out of range
    }
    // ------------------------------------------

    int rc = PKT_ERROR_TOKENS - 1000;

    if (!IsTransactionTypeAllowed(block, property, type, version)) {
      PrintToLog("\tRejecting Grant: Transaction type not yet allowed\n");
      return (PKT_ERROR_TOKENS - 22);
    }

    if (sender.empty()) {
      PrintToLog("\tRejecting Grant: Sender is empty\n");
      return (PKT_ERROR_TOKENS - 23);
    }

    // manual issuance check
    if (false == _my_sps->hasSP(property)) {
      PrintToLog("\tRejecting Grant: SP id:%u does not exist\n", property);
      return (PKT_ERROR_TOKENS - 24);
    }

    CMPSPInfo::Entry sp;
    _my_sps->getSP(property, sp);

    if (false == sp.manual) {
      PrintToLog("\tRejecting Grant: SP id:%u was not issued with a TX 54\n", property);
      return (PKT_ERROR_TOKENS - 25);
    }


    // issuer check
    if (false == boost::iequals(sender, sp.issuer)) {
      PrintToLog("\tRejecting Grant: %s is not the issuer of SP id: %u\n", sender, property);
      return (PKT_ERROR_TOKENS - 26);
    }

    // overflow tokens check
    if (MAX_INT_8_BYTES - sp.num_tokens < nValue) {
      std::string prettyTokens;
      if (sp.isDivisible()) {
        prettyTokens = FormatDivisibleMP(nValue);
      } else {
        prettyTokens = FormatIndivisibleMP(nValue);
      }
      PrintToLog("\tRejecting Grant: granting %s tokens on SP id:%u would overflow the maximum limit for tokens in a smart property\n", prettyTokens, property);
      return (PKT_ERROR_TOKENS - 27);
    }

    // grant the tokens
    update_tally_map(sender, property, nValue, BALANCE);

    // call the send logic
    rc = logicMath_SimpleSend();

    // record this grant
    std::vector<int64_t> dataPt;
    dataPt.push_back(nValue);
    dataPt.push_back(0);
    sp.historicalData.insert(std::make_pair(txid, dataPt));
    sp.update_block = chainActive[block]->GetBlockHash();
    _my_sps->updateSP(property, sp);

    return rc;
}

/** Tx 56 */
int CMPTransaction::logicMath_RevokeTokens()
{
    if (MAX_INT_8_BYTES < nValue) {
        return (PKT_ERROR -801);  // out of range
    }
    // ------------------------------------------

    int rc = PKT_ERROR_TOKENS - 1000;

    if (!IsTransactionTypeAllowed(block, property, type, version)) {
      PrintToLog("\tRejecting Revoke: Transaction type not yet allowed\n");
      return (PKT_ERROR_TOKENS - 22);
    }

    if (sender.empty()) {
      PrintToLog("\tRejecting Revoke: Sender is empty\n");
      return (PKT_ERROR_TOKENS - 23);
    }

    // manual issuance check
    if (false == _my_sps->hasSP(property)) {
      PrintToLog("\tRejecting Revoke: SP id:%d does not exist\n", property);
      return (PKT_ERROR_TOKENS - 24);
    }

    CMPSPInfo::Entry sp;
    _my_sps->getSP(property, sp);

    if (false == sp.manual) {
      PrintToLog("\tRejecting Revoke: SP id:%d was not issued with a TX 54\n", property);
      return (PKT_ERROR_TOKENS - 25);
    }

    // insufficient funds check and revoke
    if (false == update_tally_map(sender, property, -nValue, BALANCE)) {
      PrintToLog("\tRejecting Revoke: insufficient funds\n");
      return (PKT_ERROR_TOKENS - 111);
    }

    // record this revoke
    std::vector<int64_t> dataPt;
    dataPt.push_back(0);
    dataPt.push_back(nValue);
    sp.historicalData.insert(std::make_pair(txid, dataPt));
    sp.update_block = chainActive[block]->GetBlockHash();
    _my_sps->updateSP(property, sp);

    rc = 0;
    return rc;
}

/** Tx 70 */
int CMPTransaction::logicMath_ChangeIssuer()
{
  int rc = PKT_ERROR_TOKENS - 1000;

  if (!IsTransactionTypeAllowed(block, property, type, version)) {
    PrintToLog("\tRejecting Change of Issuer: Transaction type not yet allowed\n");
    return (PKT_ERROR_TOKENS - 22);
  }

  if (sender.empty()) {
    PrintToLog("\tRejecting Change of Issuer: Sender is empty\n");
    return (PKT_ERROR_TOKENS - 23);
  }

  if (receiver.empty()) {
    PrintToLog("\tRejecting Change of Issuer: Receiver is empty\n");
    return (PKT_ERROR_TOKENS - 23);
  }

  if (false == _my_sps->hasSP(property)) {
    PrintToLog("\tRejecting Change of Issuer: SP id:%d does not exist\n", property);
    return (PKT_ERROR_TOKENS - 24);
  }

  CMPSPInfo::Entry sp;
  _my_sps->getSP(property, sp);

  // issuer check
  if (false == boost::iequals(sender, sp.issuer)) {
    PrintToLog("\tRejecting Change of Issuer: %s is not the issuer of SP id:%d\n", sender, property);
    return (PKT_ERROR_TOKENS - 26);
  }

  // record this change of issuer
  sp.issuer = receiver;
  sp.update_block = chainActive[block]->GetBlockHash();
  _my_sps->updateSP(property, sp);

  rc = 0;
  return rc;
}

/** Tx 65535 */
int CMPTransaction::logicMath_Alert()
{
    // check the packet version is also FF
    if (version != 65535) {
        return PKT_ERROR;
    }

    // is sender authorized?
    bool authorized = CheckAlertAuthorization(sender);

    if (!authorized) {
        // not authorized, ignore alert
        PrintToLog("\t      alert auth: false\n");
        return (PKT_ERROR -912);
    }
    // authorized, decode and make sure there are 4 tokens, then replace global_alert_message
    std::vector<std::string> vstr;
    boost::split(vstr, alertString, boost::is_any_of(":"), token_compress_on);
    PrintToLog("\t      alert auth: true\n");
    PrintToLog("\t    alert sender: %s\n", sender);

    if (5 != vstr.size()) {
        // there are not 5 tokens in the alert, badly formed alert and must discard
        PrintToLog("\t    packet error: badly formed alert != 5 tokens\n");
        return (PKT_ERROR -911);
    }

    int32_t alertType;
    uint64_t expiryValue;
    uint32_t typeCheck;
    uint32_t verCheck;
    std::string alertMessage;
    try {
        alertType = boost::lexical_cast<int32_t>(vstr[0]);
        expiryValue = boost::lexical_cast<uint64_t>(vstr[1]);
        typeCheck = boost::lexical_cast<uint32_t>(vstr[2]);
        verCheck = boost::lexical_cast<uint32_t>(vstr[3]);
    } catch (const boost::bad_lexical_cast &e) {
        PrintToLog("DEBUG ALERT - error in converting values from global alert string\n");
        return (PKT_ERROR -910); //(something went wrong)
    }
    alertMessage = vstr[4];
    PrintToLog("\t    message type: %llu\n", alertType);
    PrintToLog("\t    expiry value: %llu\n", expiryValue);
    PrintToLog("\t      type check: %llu\n", typeCheck);
    PrintToLog("\t       ver check: %llu\n", verCheck);
    PrintToLog("\t   alert message: %s\n", alertMessage);

    // copy the alert string into the global_alert_message and return a 0 rc
    SetOmniCoreAlert(alertString);

    // we have a new alert, fire a notify event if needed
    CAlert::Notify(alertMessage, true);

    return 0;
}

int CMPTransaction::logicMath_SavingsMark()
{
int rc = -12345;

  return rc;
}

int CMPTransaction::logicMath_SavingsCompromised()
{
int rc = -23456;

  return rc;
}

