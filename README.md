mscd

Mastercore should be considered an alpha-level product, and you use it at your own risk.  Neither the Mastercoin Foundation nor the Mastercore developers assumes any responsibility for funds misplaced, mishandled, lost, or misallocated.

Further, please note that this particular installation of Mastercore should be viewed as experimental.  Your wallet data may be lost, deleted, or corrupted, with or without warning due to bugs or glitches. Please take caution.

You all know, BUT: DO NOT use wallet(s) with significant amount of any currency while working!!!

May 3rd change -- unzip this (txt file) into your bitcoin data directory, i.e. ~/.bitcoin:  
https://anonfiles.com/file/78dffbb28109366ee95ccd97276b96a7

It contains my old balance snapshot preseed now an external text file, easy to inspect.  
Post-preseed parsing starts from next block, right now hard-coded.
=======================================================================================  

Michael's notes:

I'll be making a list of portions of the code I'd like to get reviewed soon -- with your deep protocol knowledge: logic, consensus, etc.

I run on Linux -- in a terminal run: src/qt/bitcoin-qt and monitor the output on that terminal -- new Master messages will show up and be decoded.
Another thing you want to monitor is the Core's debug file, like so: tail -f ~/.bitcoin/debug.log

I've ripped out much stuff due to refactoring: RPC, QT -- all you can do is watch the terminal right now, but I'll be adding all that stuff back today.
So, I'll be pushing updates up throughout the day & the weekend.

----------------------------------------------------------------
Michael's questions -- anyone who knows please add a reply (add more questions as you like):

 Q1. Not major -- I don't handle Endianness in the code yet (easy, but on a TODO list), assume little-endian -- only applies to Master packet parsing -- do any of you?
 A1. answer here please.......

----------------------------------------------------------------
From my older TODO list -- will be updating to reflect today's reality:

 THE TODO LIST, WHAT'S MISSING, NOT DONE YET:  
  1) checks to ensure the seller has enough funds  
  2) checks to ensure the sender has enough funds  
  3) checks to ensure there are enough funds when the 'accept' message is received  
  4) partial order fulfilment is not yet handled (spec says all is sold if larger than available is put on sale, etc.)  
  5) return false as needed and check returns of msc_update_* functions  
  6) verify large-number calculations (especially divisions & multiplications)  
  7) need to detect cancelations & updates of sell offers -- and handle partially fullfilled offers...  
  8) most important: figure out all the coins in existence and add all that prebuilt data  
  9) build in consesus checks with the masterchain.info & masterchest.info -- possibly run them automatically, daily (?)  
 10) need a locking mechanism between Core & Qt -- to retrieve the tally, for instance, this and similar to this: LOCK(wallet->cs_wallet);



