#ifndef _MASTERCOIN_DEX
#define _MASTERCOIN_DEX 1

#include "mastercore.h"

//

// this is the internal format for the offer primary key (TODO: replace by a class method)
#define STR_SELLOFFER_ADDR_CURR_COMBO(x) ( x + "-" + strprintf("%d", curr))
#define STR_ACCEPT_ADDR_CURR_ADDR_COMBO( _seller , _buyer ) ( _seller + "-" + strprintf("%d", curr) + "+" + _buyer)
#define STR_PAYMENT_SUBKEY_TXID_PAYMENT_COMBO(txidStr) ( txidStr + "-" + strprintf("%d", paymentNumber))

// a single outstanding offer -- from one seller of one currency, internally may have many accepts
class CMPOffer
{
private:
  int offerBlock;
  uint64_t offer_amount_original; // the amount of MSC for sale specified when the offer was placed
  unsigned int currency;
  uint64_t BTC_desired_original; // amount desired, in BTC
  uint64_t min_fee;
  unsigned char blocktimelimit;
  uint256 txid;
  unsigned char subaction;

public:
  uint256 getHash() const { return txid; }
  unsigned int getCurrency() const { return currency; }
  uint64_t getMinFee() const { return min_fee ; }
  unsigned char getBlockTimeLimit() { return blocktimelimit; }
  unsigned char getSubaction() { return subaction; }

  uint64_t getOfferAmountOriginal() { return offer_amount_original; }
  uint64_t getBTCDesiredOriginal() { return BTC_desired_original; }

  CMPOffer():offerBlock(0),offer_amount_original(0),currency(0),BTC_desired_original(0),min_fee(0),blocktimelimit(0),txid(0)
  {
  }

  CMPOffer(int b, uint64_t a, unsigned int cu, uint64_t d, uint64_t fee, unsigned char btl, const uint256 &tx)
   :offerBlock(b),offer_amount_original(a),currency(cu),BTC_desired_original(d),min_fee(fee),blocktimelimit(btl),txid(tx)
  {
    if (msc_debug_dex) fprintf(mp_fp, "%s(%lu): %s , line %d, file: %s\n", __FUNCTION__, a, txid.GetHex().c_str(), __LINE__, __FILE__);
  }

  void Set(uint64_t d, uint64_t fee, unsigned char btl, unsigned char suba)
  {
    BTC_desired_original = d;
    min_fee = fee;
    blocktimelimit = btl;
    subaction = suba;
  }

  void saveOffer(ofstream &file, SHA256_CTX *shaCtx, string const &addr ) const {
    // compose the outputline
    // seller-address, ...
    string lineOut = (boost::format("%s,%d,%d,%d,%d,%d,%d,%d,%s")
      % addr
      % offerBlock
      % offer_amount_original
      % currency
      % BTC_desired_original
      % ( MASTERCOIN_CURRENCY_BTC )
      % min_fee
      % (int)blocktimelimit
      % txid.ToString()).str();

    // add the line to the hash
    SHA256_Update(shaCtx, lineOut.c_str(), lineOut.length());

    // write the line
    file << lineOut << endl;
  }
};  // end of CMPOffer class

// do a map of buyers, primary key is buyer+currency
// MUST account for many possible accepts and EACH currency offer
class CMPAccept
{
private:
  uint64_t accept_amount_original;    // amount of MSC/TMSC desired to purchased
  uint64_t accept_amount_remaining;   // amount of MSC/TMSC remaining to purchased
// once accept is seen on the network the amount of MSC being purchased is taken out of seller's SellOffer-Reserve and put into this Buyer's Accept-Reserve
  unsigned char blocktimelimit;       // copied from the offer during creation
  unsigned int currency;              // copied from the offer during creation

  uint64_t offer_amount_original; // copied from the Offer during Accept's creation
  uint64_t BTC_desired_original;  // copied from the Offer during Accept's creation

  uint256 offer_txid; // the original offers TXIDs, needed to match Accept to the Offer during Accept's destruction, etc.

public:
  uint256 getHash() const { return offer_txid; }

  uint64_t getOfferAmountOriginal() { return offer_amount_original; }
  uint64_t getBTCDesiredOriginal() { return BTC_desired_original; }

  int block;          // 'accept' message sent in this block

  unsigned char getBlockTimeLimit() { return blocktimelimit; }
  unsigned int getCurrency() const { return currency; }

  int getAcceptBlock()  { return block; }

