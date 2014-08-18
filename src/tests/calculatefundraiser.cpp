#include <inttypes.h>
#include <utility>
#include <map>
#include <string>
#include <stdio.h>

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


void calculateFundraiser(unsigned short int propType, uint64_t amtTransfer, unsigned char bonusPerc, uint64_t fundraiserSecs, uint64_t currentSecs, uint64_t numProps, unsigned char issuerPerc, std::pair<uint64_t, uint64_t >& tokens )
{
  
  uint64_t weeks_sec = 604800; 
  //define weeks in seconds
  uint64_t precision = 1000000000;
  //define precision for all non-bitcoin values (bonus percentages, for example)
  uint64_t percentage_precision = 100;
  //define precision for all percentages (10/100 = 10%)

  uint64_t bonusSeconds = fundraiserSecs - currentSecs;
  //calcluate the bonusseconds

  double weeks_d = bonusSeconds / (double) weeks_sec;
  //debugging
  
  uint64_t w_rem = bonusSeconds % weeks_sec;
  //calculate the remainder part for number of weeks to apply bonus 
  uint64_t w_decimal = precision * w_rem / weeks_sec;
  //calcuate the remainder part up to 'precision' number of digits
  uint64_t weeks = bonusSeconds / weeks_sec; 
  //calculate the whole number of weeks to apply bonus

  printf("\n weeks_d: %.8lf \n weeks: %lu + (%lu / %lu) =~ %.8lf \n", weeks_d, weeks, w_decimal, precision, weeks + ((double)w_decimal/precision) );
  //debugging lines

  double ebPercentage_d = weeks_d * bonusPerc;
  //debugging lines
  
  uint64_t ebperc_wrem = (w_rem * bonusPerc) / weeks_sec;
  //calculate the earlybird bonus remainder (this might have a whole part, which is stored here)
  uint64_t ebperc_wrem_rem = (w_rem * bonusPerc) % weeks_sec;
  //calcluate the earlybird bonus remainder (actual remainder part)
  uint64_t ebperc_wrem_decimal = (precision * ebperc_wrem_rem) / weeks_sec;
  //calcualate the earlybird bonus remainder with 'precision' number of digits

  uint64_t ebPercentage = weeks * bonusPerc + ebperc_wrem;
  //calculate the earlybird percentage to be applied

  printf("\n ebPercentage_d: %.8lf \n ebPercentage: %lu + (%lu / %lu) =~ %.8lf \n", ebPercentage_d, ebPercentage , ebperc_wrem_decimal, precision, ebPercentage + ((double) ebperc_wrem_decimal/precision));
  //debugging
  
  double bonusPercentage_d = ( ebPercentage_d / 100 ) + 1;
  //debugging

  uint64_t bperc_ebperc = (ebPercentage) * precision; 
  //calculate the whole part of the bonus percentage's remainder
  uint64_t bperc_ebperc_decimal = ( ( bperc_ebperc + ebperc_wrem_decimal) / percentage_precision ) % precision ; 
  //calcluate the actual remainder from bonus percentage's remainder

  uint64_t bonusPercentage = (ebPercentage+100) / percentage_precision; 
  //calcluate the bonus percentage to apply up to 'percentage_precision' number of digits

  printf("\n bonusPercentage_d: %.8lf \n bonusPercentage: %lu + (%lu / %lu) =~ %.8lf \n", bonusPercentage_d, bonusPercentage, bperc_ebperc_decimal, precision, bonusPercentage + ((double) bperc_ebperc_decimal/precision));
  //debugging

  double issuerPercentage_d = (double) (issuerPerc * 0.01);
  //debugging

  uint64_t issuerPercentage = issuerPerc / percentage_precision;
  //calcluate the issuerPercentage whole part based on 'percentage_precision' number of digits
  uint64_t issuerPercentage_rem = issuerPerc % percentage_precision;
  //calculate the issuerPercentage remainder based on 'percentage_precision' number of digits

  printf("\n issuerPercentage_d: %.8lf \n issuerPercentage: %lu + (%lu / %lu) =~ %.8lf \n", issuerPercentage_d, issuerPercentage, issuerPercentage_rem, 100, issuerPercentage + ((double) issuerPercentage_rem/100));
  //debugging

  long double ct;
  //debugging

  uint64_t satoshi_precision = 100000000;
  //define the precision for bitcoin amounts (satoshi)

  uint64_t createdTokens, createdTokens_decimal;
  //declare used variables for total created tokens

  uint64_t issuerTokens, issuerTokens_decimal;
  //declare used variables for total issuer tokens

  if( 2 == propType || 1 == propType) {
    printf("\n NUMBER OF PROPERTIES %ld", numProps); 
    printf("\n AMOUNT INVESTED: %ld BONUS PERCENTAGE: %f", amtTransfer,bonusPercentage_d);
    ct = ((amtTransfer/1e8) * (long double) numProps * bonusPercentage_d);

    uint64_t amtXfer_sig = amtTransfer/satoshi_precision;
    uint64_t amtXfer_rem = amtTransfer%satoshi_precision;
    uint64_t numProps_sig = numProps/satoshi_precision;
    uint64_t numProps_rem = numProps%satoshi_precision;

    printf("\n\n xfer sig:%lu xfer rem:%lu props_sig:%lu props_rem:%lu\n",(amtXfer_sig), amtXfer_rem, numProps_sig, numProps_rem);
    
    uint64_t Xfer_props_sigsig = (amtXfer_sig*numProps_sig)*satoshi_precision;
    uint64_t Xfer_props_sigrem = (amtXfer_sig*numProps_rem)%satoshi_precision*satoshi_precision;
    uint64_t Xfer_props_remsig = (amtXfer_rem*numProps_sig)*satoshi_precision;
    uint64_t Xfer_props_remsig_rem = (amtXfer_rem*numProps_sig)%satoshi_precision;
    uint64_t Xfer_props_remrem = (amtXfer_rem*numProps_rem)%(satoshi_precision*satoshi_precision);

    __uint128_t a;
    printf("\n\n xfer_props sig*sig: %lu xfer_props sig*rem: %lu xfer_props rem*sig: %lu xfer_props rem*rem: %lu \n", Xfer_props_sigsig, Xfer_props_sigrem, Xfer_props_remsig, Xfer_props_remrem);

    //uint64_t first_term = Xfer_props_sigsig + Xfer_props_sigrem + Xfer_props_remsig + Xfer_props_remrem; //first term is composed of the amount of satoshis * number of properties
    __uint128_t first_term = amtTransfer*numProps;
    //uint64_t first_term_r1 =  ((amtXfer_rem*numProps)%satoshi_precision);
    uint64_t second_term = (bonusPercentage*precision+bperc_ebperc_decimal); //second term is composed of bonus % + bonus % remainder/fractional part

    printf("\n props*xfer: %016llX bonus: %lu orig_props*xfer: %lu\n", first_term, second_term, (long double) numProps*amtTransfer);
    printf("\n %lu and %lu and %lu\n", amtXfer_rem*numProps ,(amtXfer_sig*numProps),satoshi_precision,0);

    uint64_t first_term_sig = first_term/satoshi_precision; //get the first term's significand
    uint64_t second_term_sig = second_term/precision;       //get the second term's significand
    uint64_t first_term_rem = first_term%satoshi_precision; //get the first term's remainder
    uint64_t second_term_rem = second_term%precision;       //get the second term's remainder
    
    //some logging to see the values for debugging
    printf("\n\nthing: isOK? %lu, %lu + (%lu / %lu) * %lu + (%lu / %lu) =~ %.8lf \n", first_term,first_term_sig, first_term_rem, satoshi_precision, second_term_sig, second_term_rem, precision, (double) first_term/satoshi_precision * (double) second_term/precision);

    //now we do some long multiplication
    uint64_t ftss = first_term_sig * second_term_sig;
    //the above is the result of multiplication of the first & second term's significand
    uint64_t ftsr = (first_term_sig * second_term_rem)/precision;
    //then we multiply the first term significand and the second term's remainder
    uint64_t ftsr_rem = (first_term_sig * second_term_rem)%precision;
    //then we get the remainder of the above term
    uint64_t ftrs = (first_term_rem * second_term_sig)/satoshi_precision;
    //then we multiply the first term's remainder with the second term's significand
    uint64_t ftrs_rem = (first_term_rem * second_term_sig)%satoshi_precision;
    //then we get the remainder of the above term
    uint64_t ftrr = (first_term_rem * second_term_rem)/(satoshi_precision * precision);
    //then we multiply the first term's remainder and the second term's remainder
    uint64_t ftrr_rem = (first_term_rem * second_term_rem)%(satoshi_precision * precision);
    //then we get the reaminder of the above term

    uint64_t carry_remainder = (ftsr_rem*satoshi_precision + ftrs_rem*precision + ftrr_rem)/(satoshi_precision * precision);
    //the above is the calculation of the significand of all the remainders above by addition
    uint64_t carry_remainder_rem =  (ftsr_rem*satoshi_precision + ftrs_rem*precision + ftrr_rem)%(satoshi_precision * precision);
    //then we get the remainder of the term above

    createdTokens = ftss + ftsr + ftrs + ftrr + carry_remainder;
    //we add the leading terms from above together and the carryover from the remainders 
    createdTokens_decimal = carry_remainder_rem;
    //then we store the remainder of the term

    //debugging
    //printf("\n\npart muli: %lu ,%lu + (%lu / %lu), %lu + (%lu / %lu), (%lu / %lu)  = %lu.%lu \n", ftss, ftsr, ftsr_rem, precision, ftrs, ftrs_rem, satoshi_precision, ftrr_rem, satoshi_precision * precision, createdTokens, createdTokens_decimal);
    
    printf("\n CREATED TOKENS %.8Lf, %lu + (%lu / %lu) ~= %.8lf",ct, createdTokens, createdTokens_decimal, precision*satoshi_precision, (double)createdTokens + (double)createdTokens_decimal/(satoshi_precision *precision));
    //TODO overflow checks  
    first_term = createdTokens * issuerPercentage; //leading terms
    second_term = (createdTokens * issuerPercentage_rem) / percentage_precision;
    second_term_rem = (createdTokens * issuerPercentage_rem) % percentage_precision;
    //printf("\n 1 whole terms: %lu * %lu + (%lu / %lu), =~ %lu + %lu + (%lu / %lu)\n", createdTokens, issuerPercentage, issuerPercentage_rem, 100, first_term, second_term, second_term_rem ,percentage_precision);

    uint64_t third_term = createdTokens_decimal * issuerPercentage;
    uint64_t fourth_term = (createdTokens_decimal * issuerPercentage_rem) / (precision*satoshi_precision*percentage_precision);
    uint64_t fourth_term_rem = (createdTokens_decimal * issuerPercentage_rem) % (precision*satoshi_precision*percentage_precision);
    //printf("\n 2 part terms: %lu %lu %lu =~ %lu + %lu + (%lu / %lu)\n", createdTokens_decimal, issuerPercentage, issuerPercentage_rem, third_term, fourth_term, fourth_term_rem, precision*satoshi_precision*percentage_precision);
    
    long double it = ct * issuerPercentage_d;

    carry_remainder = (second_term_rem*satoshi_precision*precision + fourth_term_rem)/(satoshi_precision*precision*percentage_precision);

    //printf("\n carry remainder is %lu \n", carry_remainder);

    carry_remainder_rem = second_term_rem*precision*satoshi_precision + fourth_term_rem%(satoshi_precision*precision*percentage_precision);

    issuerTokens = first_term + second_term + third_term + fourth_term + carry_remainder;
    issuerTokens_decimal = carry_remainder_rem;
    
    printf("\n ISSUER TOKENS: %.8Lf, %lu + (%lu / %lu ) ~= %.8lf \n",it, issuerTokens, issuerTokens_decimal, precision*satoshi_precision*percentage_precision, (double) issuerTokens + (double)issuerTokens_decimal/(satoshi_precision*precision*percentage_precision)); 

    //total tokens including remainders
    //printf("\n DIVISIBLE TOKENS (UI LAYER) CREATED: is ~= %.8lf, and %.8lf\n",(double)createdTokens + (double)createdTokens_decimal/(satoshi_precision *precision), (double) issuerTokens + (double)issuerTokens_decimal/(satoshi_precision*precision*percentage_precision) );

    if (2 == propType)
      printf("\n DIVISIBLE TOKENS (UI LAYER) CREATED: is ~= %.8lf, and %.8lf\n",(double)createdTokens + (double)createdTokens_decimal/(satoshi_precision *precision), (double) issuerTokens + (double)issuerTokens_decimal/(satoshi_precision*precision*percentage_precision) );
    else
      printf("\n DIVISIBLE TOKENS (UI LAYER) CREATED: is = %lu, and %lu\n", createdTokens, issuerTokens);
    
    tokens = std::make_pair(createdTokens,issuerTokens);

  } /*else {
    printf("\n NUMBER OF PROPERTIES %ld", numProps); 
    printf("\n AMOUNT INVESTED: %ld BONUS PERCENTAGE: %f", amtTransfer,bonusPercentage_d);
    ct = (uint64_t) ( (amtTransfer/1e8) * numProps * bonusPercentage_d);
    
    printf("\n CREATED TOKENS %.8Lf, %lu + (%lu / %lu)", ct,createdTokens, createdTokens_decimal, precision );
    
    uint64_t it = (uint64_t) (ct * issuerPercentage_d) ;
    issuerTokens = (uint64_t) (createdTokens * issuerPercentage) ;
    printf("\n DIVISIBLE TOKENS (UI LAYER) CREATED: is %ld, and %ld\n", (uint64_t) ct , (uint64_t) it);
    tokens = std::make_pair( (uint64_t) createdTokens, (uint64_t) issuerTokens);
  } */
}

