//
// first & so far only Master protocol source file
// WARNING: Work In Progress -- major refactoring will be occurring often
//
// I am adding comments to aid with navigation and overall understanding of the design.
// this is the 'core' portion of the node+wallet: mastercoind
// see 'qt' subdirectory for UI files
//
// for the Sprint -- search for: TODO, FIXME, consensus
//

#include "base58.h"
#include "rpcserver.h"
#include "init.h"
#include "net.h"
#include "netbase.h"
#include "util.h"
#include "wallet.h"
#include "walletdb.h"

#include <stdint.h>
#include <string.h>
#include <map>
#include <queue>

#include <fstream>

#include <boost/assign/list_of.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"

#include "mastercoin.h"

using namespace std;
using namespace boost;
using namespace boost::assign;
using namespace json_spirit;

const string exodus = "1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P";
// const string exodusHash = "946cb2e08075bcbaf157e47bcb67eb2b2339d242";

int msc_debug  = 0;
int msc_debug2 = 1;
int msc_debug3 = 0;
int msc_debug4 = 1;
int msc_debug5 = 0;
int msc_debug6 = 0;

// follow this variable through the code to see how/which Master Protocol transactions get invalidated
static int InvalidCount_per_spec = 0;  // consolidate error messages into a nice log, for now just keep a count
static int InsufficientFunds = 0;      // consolidate error messages

// disable TMSC handling for now, has more legacy corner cases
static int ignoreTMSC = 0;

// this is the internal format for the offer primary key (TODO: replace by a class method)
#define STR_ADDR_CURR_COMBO(x) ( x + "-" + strprintf("%d", curr))

char *c_strMastercoinCurrency(unsigned int i)
{
  // test user-token
  if (0x80000000 & i)
  {
    // TODO: extend it to general case currencies
    switch (0x7FFFFFFF & i)
    {
      case 0: return ((char *)"test user token 0");
      case 1: return ((char *)"test user token 1");
      case 2: return ((char *)"test user token 2");
      case 3: return ((char *)"test user token 3");
      case 4: return ((char *)"test user token 4");
      default: return ((char *)"* unknown test currency *");
    }
  }
  else
  switch (i)
  {
    case 0: return ((char *)"BTC");
    case MASTERCOIN_CURRENCY_MSC: return ((char *)"MSC");
    case MASTERCOIN_CURRENCY_TMSC: return ((char *)"TMSC");
    case MASTERCOIN_CURRENCY_SP1: return ((char *)"SP");  // first SP MaidSafe coin -- name will be pulled from the protocol of course, TODO
    default: return ((char *)"* unknown currency *");
  }
}

char *c_strMastercoinType(int i)
{
  switch (i)
  {
    case MSC_TYPE_SIMPLE_SEND: return ((char *)"Simple Send");
    case 1: return ((char *)"Investment Send");
    case MSC_TYPE_TRADE_OFFER: return ((char *)"Selling coins for BTC, trade offer");
    case 21: return ((char *)"Offer/Accept one Master Protocol Coins for another");
    case MSC_TYPE_ACCEPT_OFFER_BTC: return ((char *)"Accepting offer, buying coins with BTC");
    case MSC_TYPE_CREATE_PROPERTY_FIXED: return ((char *)"Create Property - Fixed");
    case MSC_TYPE_CREATE_PROPERTY_VARIABLE: return ((char *)"Create Property - Variable");
    case MSC_TYPE_PROMOTE_PROPERTY: return ((char *)"Promote Property");
    default: return ((char *)"* unknown type *");
  }
}

void swapByteOrder16(unsigned short& us)
{
    us = (us >> 8) |
         (us << 8);
}

void swapByteOrder32(unsigned int& ui)
{
    ui = (ui >> 24) |
         ((ui<<8) & 0x00FF0000) |
         ((ui>>8) & 0x0000FF00) |
         (ui << 24);
}

void swapByteOrder64(uint64_t& ull)
{
    ull = (ull >> 56) |
          ((ull<<40) & 0x00FF000000000000) |
          ((ull<<24) & 0x0000FF0000000000) |
          ((ull<<8) & 0x000000FF00000000) |
          ((ull>>8) & 0x00000000FF000000) |
          ((ull>>24) & 0x0000000000FF0000) |
          ((ull>>40) & 0x000000000000FF00) |
          (ull << 56);
}

// a single outstanding offer -- from one seller of one currency, internally may have many accepts
class msc_offer
{
private:
  int offerBlock;
  string offerHash; // tx of the offer
  uint64_t offer_amount;
  uint64_t original_offer_amount;
  unsigned int currency;
  uint64_t BTC_desired; // amount desired, in BTC
  uint64_t min_fee;
  unsigned char blocktimelimit;

  // do a map of buyers, primary key is buyer+currency
  // MUST account for many possible accepts and EACH currency offer
  class msc_accept
  {
  private:
    uint64_t amount;    // amount of MSC/TMSC being purchased
    uint64_t fee_paid;  //

  public:
    int block;          // 'accept' message sent in this block

    msc_accept(uint64_t a, uint64_t f, int b):amount(a),fee_paid(f),block(b)
    {
      printf("%s(), line %d, file: %s\n", __FUNCTION__, __LINE__, __FILE__);
    }

    void print()
    {
      // hm, can't access the outer class' map member to get the currency unit label... do we care?
      printf("buying: %12.8lf units in block# %d, fee: %2.8lf\n", (double)amount/(double)COIN, block, (double)fee_paid/(double)COIN);
    }
  };

  map<string, msc_accept> my_accepts;

public:
  unsigned int getCurrency() const { return currency; }
  uint64_t getOfferAmount() const { return offer_amount; }
  void reduceOfferAmount(uint64_t purchased) { offer_amount -= purchased; } // TODO: check for negatives ? assert ?

  // this is used during payment, given the amount of BTC paid this function returns the amount of currency transacted
  uint64_t getCurrencyAmount(uint64_t BTC_paid) const
  {
  double X;
  double P;
  uint64_t purchased;

    if (BTC_paid >= BTC_desired) return (offer_amount);

    if (0==(double)BTC_desired) return 0;  // divide by 0 protection

    X = (double)BTC_paid/(double)BTC_desired;
    P = (double)original_offer_amount * X;

    purchased = rounduint64(P);

    return purchased;
  }

  msc_offer(int b, uint64_t a, unsigned int cu, uint64_t d, uint64_t fee, unsigned char btl)
   :offerBlock(b),offer_amount(a),currency(cu),BTC_desired(d),min_fee(fee),blocktimelimit(btl)
  {
    if (msc_debug4) printf("%s(%lu), line %d, file: %s\n", __FUNCTION__, a, __LINE__, __FILE__);

    original_offer_amount = a;

    my_accepts.clear();
  }

