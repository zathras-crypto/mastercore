#!/bin/bash

PASS=0
FAIL=0
clear
printf "Preparing a test environment...\n"
printf "   * Starting a fresh regtest daemon\n"
rm -r ~/.bitcoin/regtest
./src/omnicored --regtest --server --daemon --omniactivationallowsender=any >/dev/null
sleep 10
printf "   * Preparing some mature testnet BTC\n"
./src/omnicore-cli --regtest setgenerate true 102 >/dev/null
printf "   * Obtaining a master address to work with\n"
ADDR=$(./src/omnicore-cli --regtest getnewaddress OMNIAccount)
printf "   * Funding the address with some testnet BTC for fees\n"
./src/omnicore-cli --regtest sendtoaddress $ADDR 0.001 >/dev/null
./src/omnicore-cli --regtest sendtoaddress $ADDR 0.002 >/dev/null
./src/omnicore-cli --regtest sendtoaddress $ADDR 0.003 >/dev/null
./src/omnicore-cli --regtest sendtoaddress $ADDR 0.004 >/dev/null
./src/omnicore-cli --regtest sendtoaddress $ADDR 0.005 >/dev/null
./src/omnicore-cli --regtest sendtoaddress $ADDR 10.005 >/dev/null
./src/omnicore-cli --regtest setgenerate true 1 >/dev/null
printf "   * Participating in the Exodus crowdsale to obtain some OMNI\n"
JSON="{\"moneyqMan7uh8FqdCA2BV5yZ8qVrc9ikLP\":10,\""$ADDR"\":0.0002}"
./src/omnicore-cli --regtest sendmany OMNIAccount $JSON >/dev/null
./src/omnicore-cli --regtest setgenerate true 1 >/dev/null
printf "   * Master address is %s\n" $ADDR
printf "   * Generating addresses to use\n"
ADDRESS=()
for i in {1..6}
do
   ADDRESS=("${ADDRESS[@]}" $(./src/omnicore-cli --regtest getnewaddress))
