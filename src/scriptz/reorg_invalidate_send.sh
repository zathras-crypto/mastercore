#Assumes the following 
#Running build: https://github.com/faizkhan00/mastercore/tree/regtest-michael-0921

echo -e "\nUSAGE: \nStarts the test case for a re-org\n"

callnode1() {
  ./bitcoin-cli -rpcport=8330 -rpcuser=user -rpcpassword=password "$@"
}

callnode2() {
  ./bitcoin-cli -rpcport=8332 -rpcuser=user -rpcpassword=password "$@"
}

callnode3() {
  ./bitcoin-cli -rpcport=8334 -rpcuser=user -rpcpassword=password "$@"
}
   #rm before starting this
   #rm ${HOME}/.bitcoin/regtest/ -rf
   #rm ${HOME}/.bitcoin2/regtest/ -rf
   #rm ${HOME}/.bitcoin3/regtest/ -rf

   connect_nodes() {
     echo "Connecting nodes..."
     callnode3 addnode localhost:18333 onetry 
     echo "Nodes 1 and 3 are now connected."
   }

   init_node1() {
     echo -e "Generating new blocks..." 
     callnode1 setgenerate true 102 
     sleep 5
   }

   connect_nodes
   
   init_node1

   ADDR1=$(callnode1 getnewaddress)
   echo -e "Made new address $ADDR1 ..."
   callnode1 setaccount $ADDR1 node1-me
   PRIV1=$(callnode1 dumpprivkey $ADDR1)
   
   ADDR2=$(callnode1 getnewaddress)
   echo -e "Made new address $ADDR2 ..."
   callnode1 setaccount $ADDR2 node1-alice
   PRIV2=$(callnode1 dumpprivkey $ADDR2)

   ADDR3=$(callnode1 getnewaddress)
   echo -e "Made new address $ADDR3 ..."
   callnode1 setaccount $ADDR3 node1-bob
   PRIV3=$(callnode1 dumpprivkey $ADDR3)

   DEFACCT=""

   echo -e "Attempting send to address $ADDR1 and $ADDR2 ..."
   callnode1 sendmany "$DEFACCT" "{\"${ADDR1}\":2.0,\"${ADDR2}\":2.0,\"${ADDR3}\":2.0}"

   echo -e "Committing transaction..."
   callnode1 setgenerate true 1
   sleep 1

   echo -e "Sending to get MSC...\n"
   callnode1 sendmany node1-me "{\"moneyqMan7uh8FqdCA2BV5yZ8qVrc9ikLP\":0.5,\"mpexoDuSkGGqvqrkrjiFng38QPkJQVFyqv\":0.5}"
   callnode1 sendmany node1-alice "{\"moneyqMan7uh8FqdCA2BV5yZ8qVrc9ikLP\":0.5,\"mpexoDuSkGGqvqrkrjiFng38QPkJQVFyqv\":0.5}"
   callnode1 sendmany node1-bob "{\"moneyqMan7uh8FqdCA2BV5yZ8qVrc9ikLP\":0.5,\"mpexoDuSkGGqvqrkrjiFng38QPkJQVFyqv\":0.5}"
   callnode1 sendtoaddress $ADDR1 1.0

   echo -e "Committing transaction...\n" 
   callnode1 setgenerate true 1
   sleep 1

   echo -e "Initial Balances..."
   callnode1 getallbalancesforid_MP 1

   echo -e "Stopping node 3 temporarily..."
   callnode3 stop 

   echo -e "Sending MSC from $ADDR1 to $ADDR2\n"
   callnode1 send_MP $ADDR1 $ADDR2 1 40.0

   echo -e "Committing transactions...\n"
   callnode1 setgenerate true 1
   sleep 5
 
   echo -e "Showing results..."
   callnode1 getallbalancesforid_MP 1

   echo -e "Done. Now we will attempt a double spend on reorg. \n\n"
   sleep 1

   echo -e "Start node 3..."
   sleep 15

   echo -e "importing keys..."
   callnode3 importprivkey $PRIV1 node1-me

   echo -e "Sending MSC from $ADDR1 to $ADDR3\n"
   callnode3 send_MP $ADDR1 $ADDR3 1 40.0

   echo -e "Committing transactions...\n"
   callnode3 setgenerate true 1
   sleep 5

   echo -e "Showing results..."
   callnode3 getallbalancesforid_MP 1


   echo -e "Burying Transactions..."
   #Node 3 started up in isolation
   callnode3 setgenerate true 20

   echo -e "Compare diverged state (balances) Node 1..."
   callnode1 getallbalancesforid_MP 1
   echo -e "Compare diverged state (balances) Node 3..."
   callnode3 getallbalancesforid_MP 1

   echo "Connecting nodes..."
   callnode3 addnode localhost:18333 onetry 
   echo "Nodes 1 and 3 are now connected."
   sleep 15

   BHASH=$(callnode1 getbestblockhash)
   BHASH__=$(callnode3 getbestblockhash)
   echo -e "\n\nBlock hashes should now be equal:\n "$BHASH" and "$BHASH__" "
   
   echo -e "Compare working state (balances) Node 1..."
   callnode1 getallbalancesforid_MP 1
   echo -e "Compare working state (balances) Node 3..."
   callnode3 getallbalancesforid_MP 1
   
   echo -e "Generating new block to re-broadcast old now invalid TX..."
   callnode1 setgenerate true 1
   sleep 15

   echo -e "Compare final state (balances) Node 1..."
   callnode1 getallbalancesforid_MP 1
   echo -e "Compare final state (balances) Node 3..."
   callnode3 getallbalancesforid_MP 1

   #done
   callnode1 stop
   callnode3 stop