  void offer_update(int b, uint64_t a, unsigned int cu, uint64_t d, uint64_t fee, unsigned char btl)
  {
    printf("%s(%lu), line %d, file: %s\n", __FUNCTION__, a, __LINE__, __FILE__);

    offerBlock = b;
    offer_amount = a;
    original_offer_amount = a;
    currency = cu;
    BTC_desired = d;
    min_fee = fee;
    blocktimelimit = btl;
  }

  // the offer is accepted by a buyer, add this purchase to the accepted list or replace an old one from this buyer
  void offer_accept(string buyer, uint64_t desired, int block, uint64_t fee)
  {
    printf("%s(buyer:%s, desired=%2.8lf, block # %d), line %d, file: %s\n",
     __FUNCTION__, buyer.c_str(), (double)desired/(double)COIN, block, __LINE__, __FILE__);

    // here we ensure the correct BTC fee was paid in this acceptance message, per spec
    if (fee < min_fee)
    {
      printf("ERROR: fee too small -- the ACCEPT is rejected! (%lu is smaller than %lu)\n", fee, min_fee);
      ++InvalidCount_per_spec;
      return;
    }

    map<string, msc_accept>::iterator my_it = my_accepts.find(buyer);

    // if an accept by this same buyer is found -- erase it and replace with the new one
    // TODO: determine if that's the correct course of action per Mastercoin protocol : consensus question
    if (my_it != my_accepts.end()) my_accepts.erase(my_it);
    my_accepts.insert(std::make_pair(buyer,msc_accept(desired, fee, block)));
  }

  void print(string address, bool bPrintAcceptsToo = false)
  {
  const double coins = (double)offer_amount/(double)COIN;
  const double original_coins = (double)original_offer_amount/(double)COIN;
  const double wants_total = (double)BTC_desired/(double)COIN;
  const double price = coins ? wants_total/coins : 0;

    printf("%36s selling %12.8lf (%12.8lf available) %4s for %12.8lf BTC (price: %1.8lf), in #%d blimit= %3u, minfee= %1.8lf\n",
     address.c_str(), original_coins, coins, c_strMastercoinCurrency(currency), wants_total, price, offerBlock, blocktimelimit,(double)min_fee/(double)COIN);

        if (bPrintAcceptsToo)
        for(map<string, msc_accept>::iterator my_it = my_accepts.begin(); my_it != my_accepts.end(); my_it++)
        {
          // my_it->first = key
          // my_it->second = value

          printf("\t%35s ", (my_it->first).c_str());
          (my_it->second).print();
        }
  }

  bool IsCustomerOnTheAcceptanceListAndWithinBlockTimeLimit(string customer, int blockNow)
  {
  int problem = 0;

    const map<string, msc_accept>::iterator my_it = my_accepts.find(customer);

    if (my_it != my_accepts.end())
    {
      printf("%s()now: %d, class: %d, limit: %d, line %d, file: %s\n",
       __FUNCTION__, blockNow, (my_it->second).block, blocktimelimit, __LINE__, __FILE__);

      if ((blockNow - (my_it->second).block) > (int) blocktimelimit)
      {
        problem+=1;
        ++InvalidCount_per_spec;
      }
    }
    else problem+=10;

    printf("%s(%s):problem=%d, line %d, file: %s\n", __FUNCTION__, customer.c_str(), problem, __LINE__, __FILE__);

    return (!problem);
  }

};

static map<string, msc_offer> my_offers;

CCriticalSection cs_tally;

// this is the master list of all amounts for all addresses for all currencies, map is sorted by Bitcoin address
map<string, msc_tally> msc_tally_map;

// TODO: when HardFail is true -- do not transfer any funds if only a partial transfer would succeed
bool update_tally_map(string who, unsigned int which, int64_t amount, bool bReserved = false)
{
bool bReturn = true;

  if (msc_debug2)
   printf("%s(%s, %d, %+ld%s), line %d, file: %s\n", __FUNCTION__, who.c_str(), which, amount, bReserved ? " RESERVED":"", __LINE__, __FILE__);

  LOCK(cs_tally);

      const map<string, msc_tally>::iterator my_it = msc_tally_map.find(who);
      if (my_it != msc_tally_map.end())
      {
        // element found -- update
        if (!bReserved) bReturn = (my_it->second).msc_update_moneys(which, amount);
        else bReturn = (my_it->second).msc_update_reserved(which, amount);
      }
      else
      {
        // not found -- insert
        if (0<=amount) msc_tally_map.insert(std::make_pair(who,msc_tally(which, amount)));
        else bReturn = false;
      }

  if (!bReturn) printf("%s(%s, %d, %+ld%s) INSUFFICIENT FUNDS\n", __FUNCTION__, who.c_str(), which, amount, bReserved ? " RESERVED":"");

  return bReturn;
}

// this class is the in-memory structure for the various MSC transactions (types) I've processed
//  ordered by the block #
// The class responsible for sorted tx parsing. It does the initial block parsing (via a priority_queue, sorted).
// Also used as new block are received.
//
// It invokes other classes & methods: offers, accepts, tallies (balances).
class msc
{
private:
  string sender;
  string receiver;
  string txid;
  int block;
  unsigned int tx_idx;  // tx # within the block, 0-based
  int pkt_size;
  unsigned char pkt[MAX_PACKETS * PACKET_SIZE];
  uint64_t nValue;
  int multi;  // Class A = 0, Class B = 1
  uint64_t tx_fee_paid;

  void update_offer_map(string seller_addr, unsigned int curr, uint64_t desired, uint64_t fee, unsigned char btl) const
  {
  // handle the offer
  map<string, msc_offer>::iterator my_it;
  const string combo = STR_ADDR_CURR_COMBO(seller_addr);

    if (msc_debug4)
    printf("%s(%s|%s), nValue=%lu), line %d, file: %s\n",
     __FUNCTION__, seller_addr.c_str(), combo.c_str(), nValue, __LINE__, __FILE__);

      my_it = my_offers.find(combo);

      // TODO: handle an existing offer (cancel, or update or what?) consensus question
      if (my_it != my_offers.end())
      {
        printf("%s() OFFER FOUND - UPDATING, line %d, file: %s\n", __FUNCTION__, __LINE__, __FILE__);
        // element found -- update
        (my_it->second).offer_update(block, nValue, curr, desired, fee, btl);
      }
      else
      {
        printf("%s() NEW OFFER - INSERTING, line %d, file: %s\n", __FUNCTION__, __LINE__, __FILE__);
        // not found -- insert
        my_offers.insert(std::make_pair(combo, msc_offer(block, nValue, curr, desired, fee, btl)));
      }
  }

  bool offerExists(string seller_addr, unsigned int curr) const
  {
  const string combo = STR_ADDR_CURR_COMBO(seller_addr);
  map<string, msc_offer>::iterator my_it = my_offers.find(combo);

    return !(my_it == my_offers.end());
  }

