//Tests for string to int64 conversion
//Build with g++ strtoint64.cpp -o main
//Execute with ./main and ensure all pass

#include <string>
#include <algorithm>
#include <stdlib.h>
#include <stdio.h>
#include <boost/lexical_cast.hpp>

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
          if ((strAmount.size()-pos)<8)
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

int main()
{
  int64_t amount = 0;
  std::string strAmount;
  int64_t expectedAmount = 0;

  printf("INDIVISIBLE TESTS:\n");

  //4000000000000000 - indivisible - big num
  strAmount = "4000000000000000";
  expectedAmount = 4000000000000000;
  amount = strToInt64(strAmount, false);
  printf("Amount detected: %lu  - ", amount);
  if (amount == expectedAmount) { printf("Pass converting from %s\n",strAmount.c_str()); } else { printf("Fail converting from %s\n",strAmount.c_str()); }

  //9223372036854775808 - indivisible - over max int64
  strAmount = "9223372036854775808";
  expectedAmount = 0;
  amount = strToInt64(strAmount, false);
  printf("Amount detected: %lu  - ", amount);
  if (amount == expectedAmount) { printf("Pass converting from %s\n",strAmount.c_str()); } else { printf("Fail converting from %s\n",strAmount.c_str()); }

  //9223372036854775807 - indivisible - max int64
  strAmount = "9223372036854775807";
  expectedAmount = 9223372036854775807;
  amount = strToInt64(strAmount, false);
  printf("Amount detected: %lu  - ", amount);
  if (amount == expectedAmount) { printf("Pass converting from %s\n",strAmount.c_str()); } else { printf("Fail converting from %s\n",strAmount.c_str()); }

  //-4 - indivisible - negative
  strAmount = "-4";
  expectedAmount = 0;
  amount = strToInt64(strAmount, false);
  printf("Amount detected: %lu  - ", amount);
  if (amount == expectedAmount) { printf("Pass converting from %s\n",strAmount.c_str()); } else { printf("Fail converting from %s\n",strAmount.c_str()); }

  //0 indivisible - zero amount
  strAmount = "0";
  expectedAmount = 0;
  amount = strToInt64(strAmount, true);
  printf("Amount detected: %lu  - ", amount);
  if (amount == expectedAmount) { printf("Pass converting from %s\n",strAmount.c_str()); } else { printf("Fail converting from %s\n",strAmount.c_str()); }

  printf("DIVISIBLE TESTS:\n");

  //0.00000004 - divisible - check padding
  strAmount = "0.00000004";
  expectedAmount = 4;
  amount = strToInt64(strAmount, true);
  printf("Amount detected: %lu  - ", amount);
  if (amount == expectedAmount) { printf("Pass converting from %s\n",strAmount.c_str()); } else { printf("Fail converting from %s\n",strAmount.c_str()); }

  //0.0004 - divisible - check padding
  strAmount = "0.0004";
  expectedAmount = 40000;
  amount = strToInt64(strAmount, true);
  printf("Amount detected: %lu  - ", amount);
  if (amount == expectedAmount) { printf("Pass converting from %s\n",strAmount.c_str()); } else { printf("Fail converting from %s\n",strAmount.c_str()); }

  //0.4 - divisible - check padding
  strAmount = "0.4";
  expectedAmount = 40000000;
  amount = strToInt64(strAmount, true);
  printf("Amount detected: %lu  - ", amount);
  if (amount == expectedAmount) { printf("Pass converting from %s\n",strAmount.c_str()); } else { printf("Fail converting from %s\n",strAmount.c_str()); }

  //4.0 - divisible - check padding
  strAmount = "4.0";
  expectedAmount = 400000000;
  amount = strToInt64(strAmount, true);
  printf("Amount detected: %lu  - ", amount);
  if (amount == expectedAmount) { printf("Pass converting from %s\n",strAmount.c_str()); } else { printf("Fail converting from %s\n",strAmount.c_str()); }

  //40.00000000000099 - divisible - over 8 digits
  strAmount = "40.00000000000099";
  expectedAmount = 4000000000;
  amount = strToInt64(strAmount, true);
  printf("Amount detected: %lu  - ", amount);
  if (amount == expectedAmount) { printf("Pass converting from %s\n",strAmount.c_str()); } else { printf("Fail converting from %s\n",strAmount.c_str()); }

  //-4.0 - divisible - negative
  strAmount = "-4.0";
  expectedAmount = 0;
  amount = strToInt64(strAmount, true);
  printf("Amount detected: %lu  - ", amount);
  if (amount == expectedAmount) { printf("Pass converting from %s\n",strAmount.c_str()); } else { printf("Fail converting from %s\n",strAmount.c_str()); }

  //92233720368.54775807 - divisible - max int64
  strAmount = "92233720368.54775807";
  expectedAmount = 9223372036854775807;
  amount = strToInt64(strAmount, true);
  printf("Amount detected: %lu  - ", amount);
  if (amount == expectedAmount) { printf("Pass converting from %s\n",strAmount.c_str()); } else { printf("Fail converting from %s\n",strAmount.c_str()); }

  //92233720368.54775808 - divisible - over max int64
  strAmount = "92233720368.54775808";
  expectedAmount = 0;
  amount = strToInt64(strAmount, true);
  printf("Amount detected: %lu  - ", amount);
  if (amount == expectedAmount) { printf("Pass converting from %s\n",strAmount.c_str()); } else { printf("Fail converting from %s\n",strAmount.c_str()); }

  //1234..12345678 - divisible - more than one decimal in string
  strAmount = "1234..12345678";
  expectedAmount = 0;
  amount = strToInt64(strAmount, true);
  printf("Amount detected: %lu  - ", amount);
  if (amount == expectedAmount) { printf("Pass converting from %s\n",strAmount.c_str()); } else { printf("Fail converting from %s\n",strAmount.c_str()); }

  //1234.12345A - divisible - alpha chars in string
  strAmount = "1234.12345A";
  expectedAmount = 0;
  amount = strToInt64(strAmount, true);
  printf("Amount detected: %lu  - ", amount);
  if (amount == expectedAmount) { printf("Pass converting from %s\n",strAmount.c_str()); } else { printf("Fail converting from %s\n",strAmount.c_str()); }

  //0 divisible - zero amount
  strAmount = "0.000";
  expectedAmount = 0;
  amount = strToInt64(strAmount, true);
  printf("Amount detected: %lu  - ", amount);
  if (amount == expectedAmount) { printf("Pass converting from %s\n",strAmount.c_str()); } else { printf("Fail converting from %s\n",strAmount.c_str()); }

}