int main() {

   long long int amountCreated = 0;
   long long int amountPremined = 0;
   
   std::map<std::string, std::pair<uint64_t, uint64_t> > database;
   std::pair <uint64_t, uint64_t> tokens;

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
   
   /*
   printf("\n funding prop 2 with prop 1 case 4... should be 1e8 :  Indivisible to Divisible (1 given -> 100000000/token) ");
   calculateFundraiser(2, (0.00000001 * 1e8) * 1e8,0,1406967060,1399591823,10000000000000000,100, tokens);
   amountCreated += tokens.first; amountPremined += tokens.second;
   printf(" HASH: c7591e401f9eb58f68d0a30150953fd4ae3ee566ceba00d99dded5b41c14b9bd\n");
   //printf("\nTokens created, Tokens for issuer: %lld %lld\n", tokens.first, tokens.second);
     
    */
   calculateFundraiser(1,(2.00000000 * 1e8),0,1398701578,1398371487,817545361,0.0, tokens);
   printf("\n262ab5f05b823c77ee7af8cb5ea9ce7ebbc0c34775a7bbeb7c3e477a4881dc89\n");
   //Expected: 'total created', 1635090722, 'tokens for issuer', 0.0
   //Returned: 1635090722, and 0.000000 

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
   //printf("\nTokens created, Tokens for issuer: %lld %lld\n", tokens.first, tokens.second);
   
   printf("\n Correct case 2...Divisible to Divisible (0.5 given -> 3400/token) ");
   calculateFundraiser(2, 0.5 * 1e8,10,1400544000,  1398109713,340000000000,0, tokens);
   amountCreated += tokens.first; amountPremined += tokens.second;
   printf(" HASH: 13f97d772f615007b2871dc40b8b9fae6cb75a133596e470558097436f556d19\n");
   //printf("\nTokens created, Tokens for issuer: %lld %lld\n", tokens.first, tokens.second);
   
   printf("\n Correct case 3... Divisible to Divisible (0.1 given -> 50/token) ");
   calculateFundraiser(2, 0.1 * 1e8,5,1409270940, 1406840780,5000000000,50, tokens);
   amountCreated += tokens.first; amountPremined += tokens.second;
   printf(" HASH: d4116a1d3830fd74fc66d026a532e1df53c2b7aa4bbe1855d56b2d866dc92c11\n");
   //printf("\nTokens created, Tokens for issuer: %lld %lld\n", tokens.first, tokens.second);
   
   printf("\n funding prop 1 with prop 2 case 3... should be 1 token : Divisible to Indivisible (1 given -> 1/token)");
   calculateFundraiser(1, (1.000 * 1e8),0,1407888180,1399591823,1,100, tokens);
   amountCreated += tokens.first; amountPremined += tokens.second;
   printf(" HASH: No hash, contrived example \n");
   //printf("\nTokens created, Tokens for issuer: %lld %lld\n", tokens.first, tokens.second);

   printf("\n funding prop 1 with prop 1 case 3.5 ... should be 1 token : Indivisible to Indivisible (50 given -> 1/token) ");
   calculateFundraiser(1, 50 * 1e8,0,1407888180,1399591823,1,100, tokens);
   amountCreated += tokens.first; amountPremined += tokens.second;
   printf(" HASH: No hash, contrived example  \n");
   //printf("\nTokens created, Tokens for issuer: %lld %lld\n", tokens.first, tokens.second);
  
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

