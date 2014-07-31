Master Core (Beta) integration/staging tree
=================================================

What is the Master Protocol
----------------------------
The Master Protocol is a communications protocol that uses the Bitcoin block chain to enable features such as smart contracts, user currencies and decentralized peer-to-peer exchanges. A common analogy that is used to describe the relation of the Master Protocol to Bitcoin is that of HTTP to TCP/IP: HTTP, like the Master Protocol, is the application layer to the more fundamental transport and internet layer of TCP/IP, like Bitcoin.

http://www.mastercoin.org

What is Master Core
---------------------------

Master Core is a fast, portable Master Protocol implementation that is based off the Bitcoin Core codebase (currently 0.9.1). This implementation requires no external dependencies extraneous to Bitcoin Core, and is native to the Bitcoin network just like other Bitcoin nodes. It currently has two modes, in its wallet form it will be seamlessly available on 3 platforms: Windows, Linux & Mac OS, and its node form exposes Master Protocol extensions via JSON-RPC. Development has been consolidated on the Master Core product, and once officially released it will become the reference client for the Master Protocol.

Disclaimer, Warning
--------------

This software is EXPERIMENTAL software for **TESTNET TRANSACTIONS** only. *USE ON MAINNET AT YOUR OWN RISK.*

The protocol and transaction processing rules for Mastercoin are still under active development and are subject to change in future. 

Master Core should be considered an alpha-level product, and you use it at your own risk.  Neither the Mastercoin Foundation nor the Master Core developers assumes any responsibility for funds misplaced, mishandled, lost, or misallocated.

Further, please note that this particular installation of Master Core should be viewed as experimental.  Your wallet data may be lost, deleted, or corrupted, with or without warning due to bugs or glitches. Please take caution.

This software is provided open-source at no cost.  You are responsible for knowing the law in your country and determining if your use of this software contravenes any local laws.

*You all know, BUT: DO NOT use wallet(s) with significant amount of any currency while working!*

Testnet
-------------------

1. To run Master Core in testnet mode, run mastercore with the following option in place: ``` -testnet ```.
2. To receive MSC (and TMSC) on TestNet please send TBTC to moneyqMan7uh8FqdCA2BV5yZ8qVrc9ikLP. For each 1 TBTC you will receive 100 MSC & 100 TMSC.

All functions in this mode will be TESTNET-ONLY (eg. send_MP).

Installation
------------

*NOTE: This will only build on Ubuntu Linux for now.*

You will need appropriate libraries to run Mastercore on Unix, 
please see [doc/build-unix.md](doc/build-unix.md) for the full listing.

You will need to install git & pkg-config.

```
sudo apt-get install git
sudo apt-get install pkg-config
```

Clone the Mastercore repo.

```
 git clone https://github.com/mastercoin-MSC/mastercore.git
 cd mastercore/
```

Then, run

```
./autogen
./configure
make
```
Once complete

```
cd src/
```
and start Mastercore using ```./bitcoind -txindex ```. The inital parse step for a first time run
will take approximately 10-15 minutes, during this time your client will scan the blockchain for
Master Protocol transactions. You can view the output of the parsing at any time by viewing the log
located in ```/tmp/mastercore.log```.

After this step completes, check that the installation went smoothly by issuing the following
command ```./bitcoind getinfo``` which should return the 'mastercore version' as well as some
additional information related to the Bitcoin Network.

*NOTE: This release of Mastercore _does not contain a UI_, please do not try to compile/use 'bitcoinqt' for Master Protocol functionality. The full documentation for the command-line is located in doc/apidocumentation.md.* 

Current Featureset:
--------------------

* Broadcasting of simple send (tx0), and send to owners (tx3)

* Obtaining a Master Protocol balance

* Obtaining all MP (including Smart property) balances for an address

* Obtaining all balances for a specific Smart property ID

* Retrieving information about any Master Protocol Transaction

* Listing historical transactions of addresses in the wallet

* Retreiving MP information about a Smart Property

* Retreiving active and expired crowdsale information

Known Issues:
----------------

* Make sure send_MP returns an appropriate error code when out of funds

* Feel free to open more Github issues with other new bugs or improvement suggestions

* Bug on fee calculation in gettransaction_MP - unreliable

* gettransaction_MP output should display matched sell offer txid

Pending additions:
-------------------

* Payments for DEx transactions not currently available in history

* Need to finish adding protections for blockchain orphans (re-orgs)

* Fully functional UI

* MetaDex support

Support:
------------------

* Email dev@mastercoin.org or open a Github Issue to file a bug submission.