  CMPAccept(uint64_t a, int b, unsigned char blt, unsigned int c, uint64_t o, uint64_t btc, const uint256 &txid):accept_amount_remaining(a),blocktimelimit(blt),currency(c),
   offer_amount_original(o), BTC_desired_original(btc),offer_txid(txid),block(b)
  {
    accept_amount_original = accept_amount_remaining;
    fprintf(mp_fp, "%s(%lu), line %d, file: %s\n", __FUNCTION__, a, __LINE__, __FILE__);
  }

  CMPAccept(uint64_t origA, uint64_t remA, int b, unsigned char blt, unsigned int c, uint64_t o, uint64_t btc, const uint256 &txid):accept_amount_original(origA),accept_amount_remaining(remA),blocktimelimit(blt),currency(c),
   offer_amount_original(o), BTC_desired_original(btc),offer_txid(txid),block(b)
  {
    fprintf(mp_fp, "%s(%lu[%lu]), line %d, file: %s\n", __FUNCTION__, remA, origA, __LINE__, __FILE__);
  }

  void print()
  {
    fprintf(mp_fp, "buying: %12.8lf (originally= %12.8lf) in block# %d\n",
     (double)accept_amount_remaining/(double)COIN, (double)accept_amount_original/(double)COIN, block);
  }

  uint64_t getAcceptAmountRemaining() const
  { 
    fprintf(mp_fp, "%s(); buyer still wants = %lu, line %d, file: %s\n", __FUNCTION__, accept_amount_remaining, __LINE__, __FILE__);

    return accept_amount_remaining;
  }

  // reduce accept_amount_remaining and return "true" if the customer is fully satisfied (nothing desired to be purchased)
  bool reduceAcceptAmountRemaining_andIsZero(const uint64_t really_purchased)
  {
  bool bRet = false;

    if (really_purchased >= accept_amount_remaining) bRet = true;

    accept_amount_remaining -= really_purchased;

    return bRet;
  }

  void saveAccept(ofstream &file, SHA256_CTX *shaCtx, string const &addr, string const &buyer ) const {
    // compose the outputline
    // seller-address, currency, buyer-address, amount, fee, block
    string lineOut = (boost::format("%s,%d,%s,%d,%d,%d,%d,%d,%d,%s")
      % addr
      % currency
      % buyer
      % block
      % accept_amount_remaining
      % accept_amount_original
      % (int)blocktimelimit
      % offer_amount_original
      % BTC_desired_original
      % offer_txid.ToString()).str();

    // add the line to the hash
    SHA256_Update(shaCtx, lineOut.c_str(), lineOut.length());

    // write the line
    file << lineOut << endl;
  }

};  // end of CMPAccept class

// a metadex trade
// TODO
// ...
class CMPMetaDEx
{
private:
  int block;
  uint256 txid;
  unsigned int idx; // index within the block
  unsigned int currency;
  uint64_t amount_original; // the amount for sale specified when the offer was placed
  unsigned int desired_currency;
  uint64_t desired_amount_original;
  unsigned char subaction;

  // price in 2 parts
  uint64_t  price_int;
  uint64_t  price_frac;

  // inverse price in 2 parts
  uint64_t  inverse_int;
  uint64_t  inverse_frac;

  string    addr;
  bool      bSell;  // selling Property for MSC: true or false

public:
  uint256 getHash() const { return txid; }
  unsigned int getCurrency() const { return currency; }

  unsigned int getDesCurrency() const { return desired_currency; }
  string getAddr() const { return addr; }
  uint64_t getAmtOrig() const { return amount_original; }
  uint64_t getAmtDes() const { return desired_amount_original; }
  unsigned char getAction() const { return subaction; }

  int getBlock() const { return block; }
  unsigned int getIdx() const { return idx; } 

  uint64_t* getPrice() const { 
    uint64_t* pricedata = new uint64_t[2];
    pricedata[0] = price_int;
    pricedata[1] = price_frac;
    return pricedata; 
  }
  uint64_t* getInversePrice() const { 
    uint64_t* pricedata = new uint64_t[2];
    pricedata[0] = inverse_int;
    pricedata[1] = inverse_frac;
    return pricedata; 
  }

  CMPMetaDEx(const string &, int, unsigned int, uint64_t, unsigned int, uint64_t, const uint256 &, unsigned int);
//  CMPMetaDEx(const string &, int, unsigned int, uint64_t, unsigned int, uint64_t, const uint256 &, unsigned int, uint64_t, uint64_t, uint64_t, uint64_t);

  void Set0(const string &, int, unsigned int, uint64_t, unsigned int, uint64_t, const uint256 &, unsigned int);

