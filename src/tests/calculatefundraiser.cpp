#include <boost/multiprecision/cpp_int.hpp>
#include <inttypes.h>
#include <utility>
#include <map>
#include <string>
#include <stdio.h>
#include <limits>

using boost::multiprecision::int128_t;
using boost::multiprecision::cpp_int;
#define MAX_INT_8_BYTES (9223372036854775807UL) 

// returns false if we are out of range and/or overflow
// call just before multiplying large numbers
bool isMultOK(const uint64_t a, const uint64_t b)
{
  printf("\n%s(%lu, %lu): \n", __FUNCTION__, a, b);

  if (!a || !b) return true;

  if (MAX_INT_8_BYTES < a) return false;
  if (MAX_INT_8_BYTES < b) return false;

  const uint64_t result = a*b;

  if (MAX_INT_8_BYTES < result) return false;

  if ((0 != a) && (result / a != b)) return false;

  return true;
}

// calculates and returns fundraiser bonus, issuer premine, and total tokens
// propType : divisible/indiv
// bonusPerc: bonus percentage
// currentSecs: number of seconds of current tx
// numProps: number of properties
// issuerPerc: percentage of tokens to issuer
int calculateFractional(int propType, int bonusPerc, long long int fundraiserSecs, int64_t numProps, int issuerPerc,  const std::map<std::string, std::pair<uint64_t, uint64_t> > database, const long long int amountPremined  )
{

  long double totalCreated = 0;
  double issuerPercentage = (double) (issuerPerc * 0.01);

  std::map<std::string, std::pair<uint64_t, uint64_t> >::const_iterator it;

  for(it = database.begin(); it != database.end(); it++) {

    //printf("\n\ndoing... \n");
    long long int currentSecs = it->second.second;
    long double amtTransfer = it->second.first;

    long long int bonusSeconds = fundraiserSecs - currentSecs;
  
    long double weeks = bonusSeconds / (double) 604800;
    double ebPercentage = weeks * bonusPerc;
    long double bonusPercentage = ( ebPercentage / 100 ) + 1;
  
    long double createdTokens;
    long double issuerTokens;

    if( 2 == propType ) {
      createdTokens = amtTransfer * (long double) numProps * bonusPercentage ;
      issuerTokens = createdTokens * issuerPercentage;
      //printf("prop 2: is %Lf, and %Lf \n", createdTokens, issuerTokens);

      totalCreated += createdTokens;
    } else {
      //printf("amount xfer %Lf and props %f and bonus percs %Lf \n", amtTransfer, (double) numProps, bonusPercentage);
      createdTokens = (long long int) (  (amtTransfer/1e9) * (long double) numProps * bonusPercentage);
      issuerTokens = (long long int) (createdTokens * issuerPercentage) ;
      //printf("prop 1: is %ld, and %ld \n", (uint64_t) createdTokens, (uint64_t) issuerTokens);

      //printf("\nWHOLES 1: is %lld, and %lld \n", (long long int) (createdTokens / 1e9), (long long int) (issuerTokens / 1e9 ));
      totalCreated += createdTokens;
    }
    //printf("did it %s \n ", it->first.c_str());
  };
  

  long double totalPremined = totalCreated * issuerPercentage;
  long double missedTokens;
  
  printf("TOTAL C %ld, %ld, %ld", (uint64_t) totalCreated, (uint64_t) totalPremined, (uint64_t) (totalCreated + totalPremined)); 
  if( 2 == propType ) {
    missedTokens = totalPremined - amountPremined;
  } else {
    missedTokens = (long long int) (totalPremined - amountPremined);
  }

  //printf("\ntotal toks %Lf and total missed %Lf and total premined %Lf and premined %lld \n ", totalCreated, missedTokens, totalPremined, amountPremined );

  return missedTokens;
}

std::string p128(int128_t quantity)
{
    //printf("\nTest # was %s\n", boost::lexical_cast<std::string>(quantity).c_str() );
   return boost::lexical_cast<std::string>(quantity);
}

