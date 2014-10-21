#include "mastercore_parse_string.h"

#include <stdint.h>
#include <algorithm>
#include <string>

#include <boost/lexical_cast.hpp>

namespace mastercore
{
int64_t strToInt64(std::string strAmount, bool divisible)
{
  int64_t Amount = 0;

  //check for a negative (minus sign) and invalidate if present
  size_t negSignPos = strAmount.find("-");
  if (negSignPos!=std::string::npos) return 0;

  //convert the string into a usable int64
  if (divisible)
  {
      //check for existance of decimal point
      size_t pos = strAmount.find(".");
      if (pos==std::string::npos)
      { //no decimal point but divisible so pad 8 zeros on right
          strAmount+="00000000";
      }
      else
      {
          size_t posSecond = strAmount.find(".", pos+1); //check for existence of second decimal point, if so invalidate amount
          if (posSecond!=std::string::npos) return 0;
          if ((strAmount.size()-pos)<9)
          { //there are decimals either exact or not enough, pad as needed
              std::string strRightOfDecimal = strAmount.substr(pos+1);
              unsigned int zerosToPad = 8-strRightOfDecimal.size();
              if (zerosToPad>0) //do we need to pad?
              {
                  for(unsigned int it = 0; it != zerosToPad; it++)
                  {
                      strAmount+="0";
                  }
              }
          }
          else
          { //there are too many decimals, truncate after 8
              strAmount = strAmount.substr(0,pos+9);
          }
      }
      strAmount.erase(std::remove(strAmount.begin(), strAmount.end(), '.'), strAmount.end());
      try { Amount = boost::lexical_cast<int64_t>(strAmount); } catch(const boost::bad_lexical_cast &e) { }
  }
  else
  {
      size_t pos = strAmount.find(".");
      std::string newStrAmount = strAmount.substr(0,pos);
      try { Amount = boost::lexical_cast<int64_t>(newStrAmount); } catch(const boost::bad_lexical_cast &e) { }
  }
return Amount;
}

} // namespace mastercore
