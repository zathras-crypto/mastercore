// global Master Protocol header file
// globals consts (DEFINEs for now) should be here
//
// for now (?) class declarations go here -- work in progress; probably will get pulled out into a separate file, note: some declarations are still in the .cpp file

#ifndef _MASTERCOIN
#define _MASTERCOIN 1

#include "netbase.h"
#include "protocol.h"

// Master Protocol Transaction (Packet) Version
#define MP_TX_PKT_V0  0
#define MP_TX_PKT_V1  1

// Maximum outputs per BTC Transaction
#define MAX_BTC_OUTPUTS 16

// TODO: clean up is needed for pre-production #DEFINEs , consts & alike belong in header files (classes)
#define MAX_SHA256_OBFUSCATION_TIMES  255

#define PACKET_SIZE_CLASS_A 19
#define PACKET_SIZE         31
#define MAX_PACKETS         5

#define MSC_TYPE_SIMPLE_SEND              0
#define MSC_TYPE_TRADE_OFFER              20
#define MSC_TYPE_ACCEPT_OFFER_BTC         22
#define MSC_TYPE_CREATE_PROPERTY_FIXED    50
#define MSC_TYPE_CREATE_PROPERTY_VARIABLE 51
#define MSC_TYPE_PROMOTE_PROPERTY         52

#define FILETYPE_BALANCES 0
#define FILETYPE_OFFERS   1
#define FILETYPE_ACCEPTS  2
const char mastercoin_filenames[][128]={
"mastercoin_balances.txt",
"mastercoin_offers.txt",
"mastercoin_accepts.txt"
};

#define MASTERCOIN_CURRENCY_MSC   1
#define MASTERCOIN_CURRENCY_TMSC  2
#define MASTERCOIN_CURRENCY_SP1   3

#define MSC_MAX_KNOWN_CURRENCIES  4

inline uint64_t rounduint64(double d)
{
    return (uint64_t)(abs(0.5 + d));
}

extern CCriticalSection cs_tally;
extern char *c_strMastercoinCurrency(int i);

class msc_tally
{

private:
  int64_t moneys[MSC_MAX_KNOWN_CURRENCIES];
  int64_t reserved[MSC_MAX_KNOWN_CURRENCIES];

public:

  bool msc_update_moneys(unsigned char which, int64_t amount)  
  {
  LOCK(cs_tally);

    if (MSC_MAX_KNOWN_CURRENCIES > which)
    {
      // check here if enough money is available for this address prior to update !!!
      if (0>(moneys[which] + amount))
      {
        printf("%s(); FUNDS AVAILABLE: ONLY= %lu (INSUFFICIENT), line %d, file: %s\n", __FUNCTION__, moneys[which], __LINE__, __FILE__);
        return false;
      }
      moneys[which] += amount;
      return true;
    }

    return false;
  }

  bool msc_update_reserved(unsigned char which, int64_t amount)  
  {
  LOCK(cs_tally);

    if (MSC_MAX_KNOWN_CURRENCIES > which)
    {
      // check here if enough money is available for this address prior to update !!!
      if (0>(reserved[which] + amount))
      {
        printf("%s(); FUNDS AVAILABLE: ONLY= %lu (INSUFFICIENT), line %d, file: %s\n", __FUNCTION__, reserved[which], __LINE__, __FILE__);
        return false;
      }
      reserved[which] += amount;
      return true;
    }

    return false;
  }

  // the constructor
  msc_tally(unsigned char which, int64_t amount)
  {
    for (unsigned int i=0;i<MSC_MAX_KNOWN_CURRENCIES;i++)
    {
      moneys[i] = 0;
      reserved[i] = 0;
    }

    (void) msc_update_moneys(which, amount);
  }

  void print()
  {
    for (unsigned int i=1;i<MSC_MAX_KNOWN_CURRENCIES;i++) // not keeping track of BTC amounts, index=0.. for now?
    {
//      printf("%s = %+15.8lf [reserved= %+15.8lf] ", c_strMastercoinCurrency(i), (double)moneys[i]/(double)COIN, (double)reserved[i]/(double)COIN);
      // just print the money, not the name, getting too wide for my screen
      printf("%+15.8lf [reserved= %+15.8lf] ", (double)moneys[i]/(double)COIN, (double)reserved[i]/(double)COIN);
    }
    printf("\n");
  }

  string getMSC();  // this function was created for QT only -- hard-coded internally, TODO: use getMoney()
  string getTMSC(); // this function was created for QT only -- hard-coded internally, TODO: use getMoney()

  uint64_t getMoney(unsigned which_currency, bool bReserved) const
  {
    if (MSC_MAX_KNOWN_CURRENCIES <= which_currency) return 0;

    if (!bReserved) return moneys[which_currency];
    return reserved[which_currency];
  }
};

extern map<string, msc_tally> msc_tally_map;

#endif