std::string p_arb(cpp_int quantity)
{
    //printf("\nTest # was %s\n", boost::lexical_cast<std::string>(quantity).c_str() );
   return boost::lexical_cast<std::string>(quantity);
}
void calculateFundraiser(unsigned short int propType, uint64_t amtTransfer, unsigned char bonusPerc, uint64_t fundraiserSecs, uint64_t currentSecs, uint64_t numProps, unsigned char issuerPerc, uint64_t totalTokens, std::pair<uint64_t, uint64_t >& tokens )
{
  uint64_t weeks_sec = 604800;
  int128_t weeks_sec_ = 604800L;
  //define weeks in seconds
  int128_t precision_ = 1000000000000L;
  //define precision for all non-bitcoin values (bonus percentages, for example)
  int128_t percentage_precision = 100L;
  //define precision for all percentages (10/100 = 10%)

  uint64_t bonusSeconds = fundraiserSecs - currentSecs;
  //calcluate the bonusseconds

  int128_t bonusSeconds_ = fundraiserSecs - currentSecs;

  double weeks_d = bonusSeconds / (double) weeks_sec;
  //debugging
  
  int128_t weeks_ = (bonusSeconds_ / weeks_sec_) * precision_ + ( (bonusSeconds_ % weeks_sec_ ) * precision_) / weeks_sec_;
  //calculate the whole number of weeks to apply bonus

  printf("\n weeks_d: %.8lf \n weeks: %s + (%s / %s) =~ %.8lf \n", weeks_d, p128(bonusSeconds_ / weeks_sec_).c_str(), p128(bonusSeconds_ % weeks_sec_).c_str(), p128(weeks_sec_).c_str(), boost::lexical_cast<double>(bonusSeconds_ / weeks_sec_) + boost::lexical_cast<double> (bonusSeconds_ % weeks_sec_) / boost::lexical_cast<double>(weeks_sec_) );
  //debugging lines

  double ebPercentage_d = weeks_d * bonusPerc;
  //debugging lines

  int128_t ebPercentage_ = weeks_ * bonusPerc;
  //calculate the earlybird percentage to be applied

  printf("\n ebPercentage_d: %.8lf \n ebPercentage: %s + (%s / %s ) =~ %.8lf \n", ebPercentage_d, p128(ebPercentage_ / precision_).c_str(), p128( (ebPercentage_) % precision_).c_str() , p128(precision_).c_str(), boost::lexical_cast<double>(ebPercentage_ / precision_) + boost::lexical_cast<double>(ebPercentage_ % precision_) / boost::lexical_cast<double>(precision_));
  //debugging
  
  double bonusPercentage_d = ( ebPercentage_d / 100 ) + 1;
  //debugging

  int128_t bonusPercentage_ = (ebPercentage_ + (precision_ * percentage_precision) ) / percentage_precision; 
  //calcluate the bonus percentage to apply up to 'percentage_precision' number of digits

  printf("\n bonusPercentage_d: %.18lf \n bonusPercentage: %s + (%s / %s) =~ %.11lf \n", bonusPercentage_d, p128(bonusPercentage_ / precision_).c_str(), p128(bonusPercentage_ % precision_).c_str(), p128(precision_).c_str(), boost::lexical_cast<double>(bonusPercentage_ / precision_) + boost::lexical_cast<double>(bonusPercentage_ % precision_) / boost::lexical_cast<double>(precision_));
  //debugging

  double issuerPercentage_d = (double) (issuerPerc * 0.01);
  //debugging

  int128_t issuerPercentage_ = (int128_t)issuerPerc * precision_ / percentage_precision;

  printf("\n issuerPercentage_d: %.8lf \n issuerPercentage: %s + (%s / %s) =~ %.8lf \n", issuerPercentage_d, p128(issuerPercentage_ / precision_ ).c_str(), p128(issuerPercentage_ % precision_).c_str(), p128( precision_ ).c_str(), boost::lexical_cast<double>(issuerPercentage_ / precision_) + boost::lexical_cast<double>(issuerPercentage_ % precision_) / boost::lexical_cast<double>(precision_));
  //debugging

  int128_t satoshi_precision_ = 100000000;
  //define the precision for bitcoin amounts (satoshi)
  //uint64_t createdTokens, createdTokens_decimal;
  //declare used variables for total created tokens

  //uint64_t issuerTokens, issuerTokens_decimal;
  //declare used variables for total issuer tokens

  printf("\n NUMBER OF PROPERTIES %ld", numProps); 
  printf("\n AMOUNT INVESTED: %ld BONUS PERCENTAGE: %.11f and %s", amtTransfer,bonusPercentage_d, p128(bonusPercentage_).c_str());
  
  long double ct = ((amtTransfer/1e8) * (long double) numProps * bonusPercentage_d);


  int128_t createdTokens_ =   (int128_t)amtTransfer*(int128_t)numProps*bonusPercentage_ ;

  cpp_int createdTokens =  (int128_t)amtTransfer*(int128_t)numProps*bonusPercentage_ ;
  printf("\n created tokens %s and max %s and \n max %s\n  ", p_arb(createdTokens).c_str(), p128(std::numeric_limits<int128_t>::max()).c_str(), createdTokens_ > std::numeric_limits<int128_t>::max(), p_arb(std::numeric_limits<cpp_int>::max()).c_str() );

  printf("\n CREATED TOKENS %.8Lf, %s + (%s / %s) ~= %.8lf",ct, p128(createdTokens_ / (precision_ * satoshi_precision_) ).c_str(), p128(createdTokens_ % (precision_ * satoshi_precision_) ).c_str() , p128( precision_*satoshi_precision_ ).c_str(), boost::lexical_cast<double>(createdTokens_ / (precision_ * satoshi_precision_) ) + boost::lexical_cast<double>(createdTokens_ % (precision_ * satoshi_precision_)) / boost::lexical_cast<double>(precision_*satoshi_precision_));
  //TODO overflow checks  

  long double it = (uint64_t) ct * issuerPercentage_d;

  int128_t issuerTokens_ = (createdTokens_ / (satoshi_precision_ * precision_ )) * (issuerPercentage_ / 100) * precision_;
  
  printf("\n ISSUER TOKENS: %.8Lf, %s + (%s / %s ) ~= %.8lf \n",it, p128(issuerTokens_ / (precision_ * satoshi_precision_ * 100 ) ).c_str(), p128( issuerTokens_ % (precision_ * satoshi_precision_ * 100 ) ).c_str(), p128(precision_*satoshi_precision_*100).c_str(), boost::lexical_cast<double>(issuerTokens_ / (precision_ * satoshi_precision_ * 100))  + boost::lexical_cast<double>(issuerTokens_ % (satoshi_precision_*precision_*100) )/ boost::lexical_cast<double>(satoshi_precision_*precision_*100)); 
  
  //total tokens including remainders
  //printf("\n DIVISIBLE TOKENS (UI LAYER) CREATED: is ~= %.8lf, and %.8lf\n",(double)createdTokens + (double)createdTokens_decimal/(satoshi_precision *precision), (double) issuerTokens + (double)issuerTokens_decimal/(satoshi_precision*precision*percentage_precision) );
  //if (2 == propType)
    //printf("\n DIVISIBLE TOKENS (UI LAYER) CREATED: is ~= %.8lf, and %.8lf\n", (uint64_t) (boost::lexical_cast<double>(createdTokens_ / (precision_ * satoshi_precision_) ) + boost::lexical_cast<double>(createdTokens_ % (precision_ * satoshi_precision_)) / boost::lexical_cast<double>(precision_*satoshi_precision_) )/1e8, (uint64_t) (boost::lexical_cast<double>(issuerTokens_ / (precision_ * satoshi_precision_ * 100))  + boost::lexical_cast<double>(issuerTokens_ % (satoshi_precision_*precision_*100) )/ boost::lexical_cast<double>(satoshi_precision_*precision_*100)) / 1e8  );
  //else
    //printf("\n INDIVISIBLE TOKENS (UI LAYER) CREATED: is = %lu, and %lu\n", boost::lexical_cast<uint64_t>(createdTokens_ / (precision_ * satoshi_precision_ ) ), boost::lexical_cast<uint64_t>(issuerTokens_ / (precision_ * satoshi_precision_ * 100)));
  
  int128_t totalCreated = totalTokens + (createdTokens_ / (precision_ * satoshi_precision_) ) + (issuerTokens_ / (precision_ * satoshi_precision_ * 100 ) );
  if ( totalCreated > MAX_INT_8_BYTES) {
    printf("\n overflow %s \n", p128(totalCreated).c_str());
    int128_t maxCreatable = MAX_INT_8_BYTES - totalTokens;
    
    int128_t newIssuerTokens;
    if (issuerTokens_ != 0) {
        newIssuerTokens = ( maxCreatable / ( 100 + issuerPercentage_ ) ) * issuerPercentage_; }
    
    printf("\n new issuer tokens %s and user Tokens %s\n",p128(newIssuerTokens).c_str(), p128(maxCreatable - newIssuerTokens).c_str() );
    //close crowdsale
  }

  printf("\n no overflow %s \n", p128(totalCreated).c_str());
  tokens = std::make_pair(boost::lexical_cast<uint64_t>(createdTokens_ / (precision_ * satoshi_precision_ ) ), boost::lexical_cast<uint64_t>(issuerTokens_ / (precision_ * satoshi_precision_ * 100)));
}

