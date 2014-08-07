#Assumes the following 
#Running build: https://github.com/faizkhan00/mastercore/tree/regtest-michael-0921

echo -e "\nUSAGE: \n-g{1,2,3}, -h: generates 101 blocks,50 BTC, and 50MSC, this help"


if [[ "$1" = "-g1" ]]
then
  echo "Generating 101 blocks on node 1...";

  ./bitcoin-cli -rpcport=8330 -rpcuser=user -rpcpassword=password setgenerate true 101 

  ./bitcoin-cli -rpcport=8330 -rpcuser=user -rpcpassword=password   sendtoaddress "moneyqMan7uh8FqdCA2BV5yZ8qVrc9ikLP" 0.5

  ./bitcoin-cli -rpcport=8330 -rpcuser=user -rpcpassword=password   sendtoaddress "mpexoDuSkGGqvqrkrjiFng38QPkJQVFyqv" 0.5
  
  ./bitcoin-cli -rpcport=8330 -rpcuser=user -rpcpassword=password setgenerate true  

  ./bitcoin-cli -rpcport=8330 -rpcuser=user -rpcpassword=password getallbalancesforid_MP 1
fi

if [[ "$1" = "-g2" ]]
then
  echo "Generating 101 blocks on node 2...";

  ./bitcoin-cli -rpcport=8332 -rpcuser=user -rpcpassword=password setgenerate true 101
 
  ./bitcoin-cli -rpcport=8332 -rpcuser=user -rpcpassword=password   sendtoaddress "moneyqMan7uh8FqdCA2BV5yZ8qVrc9ikLP" 0.5

  ./bitcoin-cli -rpcport=8332 -rpcuser=user -rpcpassword=password   sendtoaddress "mpexoDuSkGGqvqrkrjiFng38QPkJQVFyqv" 0.5
  
  ./bitcoin-cli -rpcport=8332 -rpcuser=user -rpcpassword=password setgenerate true  

  ./bitcoin-cli -rpcport=8332 -rpcuser=user -rpcpassword=password getallbalancesforid_MP 1
fi

if [[ "$1" = "-g3" ]]
then
  echo "Generating 101 blocks on node 3...";

  ./bitcoin-cli -rpcport=8334 -rpcuser=user -rpcpassword=password setgenerate true 101
 
  ./bitcoin-cli -rpcport=8334 -rpcuser=user -rpcpassword=password   sendtoaddress "moneyqMan7uh8FqdCA2BV5yZ8qVrc9ikLP" 0.5

  ./bitcoin-cli -rpcport=8334 -rpcuser=user -rpcpassword=password   sendtoaddress "mpexoDuSkGGqvqrkrjiFng38QPkJQVFyqv" 0.5
  
  ./bitcoin-cli -rpcport=8334 -rpcuser=user -rpcpassword=password setgenerate true  

  ./bitcoin-cli -rpcport=8334 -rpcuser=user -rpcpassword=password getallbalancesforid_MP 1
fi