  void offerCancel(string seller_addr, unsigned int curr) const
  {
  const string combo = STR_ADDR_CURR_COMBO(seller_addr);
  map<string, msc_offer>::iterator my_it = my_offers.find(combo);
  uint64_t nValue;

    if (my_offers.end() == my_it) return; // offer not found

//    string sellerAddr = (my_it->first).substr(0, strLine.find("-"));  // redundant
    nValue = (my_it->second).getOfferAmount();

    // take from RESERVED, give to REAL
    if (!update_tally_map(seller_addr, curr, - nValue, true))
    {
      ++InsufficientFunds;
      return;
    }
    update_tally_map(seller_addr, curr, nValue);

    // erase the offer from the map
    my_offers.erase(my_it);
  }

  // will replace the previous accept for a specific item from this buyer
  void update_offer_accepts(int curr)
  {
  // find the offer
  map<string, msc_offer>::iterator my_it;
  const string combo = STR_ADDR_CURR_COMBO(receiver);

      my_it = my_offers.find(combo);
      if (my_it != my_offers.end())
      {
        printf("%s(%s) OFFER FOUND, line %d, file: %s\n", __FUNCTION__, combo.c_str(), __LINE__, __FILE__);
        // offer found -- update
        (my_it->second).offer_accept(sender, nValue, block, tx_fee_paid);
      }
      else
      {
        printf("SELL OFFER NOT FOUND %s !!!\n", combo.c_str());
      }
  }

 // the 31-byte packet & the packet #
 // int interpretPacket(int blocknow, unsigned char pkt[], int size)
 // NOTE: TMSC is ignored for now...
 int interpretPacket()
 {
 unsigned short version = MP_TX_PKT_V0;
 unsigned int type, currency;
 uint64_t amount_desired, min_fee;
 unsigned char blocktimelimit, subaction = 0;

//  if (PACKET_SIZE-1 > pkt_size)
  if (PACKET_SIZE_CLASS_A > pkt_size)  // class A could be 19 bytes
  {
    printf("%s(); size = %d, line %d, file: %s\n", __FUNCTION__, pkt_size, __LINE__, __FILE__);
    return -1;
  }

  // collect version
  memcpy(&version, &pkt[0], 2);
  swapByteOrder16(version);

  // blank out version bytes in the packet
  pkt[0]=0; pkt[1]=0;
  
  memcpy(&type, &pkt[0], 4);
  memcpy(&currency, &pkt[4], 4);
  memcpy(&nValue, &pkt[8], 8);

  if (ignoreTMSC)
  if (currency != MASTERCOIN_CURRENCY_MSC)
  {
    return -2;
  }

  printf("version: %d, Class %s\n", version, !multi ? "A":"B");

  // FIXME: only do swaps for little-endian system(s) !
  swapByteOrder32(type);
  swapByteOrder32(currency);
  swapByteOrder64(nValue);

    printf("\t            type: %u (%s)\n", type, c_strMastercoinType(type));
    printf("\t        currency: %u (%s)\n", currency, c_strMastercoinCurrency(currency));
    printf("\t           value: %lu.%08lu\n", nValue/COIN, nValue%COIN);

  // further processing for complex types
  // TODO: version may play a role here !
  switch(type)
  {
    case MSC_TYPE_SIMPLE_SEND:
      if (sender.empty()) ++InvalidCount_per_spec;
      if (!update_tally_map(sender, currency, - nValue)) break;
      // special case: if can't find the receiver -- assume sending to itself !
      // may also be true for BTC payments........
      // TODO: think about this..........
      if (receiver.empty())
      {
        receiver = sender;
      }
      update_tally_map(receiver, currency, nValue);
      break;

    case MSC_TYPE_TRADE_OFFER:
    {
    enum ActionTypes { INVALID = 0, NEW = 1, UPDATE = 2, CANCEL = 3 };
    bool bActionNew = false;
    bool bActionUpdate = false;
    bool bActionCancel = false;

      memcpy(&amount_desired, &pkt[16], 8);
      memcpy(&blocktimelimit, &pkt[24], 1);
      memcpy(&min_fee, &pkt[25], 8);
      memcpy(&subaction, &pkt[33], 1);

      // FIXME: only do swaps for little-endian system(s) !
      swapByteOrder64(amount_desired);
      swapByteOrder64(min_fee);

    printf("\t  amount desired: %lu.%08lu\n", amount_desired / COIN, amount_desired % COIN);
    printf("\tblock time limit: %u\n", blocktimelimit);
    printf("\t         min fee: %lu.%08lu\n", min_fee / COIN, min_fee % COIN);
    printf("\t      sub-action: %u\n", subaction);

      // figure out which Action this is based on amount for sale, version & etc.
      switch (version)
      {
        case MP_TX_PKT_V0:
          if ((0 == nValue) && (MASTERCOIN_CURRENCY_TMSC == currency)) bActionCancel = true;

          if (0 != nValue)
          {
            if (!offerExists(sender, currency))
            {
              bActionNew = true;
            }
            else
            {
              bActionUpdate = true;
            }
          }

          break;

        case MP_TX_PKT_V1:  // same as default right now
        default:
        {
          if (offerExists(sender, currency))
          {
            if ((CANCEL != subaction) && (UPDATE != subaction))
            {
              printf("%s() INVALID SELL OFFER -- ONE ALREADY EXISTS, line %d, file: %s\n", __FUNCTION__, __LINE__, __FILE__);
              ++InvalidCount_per_spec;
              break;
            }
          }
 
          switch (subaction)
          {
            case NEW: bActionNew = true; break;
            case UPDATE: bActionUpdate = true; break;
            case CANCEL: bActionCancel = true; break;
            default:
              ++InvalidCount_per_spec;
              break;
          }

          break;
        }
      };

      // my simple math is: UPDATE = CANCEL + NEW
      if (bActionUpdate)
      {
        bActionCancel = true;
        bActionNew = true;
      }
      
      if (bActionCancel)
      {
        offerCancel(sender, currency);  // tally is adjusted internally
      }

      if (bActionNew)
      {
        if (update_tally_map(sender, currency, - nValue)) // subtract from available
        {
          update_tally_map(sender, currency, nValue, true); // put in reserve
          update_offer_map(sender, currency, amount_desired, min_fee, blocktimelimit);
        }
      }
      break;
    }

    case MSC_TYPE_ACCEPT_OFFER_BTC:
      // the min fee spec requirement is checked in the following function
      update_offer_accepts(currency);
      break;
  }

  return 0;
 }

public:
//  mutable CCriticalSection cs_msc;  // TODO: need to refactor first...

  msc() : block(-1), pkt_size(0), tx_fee_paid(0)
  {
  }