int main() {

   long long int amountCreated = 0;
   long long int amountPremined = 0;
   
   std::map<std::string, std::pair<uint64_t, uint64_t> > database;
   std::pair <uint64_t, uint64_t> tokens;

   printf("\n div funding div,  ");
   calculateFundraiser(2, 30 * 1e8,6,1407064860000,1407877014,31337 * 1e8,10, 0, tokens);
   amountCreated += tokens.first; amountPremined += tokens.second;
   printf(" HASH: 8fbd96005aba5671daf8288f89df8026a7ce4782a0bb411937537933956b827b \n");
   printf("\nTokens created, Tokens for issuer: %lld %lld\n", tokens.first, tokens.second);
   
  /*
   printf("\n div funding indiv,  ");
   calculateFundraiser(1, 0.0001 * 1e8,28,1406925000000,1405610834,8686,68, 0, tokens);
   amountCreated += tokens.first; amountPremined += tokens.second;
   printf(" HASH: 8b797b75518cc7b7da8807e25ec9c62ad59644b871106d4fbc08a989f74716f6 \n");
   printf("\nTokens created, Tokens for issuer: %lld %lld\n", tokens.first, tokens.second);

   printf("\n should get SPT back");
   calculateFundraiser(1, (0.0001 * 1e8),28,1406925000000,1397841991,8686,68, tokens);
   amountCreated += tokens.first; amountPremined += tokens.second;
   printf(" HASH: 637a4c09e5deafe31533ba8eff1faa7b68a55c6cdf734d7735095823099af9fe\n");
   //printf("\nTokens created, Tokens for issuer: %lld %lld\n", tokens.first, tokens.second);
   

   printf("\n should get 5173.93687812 SPT back");
   calculateFundraiser(2, (2.133 * 1e8),10,1400423640,1397841991,170000000000,10, tokens);
   amountCreated += tokens.first; amountPremined += tokens.second;
   printf(" HASH: 637a4c09e5deafe31533ba8eff1faa7b68a55c6cdf734d7735095823099af9fe\n");
   //printf("\nTokens created, Tokens for issuer: %lld %lld\n", tokens.first, tokens.second);
   

   printf("\n funding prop 2 with prop 1 case 4... should be 1e8 :  Indivisible to Divisible (1 given -> 100000000/token) ");
   calculateFundraiser(2, (0.00000001 * 1e8) * 1e8,0,1406967060,1399591823,10000000000000000,100, tokens);
   amountCreated += tokens.first; amountPremined += tokens.second;
   printf(" HASH: c7591e401f9eb58f68d0a30150953fd4ae3ee566ceba00d99dded5b41c14b9bd\n");
   printf("\nTokens created, Tokens for issuer: %lld %lld\n", tokens.first, tokens.second);
     

   calculateFundraiser(1,(2.00000000 * 1e8),0,1398701578,1398371487,817545361,0.0, tokens);
   printf("\n262ab5f05b823c77ee7af8cb5ea9ce7ebbc0c34775a7bbeb7c3e477a4881dc89\n");
   //Expected: 'total created', 1635090722, 'tokens for issuer', 0.0
   //Returned: 1635090722, and 0.000000 
 */

   //calculateFundraiser(2,739038774,25,22453142409904,1403765616,100,10, tokens);
   //printf("\n333d8fd459b270fde95736846eb81b2547837476f33e8e0b4c1158906870155f\n");
   //Expected: 'total created', 6858757932.260923, 'tokens for issuer', 685875793.2260923
   //Returned: 6858757932.260923, and 685875793.226092
   
   //calculateFundraiser(1,2.0 * 1e8,10,1396500000,1396067389,1500,5, tokens);
   //printf("\n9f8f19ee4dbc6eb23905c6416053a651259a22c88be0e55e61454909d20ce66d\n");
   //Expected: 'total created', 3214, 'tokens for issuer', 160
   //Returned: 3214, and 160
   
   //calculateFundraiser(1,96.0 * 1e8,10,1400198400,1398292763,1700,42, tokens);
   //printf("\n94d23c01d82ffb8b7fa1c25a919a96713afa1198ce02a06c1bd6713af6a6d97a\n");
   //Expected: 'total created', 214621, 'tokens for issuer', 90140
   //Returned: 214621, and 90140
   /*
   printf("\n\n Correct case 1... Divisible to Divisible (0.3 given -> 50/token)");
   calculateFundraiser(2, 0.3 * 1e8,5,1409270940,1406684330,5000000000,50, tokens);
   amountCreated += tokens.first; amountPremined += tokens.second;
   printf(" HASH: 8b051b9b50123de6f097e8b4d0fdc5de8efc9309b68c0438ce385b5dab2925df\n");
   printf("\nTokens created, Tokens for issuer: %lld %lld\n", tokens.first, tokens.second);
   
   printf("\n Correct case 2...Divisible to Divisible (0.5 given -> 3400/token) ");
   calculateFundraiser(2, 0.5 * 1e8,10,1400544000,  1398109713,340000000000,0, tokens);
   amountCreated += tokens.first; amountPremined += tokens.second;
   printf(" HASH: 13f97d772f615007b2871dc40b8b9fae6cb75a133596e470558097436f556d19\n");
   printf("\nTokens created, Tokens for issuer: %lld %lld\n", tokens.first, tokens.second);
   
   printf("\n Correct case 3... Divisible to Divisible (0.1 given -> 50/token) ");
   calculateFundraiser(2, 0.1 * 1e8,5,1409270940, 1406840780,5000000000,50, tokens);
   amountCreated += tokens.first; amountPremined += tokens.second;
   printf(" HASH: d4116a1d3830fd74fc66d026a532e1df53c2b7aa4bbe1855d56b2d866dc92c11\n");
   printf("\nTokens created, Tokens for issuer: %lld %lld\n", tokens.first, tokens.second);
   
   printf("\n funding prop 1 with prop 2 case 3... should be 1 token : Divisible to Indivisible (1 given -> 1/token)");
   calculateFundraiser(1, (1.000 * 1e8),0,1407888180,1399591823,1,100, tokens);
   amountCreated += tokens.first; amountPremined += tokens.second;
   printf(" HASH: No hash, contrived example \n");
   printf("\nTokens created, Tokens for issuer: %lld %lld\n", tokens.first, tokens.second);

   printf("\n funding prop 1 with prop 1 case 3.5 ... should be 1 token : Indivisible to Indivisible (50 given -> 1/token) ");
   calculateFundraiser(1, 50 * 1e8,0,1407888180,1399591823,1,100, tokens);
   amountCreated += tokens.first; amountPremined += tokens.second;
   printf(" HASH: No hash, contrived example  \n");
   printf("\nTokens created, Tokens for issuer: %lld %lld\n", tokens.first, tokens.second);
  
  //if funding property 2 and sending property 1, multiply the sending property by 1e8
  //if funding property 1 and sending property 2, divide the sending property by 1e8
   */
   /*printf("\n negative tokens? ");
   calculateFundraiser(2, (30.00000000 * 1e8),6,1407064860000,1407877014,3133700000000,10, tokens);
   amountCreated += tokens.first; amountPremined += tokens.second;
   printf(" HASH: c7591e401f9eb58f68d0a30150953fd4ae3ee566ceba00d99dded5b41c14b9bd\n");
   //printf("\nTokens created, Tokens for issuer: %lld %lld\n", tokens.first, tokens.second);
   */
   
   
   
   //calculate fractional test
   /*
   long long int e1[] = { (long long int) (42.0 * 1e9 ), 1397758429};
   std::vector<long long int> entry1(e1, e1 + sizeof(e1));

   long long int e2[] = { (long long int) (2.0 * 1e9 ), 1398103687};
   std::vector<long long int> entry2(e2, e2 + sizeof(e2));
   
   long long int e3[] = { (long long int) (2.0 * 1e9 ), 1398138271};
   std::vector<long long int> entry3(e3, e3 + sizeof(e3));

   long long int e4[] = { (long long int) (96.0 * 1e9 ), 1398292763};
   std::vector<long long int> entry4(e4, e4 + sizeof(e4));
   
   long long int e5[] = { (long long int) (1.0 * 1e9 ), 1398790874};
   std::vector<long long int> entry5(e5, e5 + sizeof(e5));

   long long int e6[] = { (long long int) (55.23485377 * 1e9 ), 1399591823};
   std::vector<long long int> entry6(e6, e6 + sizeof(e6));
    */
   /*
   database.insert(std::make_pair("9578e55f53acaac49bb691efd7f7f5fac7b13d148a73222fc986932da3126662",std::make_pair(42.0 * 1e9, 1397758429)));
   database.insert(std::make_pair("9a206f4caea9ce9392432763285adfdb6feab75b8d6e6b290aa87bdde91e54ba",std::make_pair(2.0 * 1e9, 1398103687)));
   database.insert(std::make_pair("68859a806d7e453c097cdad061a80c90c6a8eb17f3dc0a147255c69e4f2878f4",std::make_pair(2.0 * 1e9, 1398138271)));
   database.insert(std::make_pair("c404bedee25b7d3dc790280d2913c2092c976df26fe2db4c2b08dc9eb12bedd6",std::make_pair(96.0 * 1e9, 1398292763)));
   database.insert(std::make_pair("45acd766a41d4418963470cc10f8ec76e63f4db0d30a258815e3dbc1306ebc0b",std::make_pair(1.0 * 1e9, 1398790874)));
   database.insert(std::make_pair("77030cc6217c7555d056566c99a46178215d0254e9567445b615e71b1838e11c",std::make_pair(55.23485377 * 1e9, 1399591823)));

   double missedTokens = calculateFractional(1,10,1400198400,1700,42, database, amountPremined);

   printf("\nTotal tokens, Tokens created, Tokens for issuer, amountMissed:%ld %ld %ld %f\n", amountCreated + amountPremined, amountCreated, amountPremined, missedTokens);*/
   
   return 0;
}

