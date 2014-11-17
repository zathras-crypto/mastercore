#Assumes the following 
#Running build: https://github.com/faizkhan00/mastercore/tree/regtest-michael-0921

echo -e "\nUSAGE: \nStarts the test case for a re-org\n"

callnode1() {
  ./bitcoin-cli -rpcport=8330 -rpcuser=user -rpcpassword=password "$@"
}

   init_node1() {
     echo -e "Generating new blocks..." 
     callnode1 setgenerate true 102 
     sleep 5
   }

   init_node1

   ADDR1=$(callnode1 getnewaddress)
   echo -e "Made new address $ADDR1 ..."
   callnode1 setaccount $ADDR1 node1-me

   ADDR2=$(callnode1 getnewaddress)
   echo -e "Made new address $ADDR2 ..."
   callnode1 setaccount $ADDR2 extras

   ADDR3=$(callnode1 getnewaddress)
   echo -e "Made new address $ADDR3 ..."
   callnode1 setaccount $ADDR3 extras

   P2SHADDR1=$(callnode1 addmultisigaddress 2 "[\"$ADDR1\",\"$ADDR2\",\"$ADDR3\"]")
   echo -e "Made new multisig address ... $P2SHADDR1"
   
   DEFACCT=""

   echo -e "Attempting seed BTC to address $ADDR1 ..."
   callnode1 sendmany "$DEFACCT" "{\"${ADDR1}\":2.0}"

   echo -e "Committing transaction..."
   callnode1 setgenerate true 1
   sleep 1

   echo -e "Sending to get MSC...\n"
   callnode1 sendmany node1-me "{\"moneyqMan7uh8FqdCA2BV5yZ8qVrc9ikLP\":0.5,\"mpexoDuSkGGqvqrkrjiFng38QPkJQVFyqv\":0.5}"
   callnode1 sendtoaddress $ADDR1 1.0
   callnode1 sendtoaddress $P2SHADDR1 1.0

   echo -e "Committing transaction...\n" 
   callnode1 setgenerate true 1
   sleep 1

   echo -e "Initial Balances..."
   callnode1 getallbalancesforid_MP 1

   echo -e "Sending MSC from $ADDR1 to $P2SHADDR1\n"
   callnode1 send_MP $ADDR1 $P2SHADDR1 1 40.0

   echo -e "Committing transactions...\n"
   callnode1 setgenerate true 1
   sleep 5
 
   echo -e "Showing results..."
   callnode1 getallbalancesforid_MP 1

   echo -e "Sending MSC from $P2SHADDR1 to $ADDR1\n"
   callnode1 send_MP $P2SHADDR1 $ADDR1 1 25.0 $ADDR2

   echo -e "Committing transactions...\n"
   callnode1 setgenerate true 1
   sleep 5
 
   echo -e "Showing results..."
   callnode1 getallbalancesforid_MP 1

   #done
   callnode1 stop