  msc(string s, string r, uint64_t n, string t, int b, unsigned int idx, unsigned char p[], unsigned int size, int fMultisig, uint64_t txf) :
   sender(s), receiver(r), txid(t), block(b), tx_idx(idx), pkt_size(size), nValue(n), multi(fMultisig), tx_fee_paid(txf)
  {
    memcpy(pkt, p, size < sizeof(pkt) ? size : sizeof(pkt));
  }

  bool operator<(const msc& other) const
  {
    // sort by block # & additionally the tx index within the block
    if (block != other.block) return block > other.block;
    return tx_idx > other.tx_idx;
  }

  void print()
  {
    printf("===BLOCK: %d =txid: %s =fee: %1.8lf ====\n", block, txid.c_str(), (double)tx_fee_paid/(double)COIN);
    printf("sender: %s ; receiver: %s\n", sender.c_str(), receiver.c_str());

    if (0<pkt_size)
    {
      printf("pkt: %s\n", HexStr(pkt, pkt_size + pkt, false).c_str());
      (void) interpretPacket();
    }
    else
    {
      ++InvalidCount_per_spec;
    }
  }
};

// incoming BTC payment for the offer
// TODO: verify proper partial payment handling
int matchBTCpayment(string seller, string customer, uint64_t BTC_amount, int blockNow)
{
  for (unsigned int curr=0;curr<MSC_MAX_KNOWN_CURRENCIES;curr++)
  {
  const string combo = STR_ADDR_CURR_COMBO(seller);

  if (msc_debug4) printf("%s(looking for: %s), line %d, file: %s\n", __FUNCTION__, combo.c_str(), __LINE__, __FILE__);

  const map<string, msc_offer>::iterator my_it = my_offers.find(combo);

    if (my_it != my_offers.end())
    {
      printf("%s(%s) FOUND the SELL OFFER, line %d, file: %s\n", __FUNCTION__, combo.c_str(), __LINE__, __FILE__);

      // element found
      msc_offer &offer = (my_it->second);

      offer.print((my_it->first));

      // must now check if the customer is in the accept list
      // if he is -- he will be removed by the periodic cleanup, upon every new block received as the spec says
      if (offer.IsCustomerOnTheAcceptanceListAndWithinBlockTimeLimit(customer, blockNow))
      {
        uint64_t target_currency_amount = offer.getCurrencyAmount(BTC_amount);

          // good, now adjust the amounts!!!
          if (update_tally_map(seller, offer.getCurrency(), - target_currency_amount, true))  // remove from reserve of the seller
          {
            update_tally_map(customer, offer.getCurrency(), target_currency_amount);  // give to buyer
            // update the amount available in the offer
            offer.reduceOfferAmount(target_currency_amount);
          }

          printf("#######################################################\n");
      }
    }
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// THE TODO LIST, WHAT'S MISSING, NOT DONE YET:
//  1) checks to ensure the seller has enough funds
//  2) checks to ensure the sender has enough funds
//  3) checks to ensure there are enough funds when the 'accept' message is received
//  4) partial order fulfilment is not yet handled (spec says all is sold if larger than available is put on sale, etc.)
//  5) return false as needed and check returns of msc_update_* functions
//  6) verify large-number calculations (especially divisions & multiplications)
//  7) need to detect cancelations & updates of sell offers -- and handle partially fullfilled offers...
//  8) most important: figure out all the coins in existence and add all that prebuilt data
//  9) build in consesus checks with the masterchain.info & masterchest.info -- possibly run them automatically, daily (?)
// 10) need a locking mechanism between Core & Qt -- to retrieve the tally, for instance, this and similar to this: LOCK(wallet->cs_wallet);
//


// MSC_periodic_function() will be called upon every new block received
// it performs cleanup and other functions
int MSC_periodic_function()
{
// for every new received block must do:
// 1) remove expired entries from the accept list (per spec accept entries are valid until their blocklimit expiration; because the customer can keep paying BTC for the offer in several installments)
// 2) update the amount in the Exodus address

  return 0;
}

// called once per block
int mastercoin_block_handler(int nBlock)
{
  return 0;
}

static priority_queue<msc>txq;
// idx is position within the block, 0-based
int msc_tx_push(const CTransaction &wtx, int nBlock, unsigned int idx)
{
string strSender;
uint64_t nMax = 0;
// class A: data & address storage -- combine them into a structure or something
vector<string>script_data;
vector<string>address_data;
vector<uint64_t>value_data;
uint64_t nExodusValue = 0;
string strReference;
unsigned char single_pkt[MAX_PACKETS * PACKET_SIZE];
unsigned int packet_size = 0;
int fMultisig = 0;
int marker_count = 0;
// class B: multisig data storage
vector<string>multisig_script_data;
uint64_t inAll = 0;
uint64_t outAll = 0;
uint64_t txFee = 0;

            // quickly go through the outputs & ensure there is a marker (a send to the Exodus address)
            for (unsigned int i = 0; i < wtx.vout.size(); i++)
            {
            CTxDestination dest;
            string strAddress;

              outAll += wtx.vout[i].nValue;

              if (ExtractDestination(wtx.vout[i].scriptPubKey, dest))
              {
                strAddress = CBitcoinAddress(dest).ToString();

                if (exodus == strAddress)
                {
                  ++marker_count;
                  // TODO: add other checks to verify a mastercoin tx !!!
                  nExodusValue = wtx.vout[i].nValue;
                }
              }
            }

            // TODO: ensure only 1 output to the Exodus address is allowed (corner case?)
//            if (1 != marker_count)
            if (!marker_count)  // 1Exodus is a special case, TODO: resolve later after PoC Sprint
// https://github.com/m21/mastercore/commit/fbf06f6dbda06b5a8cf061414ff76f42194544d8#diff-7322bd4b20fe7eed15aa568e8905f657R607
            {
              // not Mastercoin -- no output to the 'marker' Exodus or more than 1 -- more than 1 is OK per Zathras, May 2014
              // TODO: if multiple markers are visible : how is nExodusValue calculated?
  // [11:33:08 PM] my99key: I don't have a case -- so if multiple outputs to 1Exodus are found in a Class A TX -- it is not a valid MP TX?
  // [11:33:36 PM] faiz: as far as i know, the only case that it would be valid is if the TX was actually also sent by exodus
  // So, send from 1Exodus to self may have multiple outputs in a Class A TX !!!
              return -1;
            }

            if (msc_debug4 || msc_debug2 || msc_debug3) printf("================BLOCK: %d======\ntxid: %s\n", nBlock, wtx.GetHash().GetHex().c_str());

            // now save output addresses & scripts for later use
            // also determine if there is a multisig in there, if so = Class B
            for (unsigned int i = 0; i < wtx.vout.size(); i++)
            {
            CTxDestination dest;
            string strAddress;

              if (ExtractDestination(wtx.vout[i].scriptPubKey, dest))
              {
                strAddress = CBitcoinAddress(dest).ToString();

                if (exodus != strAddress)
                {
                  if (msc_debug3) printf("saving address_data #%d: %s:%s\n", i, strAddress.c_str(), wtx.vout[i].scriptPubKey.ToString().c_str());

                  // saving for Class A processing or reference
                  wtx.vout[i].scriptPubKey.msc_parse(script_data);
                  address_data.push_back(strAddress);
                  value_data.push_back(wtx.vout[i].nValue);
                }
              }
              else
              {
              // a multisig ?
              txnouttype type;
              std::vector<CTxDestination> vDest;
              int nRequired;

                if (ExtractDestinations(wtx.vout[i].scriptPubKey, type, vDest, nRequired))
                {
                  ++fMultisig;
                }
              }
            }

            if (msc_debug3)
            {
              printf("address_data.size=%lu\n", address_data.size());
              printf("script_data.size=%lu\n", script_data.size());
              printf("value_data.size=%lu\n", value_data.size());
            }

            // now go through inputs & identify the sender, collect input amounts
            // go through inputs, find the largest per Mastercoin protocol, the Sender
            for (unsigned int i = 0; i < wtx.vin.size(); i++)
            {
            CTxDestination address;

            if (msc_debug) printf("vin=%d:%s\n", i, wtx.vin[i].scriptSig.ToString().c_str());

            CTransaction txPrev;
            uint256 hashBlock;
            GetTransaction(wtx.vin[i].prevout.hash, txPrev, hashBlock, true);  // get the vin's previous transaction 
            unsigned int n = wtx.vin[i].prevout.n;
            CTxDestination source;

            uint64_t nValue = txPrev.vout[n].nValue;

             inAll += nValue;

             if (ExtractDestination(txPrev.vout[n].scriptPubKey, source))  // extract the destination of the previous transaction's vout[n]
             {
                CBitcoinAddress addressSource(source);              // convert this to an address
   
                if (nValue > nMax)
                {
                  nMax = nValue;
                  strSender = addressSource.ToString();
                }
             }
              if (msc_debug) printf("vin=%d:%s\n", i, wtx.vin[i].ToString().c_str());
            }

            txFee = inAll - outAll; // this is the fee paid to miners for this TX

            if (!strSender.empty())
            {
              if (msc_debug2) printf("The Sender: %s : Value= %lu.%08lu ; fee= %lu.%08lu\n", strSender.c_str(), nMax / COIN, nMax % COIN, txFee/COIN, txFee%COIN);
            }
            else
            {
              printf("The sender is EMPTY !!! txid: %s\n", wtx.GetHash().GetHex().c_str());
              return -5;
            }

            // go through the outputs
            for (unsigned int i = 0; i < wtx.vout.size(); i++)
            {
            CTxDestination address;

              // if TRUE -- non-multisig
              if (ExtractDestination(wtx.vout[i].scriptPubKey, address))
              {
              }
              else
              {
                // probably a multisig -- get them

        txnouttype type;
        std::vector<CTxDestination> vDest;
        int nRequired;

        // CScript is a std::vector -- see what else is available for vector !!!
        if (msc_debug) printf("scriptPubKey: %s\n", wtx.vout[i].scriptPubKey.getHex().c_str());

        if (ExtractDestinations(wtx.vout[i].scriptPubKey, type, vDest, nRequired))
        {
          if (msc_debug) printf(" >> multisig: ");
            BOOST_FOREACH(const CTxDestination &dest, vDest)
            {
            CBitcoinAddress address = CBitcoinAddress(dest);
            CKeyID keyID;

            if (!address.GetKeyID(keyID))
            {
            // TODO: add an error handler
            }

              // base_uint is a superclass of dest, size(), GetHex() is the same as ToString()
//              printf("%s size=%d (%s); ", address.ToString().c_str(), keyID.size(), keyID.GetHex().c_str());
              if (msc_debug) printf("%s ; ", address.ToString().c_str());

            }
          if (msc_debug) printf("\n");

          // TODO: verify that we can handle multiple multisigs per tx
          wtx.vout[i].scriptPubKey.msc_parse(multisig_script_data);

          break;  // get out of processing this first multisig
        }
              }
            } // end of the outputs' for loop

              unsigned char sha_input[128];
              unsigned char sha_result[128];
              string strObfuscatedHash[1+MAX_SHA256_OBFUSCATION_TIMES];
              vector<unsigned char> vec_chars;

              strcpy((char *)sha_input, strSender.c_str());
              // do only as many re-hashes as there are mastercoin packets, per spec, 255 per Zathras
              // TODO -- verify what he meant:
              // Zahtras: "We no longer need to brute force this though, since the location of the packet is fixed in order of the multisigs/vouts
              // we can now determine sequence (and thus number of times to hash to deobfuscate) easily."
              for (unsigned int j = 1; j<=MAX_SHA256_OBFUSCATION_TIMES;j++)
              {
                SHA256(sha_input, strlen((const char *)sha_input), sha_result);

                  vec_chars.resize(32);
                  memcpy(&vec_chars[0], &sha_result[0], 32);
                  strObfuscatedHash[j] = HexStr(vec_chars);
                  boost::to_upper(strObfuscatedHash[j]); // uppercase per spec

                  if (msc_debug6) if (10>j) printf("%d: sha256 hex: %s\n", j, strObfuscatedHash[j].c_str());
                  strcpy((char *)sha_input, strObfuscatedHash[j].c_str());
              }

          unsigned char packets[MAX_PACKETS][32];
          int mdata_count = 0;  // multisig data count
          if (!fMultisig)
          {
          string strData;
          unsigned char seq = 0xFF;

          // non-multisig data packets from this tx may be found here...
            // TODO: figure out what the data packet(s) are/is for non-multisig Class A transaction
            // from msc_utils_parsing.py, March 2014
            // look for data & for reference:
            // first, check the list of outputs with values equal to Exodus's
            // second, check the list of all outputs even with random values
            // third, not yet implemented here -- is it needed?:
            //    # Level 3
            //    # all output values are equal in size and if there are three outputs
            //    # of these type of outputs total
            // Class A

            // find data
          for (unsigned k = 0; k<script_data.size();k++)
          {
            if (msc_debug3) printf("data[%d]:%s: %s (%lu.%08lu)\n", k, script_data[k].c_str(), address_data[k].c_str(), value_data[k] / COIN, value_data[k] % COIN);

            {
              string strSub = script_data[k].substr(2,16);
              seq = (ParseHex(script_data[k].substr(0,2)))[0];
  
                if (("0000000000000001" == strSub) || ("0000000000000002" == strSub))
                {
                  if (strData.empty()) strData = script_data[k].substr(2*1,2*PACKET_SIZE_CLASS_A);
  
                  if (msc_debug3) printf("strData #1:%s, seq = %x\n", strData.c_str(), seq);

                  if (value_data[k] == nExodusValue)
                  {
                    if (msc_debug3) printf("strData #2:%s, seq = %x\n", strData.c_str(), seq);
                    strData = script_data[k].substr(2,2*PACKET_SIZE_CLASS_A);
                    break;
            }
                }
              }
            }

            if (!strData.empty())
            {
              ++seq;
              // look for reference using the seq #
              for (unsigned k = 0; k<script_data.size();k++)
              {
                if ((address_data[k] != strData) && (address_data[k] != exodus))
                {
                  if (seq == ParseHex(script_data[k].substr(0,2))[0])
                  {
                    strReference = address_data[k];
            }
                }
              }

              // TODO:     # on failure with 3 outputs case, take non data/exodus to be the recipient
              if (strReference.empty())
              {
                if (3 == script_data.size())
                {
                  for (unsigned k = 0; k<script_data.size();k++)
                  {
                    if ((address_data[k] != strData) && (address_data[k] != exodus))
                    {
                      strReference = address_data[k];
                    }
                  }
                }
              }
            }

            if (strData.empty() || strReference.empty())
            {
            // this must be the BTC payment - validate (?)
            // TODO
            // ...
              printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
              printf("sender: %s , receiver: %s\n", strSender.c_str(), strReference.c_str());
              printf("!!!!!!!!!!!!!!!!! this may be the BTC payment for an offer !!!!!!!!!!!!!!!!!!!!!!!!\n");

            // TODO collect all payments made to non-itself & non-exodus and their amounts -- these may be purchases!!!

              // go through the outputs, once again...
              {
                for (unsigned int i = 0; i < wtx.vout.size(); i++)
                {
                CTxDestination dest;
    
                  if (ExtractDestination(wtx.vout[i].scriptPubKey, dest))
                  {
                  const string strAddress = CBitcoinAddress(dest).ToString();

                    if (exodus == strAddress) continue;
                    if (strSender == strAddress) continue;
                    printf("payment? %s %11.8lf\n", strAddress.c_str(), (double)wtx.vout[i].nValue/(double)COIN);

                    // check everything & pay BTC for the currency we are buying here...
                    matchBTCpayment(strAddress, strSender, wtx.vout[i].nValue, nBlock);
                    return 0;
                  }
                }
              }
            }
            else
            {
            // valid Class A packet almost ready
              if (msc_debug3) printf("valid Class A:from=%s:to=%s:data=%s\n", strSender.c_str(), strReference.c_str(), strData.c_str());
              packet_size = PACKET_SIZE_CLASS_A;
              memcpy(single_pkt, &ParseHex(strData)[0], packet_size);
            }
          }
          else // if (fMultisig)
          {
            unsigned int k = 0;
            // gotta find the Reference
            BOOST_FOREACH(const string &addr, address_data)
            {
              if (msc_debug3) printf("ref? data[%d]:%s: %s (%lu.%08lu)\n", k, script_data[k].c_str(), addr.c_str(), value_data[k] / COIN, value_data[k] % COIN);
              ++k;
              if ((addr != exodus) && (addr != strSender))
              {
                strReference = addr;
                break;
              }
            }

#if 0
            // if can't find the reference for a multisig tx -- assume the sender is sending MSC to itself !
            if (strReference.empty()) strReference = strSender;
#endif


          if (msc_debug) printf("%s(), line %d, file: %s\n", __FUNCTION__, __LINE__, __FILE__);
          // multisig , Class B; get the data packets can be found here...
          for (unsigned k = 0; k<multisig_script_data.size();k++)
          {

          if (msc_debug) printf("%s(), line %d, file: %s\n", __FUNCTION__, __LINE__, __FILE__);
            CPubKey key(ParseHex(multisig_script_data[k]));
            CKeyID keyID = key.GetID();
            string strAddress = CBitcoinAddress(keyID).ToString();
            char *c_addr_type = (char *)"";
            string strPacket;

          if (msc_debug) printf("%s(), line %d, file: %s\n", __FUNCTION__, __LINE__, __FILE__);
            if (exodus == strAddress) c_addr_type = (char *)" (EXODUS)";
            else
            if (strAddress == strSender)
            {
              c_addr_type = (char *)" (SENDER)";
              // Are we the sender of BTC here?
              // TODO: do we care???
            }
            else
            {
              // this is a data packet, must deobfuscate now
              vector<unsigned char>hash = ParseHex(strObfuscatedHash[mdata_count+1]);      
              vector<unsigned char>packet = ParseHex(multisig_script_data[k].substr(2*1,2*PACKET_SIZE));

              for (unsigned int i=0;i<packet.size();i++)
              {
                packet[i] ^= hash[i];
              }

              // ensure the first byte of the first packet is 01; is it the sequence number???
              if (03 >= packet[0])
              {
                memcpy(&packets[mdata_count], &packet[0], PACKET_SIZE);
                strPacket = HexStr(packet.begin(),packet.end(), false);
                ++mdata_count;
              }
            }

          if (msc_debug) printf("%s(), line %d, file: %s\n", __FUNCTION__, __LINE__, __FILE__);
            if (msc_debug2)
            printf("multisig_data[%d]:%s: %s%s\n", k, multisig_script_data[k].c_str(), strAddress.c_str(), c_addr_type);
            if (!strPacket.empty())
            {
              if (msc_debug) printf("packet #%d: %s\n", mdata_count, strPacket.c_str());
            }
          if (msc_debug) printf("%s(), line %d, file: %s\n", __FUNCTION__, __LINE__, __FILE__);
          }

            packet_size = mdata_count * (PACKET_SIZE - 1);

          if (msc_debug) printf("%s(), line %d, file: %s\n", __FUNCTION__, __LINE__, __FILE__);
          }
          if (msc_debug) printf("%s(), line %d, file: %s\n", __FUNCTION__, __LINE__, __FILE__);

            // now decode mastercoin packets
            for (int m=0;m<mdata_count;m++)
            {
              if (msc_debug2)
              printf("m=%d: %s\n", m, HexStr(packets[m], PACKET_SIZE + packets[m], false).c_str());

              // ignoring sequence numbers for Class B packets -- TODO: revisit this
              memcpy(m*(PACKET_SIZE-1)+single_pkt, 1+packets[m], PACKET_SIZE-1);
            }

            if (msc_debug2) printf("single_pkt: %s\n", HexStr(single_pkt, packet_size + single_pkt, false).c_str());

            txq.push(msc(strSender, strReference, 0, wtx.GetHash().GetHex(), nBlock, idx, single_pkt, packet_size, fMultisig, (inAll-outAll)));  

  return (fMultisig ? 2:1);
}

int msc_tx_pop()
{
  if (txq.empty()) return -1;

  {
  msc tx_top;

    tx_top = txq.top();
    tx_top.print();
    txq.pop();
  }

  return 0;
}

// display the tally map & the offer/accept list(s)
Value mscrpc(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getblockcount\n"
            "\nReturns the number of blocks in the longest block chain.\n"
            "\nResult:\n"
            "n    (numeric) The current block count\n"
            "\nExamples:\n"
            + HelpExampleCli("getblockcount", "")
            + HelpExampleRpc("getblockcount", "")
        );

        for(map<string, msc_offer>::iterator my_it = my_offers.begin(); my_it != my_offers.end(); my_it++)
        {
          // my_it->first = key
          // my_it->second = value
          (my_it->second).print((my_it->first), true);
        }


  printf("%s(), line %d, file: %s\n", __FUNCTION__, __LINE__, __FILE__); int count = 0;


        for(map<string, msc_tally>::iterator my_it = msc_tally_map.begin(); my_it != msc_tally_map.end(); my_it++)
        {
          // my_it->first = key
          // my_it->second = value

          printf("%34s : ", (my_it->first).c_str());
          (my_it->second).print();

          ++count;
        }

  printf("%s(), line %d, file: %s\n", __FUNCTION__, __LINE__, __FILE__);

    return chainActive.Height();
}


Value mgetbalance(const Array& params, bool fHelp)
{
  LogPrintf("%s(), line %d, file: %s\n", __FUNCTION__, __LINE__, __FILE__);
  printf("%s(), line %d, file: %s\n", __FUNCTION__, __LINE__, __FILE__);
  fprintf(stderr, "%s(), line %d, file: %s\n", __FUNCTION__, __LINE__, __FILE__);
        int64_t nBalance = 0;
        return  ValueFromAmount(nBalance);
}

int msc_function(int nHeight)
{
int n_total = 0, n_found = 0;
int max_block = chainActive.Height();

  int nParam2 = 0;

//    nParam2 = params[1].get_int();

//  msc_tally_map.clear();
  my_offers.clear();
  printf("starting block= %d, max_block= %d\n", nHeight, max_block);

  CBlock block;
  for (int blockNum = nHeight;blockNum<=max_block;blockNum++)
  {
    CBlockIndex* pblockindex = chainActive[blockNum];
    if (msc_debug) printf("%s(), line %d, file: %s\n", __FUNCTION__, __LINE__, __FILE__);
    string strBlockHash = pblockindex->GetBlockHash().GetHex();

    if (msc_debug) printf("%s(%d,%d; max=%d):%s, line %d, file: %s\n",
     __FUNCTION__, blockNum, nParam2, chainActive.Height(), strBlockHash.c_str(), __LINE__, __FILE__);

    ReadBlockFromDisk(block, pblockindex);

    if (msc_debug) printf("%s(), line %d, file: %s\n", __FUNCTION__, __LINE__, __FILE__);

    if (msc_debug) printf("%s(), line %d, file: %s\n", __FUNCTION__, __LINE__, __FILE__);

    int tx_count = 0;
    BOOST_FOREACH(const CTransaction&tx, block.vtx)
    {

//      if (msc_debug) printf("%4d:txid:%s\n", tx_count, tx.GetHash().GetHex().c_str());

      if (0 < msc_tx_push((const CTransaction) tx, blockNum, tx_count)) ++n_found;

      ++tx_count;
    }

    n_total += tx_count;
    if (msc_debug) printf("%4d:n_total= %d, n_found= %d\n", blockNum, n_total, n_found);

    {

/*
        msc tx_top;
        while (!txq.empty())
        {
          tx_top = txq.top();
          tx_top.print();
          txq.pop();
        }
*/
        while (!txq.empty()) msc_tx_pop();
    }
    if (msc_debug) printf("%s(), line %d, file: %s\n", __FUNCTION__, __LINE__, __FILE__);
  }

        for(map<string, msc_tally>::iterator my_it = msc_tally_map.begin(); my_it != msc_tally_map.end(); my_it++)
        {
          // my_it->first = key
          // my_it->second = value

          printf("%34s =>> ", (my_it->first).c_str());
          (my_it->second).print();
        }

  printf("starting block= %d, max_block= %d\n", nHeight, max_block);
  printf("n_total= %d, n_found= %d\n", n_total, n_found);

//  return my_blockToJSON(block, pblockindex);
//  return strBlockHash;
  return 0;
}

Value mscm(const Array& params, bool fHelp)
{
  printf("%s(), line %d, file: %s\n", __FUNCTION__, __LINE__, __FILE__);

    if (fHelp || params.size() != 1)
        throw runtime_error(
            "getblockhash index\n"
            "\nReturns hash of block in best-block-chain at index provided.\n"
            "\nArguments:\n"
            "1. index         (numeric, required) The block index\n"
            "\nResult:\n"
            "\"hash\"         (string) The block hash\n"
            "\nExamples:\n"
            + HelpExampleCli("getblockhash", "1000")
            + HelpExampleRpc("getblockhash", "1000")
        );

    int nHeight = params[0].get_int();
    if (nHeight < 0 || nHeight > chainActive.Height())
        throw runtime_error("Block number out of range.");

  msc_function(nHeight);
  return (string("done"));
}

int input_msc_balances_string(const string &s)
{
uint64_t  uValue = 0, uReserved = 0;
std::vector<std::string> vstr;
boost::split(vstr, s, boost::is_any_of(" ,="), token_compress_on);
int i = 0;
string strAddress = vstr[0];

//  if (msc_debug4) { BOOST_FOREACH(const string &debug_str, vstr) printf("%s\n", debug_str.c_str()); }

  ++i;

  uValue = boost::lexical_cast<boost::uint64_t>(vstr[i++]);
  uReserved = boost::lexical_cast<boost::uint64_t>(vstr[i++]);
  
  // want to bypass 0-value addresses...
  if ((0 == uValue) && (0 == uReserved)) return 0;

  // ignoring TMSC for now...
  update_tally_map(strAddress, MASTERCOIN_CURRENCY_MSC, uValue);
  update_tally_map(strAddress, MASTERCOIN_CURRENCY_MSC, uReserved, true);

  return 1;
}

// seller-address, offer_block, amount, currency, desired BTC , fee, blocktimelimit
// 13z1JFtDMGTYQvtMq5gs4LmCztK3rmEZga,299076,76375000,1,6415500,10000,6
int input_msc_offers_string(const string &s)
{
int offerBlock;
uint64_t Amount, Desired_BTC;
unsigned int curr;
uint64_t min_fee;
unsigned char blocktimelimit;
std::vector<std::string> vstr;
boost::split(vstr, s, boost::is_any_of(" ,="), token_compress_on);
string sellerAddr;
int i = 0;

  if (7 != vstr.size()) return -1;

  printf("%s(%s), line %d, file: %s\n", __FUNCTION__, s.c_str(), __LINE__, __FILE__);

  sellerAddr = vstr[i++];
  offerBlock = atoi(vstr[i++]);
  Amount = boost::lexical_cast<uint64_t>(vstr[i++]);
  curr = boost::lexical_cast<unsigned int>(vstr[i++]);
  Desired_BTC = boost::lexical_cast<uint64_t>(vstr[i++]);
  min_fee = boost::lexical_cast<uint64_t>(vstr[i++]);
  blocktimelimit = atoi(vstr[i++]);

  if (msc_debug4) { BOOST_FOREACH(const string &debug_str, vstr) printf("%s\n", debug_str.c_str()); }

  const string combo = STR_ADDR_CURR_COMBO(sellerAddr);

  printf("%s(%s):%s:%d:%ld:%d:%ld:%ld:%d\n", __FUNCTION__,
   combo.c_str(), sellerAddr.c_str(), offerBlock, Amount, curr, Desired_BTC, min_fee, blocktimelimit);

  return 0;
}

// seller-address, currency, buyer-address, amount, fee, block
// 13z1JFtDMGTYQvtMq5gs4LmCztK3rmEZga,1, 148EFCFXbk2LrUhEHDfs9y3A5dJ4tttKVd,100000,11000,299126
// 13z1JFtDMGTYQvtMq5gs4LmCztK3rmEZga,1,1Md8GwMtWpiobRnjRabMT98EW6Jh4rEUNy,50000000,11000,299132
int input_msc_accepts_string(const string &s)
{
int nBlock;
std::vector<std::string> vstr;
boost::split(vstr, s, boost::is_any_of(" ,="), token_compress_on);
uint64_t Amount;
uint64_t fee;
unsigned int curr;
string sellerAddr, buyerAddr;
int i = 0;

  if (6 != vstr.size()) return -1;

  printf("%s(%s), line %d, file: %s\n", __FUNCTION__, s.c_str(), __LINE__, __FILE__);

  sellerAddr = vstr[i++];
  curr = boost::lexical_cast<unsigned int>(vstr[i++]);
  buyerAddr = vstr[i++];
  Amount = boost::lexical_cast<uint64_t>(vstr[i++]);
  fee = boost::lexical_cast<uint64_t>(vstr[i++]);
  nBlock = atoi(vstr[i++]);

  if (msc_debug4) { BOOST_FOREACH(const string &debug_str, vstr) printf("%s\n", debug_str.c_str()); }

  const string combo = STR_ADDR_CURR_COMBO(sellerAddr);

  printf("%s(%s):%s:%d:%s:%ld:%ld:%d\n", __FUNCTION__,
   combo.c_str(), sellerAddr.c_str(), curr, buyerAddr.c_str(), Amount, fee, nBlock);

  return 0;
}

int msc_file_load(int what)
{
int lines = 0;
int (*inputLineFunc)(const string &) = NULL;
const string filename = GetDataDir().string() + "/" + string(mastercoin_filenames[what]);

#ifdef  WIN32
// FIXME -- switch to boost:path for Windows compatibility !
#error Implement boost path here
#endif

// TODO: think about placement for preseed files -- perhaps the directory where executables live is better?
// these files are read-only preseeds
// all run-time updates should go to a KV-store (leveldb is envisioned)

  switch (what)
  {
    case FILETYPE_BALANCES:
      msc_tally_map.clear();
      inputLineFunc = input_msc_balances_string;
      break;

    case FILETYPE_OFFERS:
      my_offers.clear();
      inputLineFunc = input_msc_offers_string;
      break;

    case FILETYPE_ACCEPTS:
      inputLineFunc = input_msc_accepts_string;
      break;

    default:
      return -1;
  }

    LogPrintf("Loading %s ... \n", filename);

    ifstream file;
    file.open(filename.c_str());
    if (!file.is_open())
    {
      LogPrintf("%s(): file not found, line %d, file: %s\n", __FUNCTION__, __LINE__, __FILE__);
      return -1;
    }

    while (file.good())
    {
        std::string line;
        std::getline(file, line);
        if (line.empty() || line[0] == '#') continue;

        // remove \r if the file came from Windows
        line.erase( std::remove( line.begin(), line.end(), '\r' ), line.end() ) ;

        if (inputLineFunc) inputLineFunc(line);

        ++lines;
    }

    file.close();

    printf("%s(): file: %s, loaded lines= %d\n", __FUNCTION__, filename.c_str(), lines);
    LogPrintf("%s(): file: %s, loaded lines= %d\n", __FUNCTION__, filename, lines);

  return 0;
}

// called from init.cpp of Bitcoin Core
int mastercoin_init()
{
  printf("%s(), line %d, file: %s\n", __FUNCTION__, __LINE__, __FILE__);
//  (void) load_checkpoint();

  (void) msc_file_load(FILETYPE_BALANCES);
  (void) msc_file_load(FILETYPE_OFFERS);
//  (void) msc_file_load(FILETYPE_ACCEPTS); // not needed per Zathras -- we are capturing blocks for which there are no outstanding accepts!

//  (void) msc_function(292421);  // scan new blocks after the checkpoint above
  (void) msc_function(290630);  // the DEX block, using Zathras msc_balances_290629.txt , md5: f275c5a17bd2d36da8c686f2a4337e06

//  (void) msc_function(249497);  // Exodus block, dump for Zathras

  return 0;
}

unsigned int msc_pos = 0;
unsigned int msc_neg = 0;
unsigned int msc_zero = 0;
unsigned int msc_total = 0; // position within the block, when available, 0-based

// this is called for every new transaction that comes in (actually in block parsing loop)
int mastercoin_tx_handler(const CTransaction &tx, int nBlock, unsigned int idx)
{
int rc = 0;

      {
        rc = msc_tx_push(tx, nBlock, idx);

        if (0>rc) ++msc_neg;
        if (0<rc) ++msc_pos;
        if (0==rc) ++msc_zero;

        ++msc_total;

        (void) msc_tx_pop();
      }

  // display by the end of the block or something:  LogPrintf("%s()msc_tx()=%lu: +=%lu , 0=%lu , -=%lu ,  END, line %d, file: %s\n", __FUNCTION__, msc_total, msc_neg, msc_zero, msc_pos, __LINE__, __FILE__);

  return 0;
}

string msc_tally::getMSC()
{
    // FIXME: negative numbers -- do they work here?
    return strprintf("%d.%08d", moneys[MASTERCOIN_CURRENCY_MSC]/COIN, moneys[MASTERCOIN_CURRENCY_MSC]%COIN);
}

string msc_tally::getTMSC()
{
    // FIXME: negative numbers -- do they work here?
    return strprintf("%d.%08d", moneys[MASTERCOIN_CURRENCY_TMSC]/COIN, moneys[MASTERCOIN_CURRENCY_TMSC]%COIN);
}

