Master Protocol Core integration/staging tree
=================================================

http://www.mastercoin.org

The Master Protocol is a communications protocol that uses the Bitcoin block chain to enable features such as smart contracts, user currencies and decentralized peer-to-peer exchanges. A common analogy that is used to describe the relation of the Master Protocol to Bitcoin is that of HTTP to TCP/IP: HTTP, like the Master Protocol, is the application layer to the more fundamental transport and internet layer of TCP/IP, like Bitcoin.

Disclaimer, Warning
--------------

This software is EXPERIMENTAL software for TESTING only. *USE AT YOUR OWN RISK.*

The protocol and transaction processing rules for Mastercoin are still under active development and are subject to change in future. 

Mastercore should be considered an alpha-level product, and you use it at your own risk.  Neither the Mastercoin Foundation nor the Mastercore developers assumes any responsibility for funds misplaced, mishandled, lost, or misallocated.

Further, please note that this particular installation of Mastercore should be viewed as experimental.  Your wallet data may be lost, deleted, or corrupted, with or without warning due to bugs or glitches. Please take caution.

This software is provided open-source at no cost.  You are responsible for knowing the law in your country and determining if your use of this software contravenes any local laws.

*You all know, BUT: DO NOT use wallet(s) with significant amount of any currency while working!*

Sanctioned Preseed
--------------------

*Note: Pre-seeding will be removed during future development.*

It contains my old balance snapshot preseed now an external text file, easy to inspect.  
Post-preseed parsing starts from next block, right now hard-coded.

May 3rd change -- unzip this (txt file) into your bitcoin data directory, i.e. ~/.bitcoin:  
https://anonfiles.com/file/78dffbb28109366ee95ccd97276b96a7

Installation
------------

```
./autogen
./configure
make
```

Known Issues:
----------------
* Payments for DEx transactions not currently available in history

* Transactions before preseed (290630) not currently available in history

* Feel free to open more Github issues with other new bugs or improvement suggestions

Pending additions:
------------------

* gettransaction_MP output should display matched sell offer txid