  void Set(uint64_t, uint64_t);
  void Set(uint64_t, uint64_t, uint64_t, uint64_t);

  std::string ToString() const;

  uint64_t getPriceInt() { return price_int; }
  uint64_t getPriceFrac() { return price_frac; }
};

unsigned int eraseExpiredAccepts(int blockNow);


namespace mastercore
{
typedef std::map<string, CMPOffer> OfferMap;
typedef std::map<string, CMPAccept> AcceptMap;
typedef std::map<string, CMPMetaDEx> MetaDExMap;

extern OfferMap my_offers;
extern AcceptMap my_accepts;

extern MetaDExMap metadex;

typedef std::pair < uint64_t, uint64_t > MetaDExTypePrice; // the price split up into integer & fractional part for precision

class mmap_compare
{
public:

  bool operator()(const MetaDExTypePrice &lhs, const MetaDExTypePrice &rhs) const;
};

class MetaDEx_compare
{
public:

  bool operator()(const CMPMetaDEx &lhs, const CMPMetaDEx &rhs) const;
};

typedef std::multimap < MetaDExTypePrice , CMPMetaDEx > MetaDExTypeMMap;
// typedef std::multimap < MetaDExTypePrice , CMPMetaDEx , mmap_compare > MetaDExTypeMMap;
// typedef std::multiset < pair < MetaDExTypePrice , CMPMetaDEx > > MetaDExTypeMSet;
typedef std::set < std::string > MetaDExTypeUniq;
typedef std::pair < MetaDExTypeMMap, MetaDExTypeUniq > MetaDExTypePair;
// typedef std::pair < MetaDExTypeMSet, MetaDExTypeUniq > MetaDExTypePair;
typedef std::map < unsigned int, MetaDExTypePair > MetaDExTypeMap;  // uniq primary key = currency

// ---------------
typedef std::set < CMPMetaDEx , MetaDEx_compare > md_Indexes; // set of objects sorted by block+idx
// TODO: replace double with float512 or float1024 // FIXME hitting the limit on trading 1 Satoshi for 100 BTC !!!
typedef std::map < double , md_Indexes > md_Prices;         // map of prices; there is a set of sorted objects for each price
typedef std::map < unsigned int, md_Prices > md_Currencies; // map of currencies; there is a map of prices for each currency
// ---------------

bool DEx_offerExists(const string &seller_addr, unsigned int curr);
CMPOffer *DEx_getOffer(const string &seller_addr, unsigned int curr);
CMPAccept *DEx_getAccept(const string &seller_addr, unsigned int curr, const string &buyer_addr);
int DEx_offerCreate(string seller_addr, unsigned int curr, uint64_t nValue, int block, uint64_t amount_desired, uint64_t fee, unsigned char btl, const uint256 &txid, uint64_t *nAmended = NULL);
int DEx_offerDestroy(const string &seller_addr, unsigned int curr);
int DEx_offerUpdate(const string &seller_addr, unsigned int curr, uint64_t nValue, int block, uint64_t desired, uint64_t fee, unsigned char btl, const uint256 &txid, uint64_t *nAmended = NULL);
int DEx_acceptCreate(const string &buyer, const string &seller, int curr, uint64_t nValue, int block, uint64_t fee_paid, uint64_t *nAmended = NULL);
int DEx_acceptDestroy(const string &buyer, const string &seller, int curr, bool bForceErase = false);
int DEx_payment(uint256 txid, unsigned int vout, string seller, string buyer, uint64_t BTC_paid, int blockNow, uint64_t *nAmended = NULL);

CMPMetaDEx *getMetaDEx(const string &sender_addr, unsigned int curr);

int MetaDEx_Trade(const string &customer, unsigned int currency, unsigned int currency_desired, uint64_t amount_desired, uint64_t price_int, uint64_t price_frac);
int MetaDEx_Phase1(const string &addr, unsigned int property, bool bSell, const uint256 &txid, unsigned int idx);
int MetaDEx_Create(const string &sender_addr, unsigned int curr, uint64_t amount, int block, unsigned int currency_desired, uint64_t amount_desired, const uint256 &txid, unsigned int idx);
int MetaDEx_Destroy(const string &sender_addr, unsigned int curr);
int MetaDEx_Update(const string &sender_addr, unsigned int curr, uint64_t nValue, int block, unsigned int currency_desired, uint64_t amount_desired, const uint256 &txid, unsigned int idx);

void MetaDEx_debug_print();
void MetaDEx_debug_print3();
}

#endif // #ifndef _MASTERCOIN_DEX