done
printf "Confirming publish feed transactions are invalidated before activation...\n"
printf "   * Sending a Publish Feed transaction\n"
TXID=$(./src/omnicore-cli --regtest omni_sendpublishfeed $ADDR 1 1000)
./src/omnicore-cli --regtest setgenerate true 1 >/dev/null
printf "     # Checking the transaction was invalid... "
RESULT=$(./src/omnicore-cli --regtest omni_gettransaction $TXID | grep valid | cut -c15-)
if [ $RESULT == "false," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "Activating the publish feed transaction...\n"
printf "   * Sending the activation\n"
BLOCKS=$(./src/omnicore-cli --regtest getblockcount)
TXID=$(./src/omnicore-cli --regtest omni_sendactivation $ADDR 12 $(($BLOCKS + 8)) 1100001)
./src/omnicore-cli --regtest setgenerate true 1 >/dev/null
printf "     # Checking the activation transaction was valid... "
RESULT=$(./src/omnicore-cli --regtest omni_gettransaction $TXID | grep valid | cut -c15-)
if [ $RESULT == "true," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   * Mining 10 blocks to forward past the activation block\n"
./src/omnicore-cli --regtest setgenerate true 10 >/dev/null
printf "     # Checking the activation went live as expected... "
FEATUREID=$(./src/omnicore-cli --regtest omni_getactivations | grep -A 10 completed | grep featureid | cut -c27-28)
if [ $FEATUREID == "12" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $FEATUREID
    FAIL=$((FAIL+1))
fi
printf "Testing Publish Feed...\n"
printf "   * Sending a Publish Feed transaction (feed 1, value 1000)\n"
TXID=$(./src/omnicore-cli --regtest omni_sendpublishfeed $ADDR 1 1000)
./src/omnicore-cli --regtest setgenerate true 1 >/dev/null
printf "     # Checking the transaction was valid... "
RESULT=$(./src/omnicore-cli --regtest omni_gettransaction $TXID | grep valid | cut -c15-)
if [ $RESULT == "true," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "     # Checking the value for feed 1 at address is now 1000... "
RESULT=$(./src/omnicore-cli --regtest omni_getfeed $ADDR 1 | grep value | cut -d ':' -f2 | tr -d '[[:space:]]')
if [ $RESULT == "1000" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "     # Getting the block number to use later...\n"
BLOCKS=$(./src/omnicore-cli --regtest getblockcount)
printf "   * Sending several Publish Feed transactions for feed 1, (highest will be 5000 but latest 3000)\n"
TXID=$(./src/omnicore-cli --regtest omni_sendpublishfeed $ADDR 1 2000)
./src/omnicore-cli --regtest setgenerate true 1 >/dev/null
TXID=$(./src/omnicore-cli --regtest omni_sendpublishfeed $ADDR 1 4000)
./src/omnicore-cli --regtest setgenerate true 1 >/dev/null
TXID=$(./src/omnicore-cli --regtest omni_sendpublishfeed $ADDR 1 5000)
./src/omnicore-cli --regtest setgenerate true 1 >/dev/null
TXID=$(./src/omnicore-cli --regtest omni_sendpublishfeed $ADDR 1 3000)
./src/omnicore-cli --regtest setgenerate true 1 >/dev/null
printf "     # Checking the value for feed 1 at address is now at latest (3000)... "
RESULT=$(./src/omnicore-cli --regtest omni_getfeed $ADDR 1 | grep value | cut -d ':' -f2 | tr -d '[[:space:]]')
if [ $RESULT == "3000" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "     # Checking the value for feed 1 at address is 1000 at block %d... " $BLOCKS
RESULT=$(./src/omnicore-cli --regtest omni_getfeed $ADDR 1 $BLOCKS | grep value | cut -d ':' -f2 | tr -d '[[:space:]]')
if [ $RESULT == "1000" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   * Sending Publish Feed transactions for feed refernces 2 and 65\n"
TXID=$(./src/omnicore-cli --regtest omni_sendpublishfeed $ADDR 2 200)
./src/omnicore-cli --regtest setgenerate true 1 >/dev/null
TXID=$(./src/omnicore-cli --regtest omni_sendpublishfeed $ADDR 65 6500)
./src/omnicore-cli --regtest setgenerate true 1 >/dev/null
printf "     # Checking the value for feed 2 at address is now 200... "
RESULT=$(./src/omnicore-cli --regtest omni_getfeed $ADDR 2 | grep value | cut -d ':' -f2 | tr -d '[[:space:]]')
if [ $RESULT == "200" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "     # Checking the value for feed 65 at address is now 6500... "
RESULT=$(./src/omnicore-cli --regtest omni_getfeed $ADDR 65 | grep value | cut -d ':' -f2 | tr -d '[[:space:]]')
if [ $RESULT == "6500" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "     # Checking that feeds 1, 2 and 65 are returned in omni_getfeeds... "
RESULT=$(./src/omnicore-cli --regtest omni_getfeeds $ADDR | grep reference | cut -d ':' -f2 | tr -d '[[:space:]]')
if [ $RESULT == "1,2,65," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   * Checking feed history for feed ref 1\n"
printf "     # Checking that values 1000, 2000, 4000, 5000 & 3000 exist in history... "
RESULT=$(./src/omnicore-cli --regtest omni_getfeedhistory $ADDR 1 | grep value | cut -d ':' -f2 | tr -d '[[:space:]]')
if [ $RESULT == "10002000400050003000" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   * Committing feed database to log\n"
./src/omnicore-cli --regtest mscrpc 15


# TODO - reorg


printf "\n"
printf "####################\n"
printf "#  Summary:        #\n"
printf "#    Passed = %d   #\n" $PASS
printf "#    Failed = %d    #\n" $FAIL
printf "####################\n"
printf "\n"

./src/omnicore-cli --regtest stop



