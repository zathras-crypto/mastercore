#Assumes the following 
#Running build: https://github.com/faizkhan00/mastercore/tree/regtest-michael-0921

echo -e "\nUSAGE: \nStarts the test case for a raw tx send\n"

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
   callnode1 setaccount $ADDR2 node1-them

   DEFACCT=""

   echo -e "Attempting seed BTC to address $ADDR1 ..."
   callnode1 sendmany "$DEFACCT" "{\"${ADDR1}\":2.0}"

   echo -e "Committing transaction..."
   callnode1 setgenerate true 1
   sleep 1

   echo -e "Sending to get MSC...\n"
   callnode1 sendmany node1-me "{\"moneyqMan7uh8FqdCA2BV5yZ8qVrc9ikLP\":0.5,\"mpexoDuSkGGqvqrkrjiFng38QPkJQVFyqv\":0.5}"
   callnode1 sendtoaddress $ADDR1 1.0

   echo -e "Committing transaction...\n" 
   callnode1 setgenerate true 1
   sleep 1

   echo -e "Initial Balances..."
   callnode1 getallbalancesforid_MP 1

   echo -e "Creating fixed SP with raw transaction\n"
   callnode1 sendrawtx_MP $ADDR1 "00000032010002000000007465737420737472696e672031007465737420737472696e672032007465737420737472696e672033007465737420737472696e672034007465737420737472696e67203500000000174876E800"

   echo -e "Committing transactions...\n"
   callnode1 setgenerate true 1
   sleep 5
 
   echo -e "Showing results..."
   callnode1 getallbalancesforid_MP 3

   echo -e "Sending SP3 from $ADDR1 to $ADDR2\n"
   callnode1 send_MP $ADDR1 $ADDR2 3 25.00000000

   echo -e "Committing transactions...\n"
   callnode1 setgenerate true 1
   sleep 5
 
   echo -e "Showing results..."
   callnode1 getallbalancesforid_MP 3

   #done
   callnode1 stop
