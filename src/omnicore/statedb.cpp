/**
 * @file statedb.cpp
 *
 * This file contains code for the state database, which contains all state changes made to the Omni state.
 *
 * During reorganizations each action in the state database can be reversed in sequence to arrive at a previous state.
 */

#include "omnicore/statedb.h"

#include "omnicore/omnicore.h"
#include "omnicore/log.h"

#include "leveldb/db.h"

#include <stdint.h>

using namespace mastercore;

// Obtain the next sequence number
int64_t COmniStateDB::getNextSequenceNo()
{
   int seqNo = 1;
   leveldb::Iterator* it = NewIterator();
   for (it->SeekToFirst(); it->Valid(); it->Next()) {
        ++seqNo;
   }
   delete it;

   // TODO: potential optimization via SeekToLast();

   return seqNo;
}

// Write a new entry into the stateDB
void COmniStateDB::writeStateEntry(const uint256& txid, int block, const std::string& action)
{
   std::string key = strprintf("%d", getNextSequenceNo());
   std::string value = strprintf("%s:%d:%s", txid.GetHex(), block, action);

   leveldb::Status status = pdb->Put(writeoptions, key, value);
   assert(status.ok());
   ++nWritten;
}

// Show DB statistics
void COmniStateDB::printStats()
{
    PrintToConsole("COmniStateDB stats: nWritten= %d , nRead= %d\n", nWritten, nRead);
}

// Show DB records
void COmniStateDB::printAll()
{
    int count = 0;
    leveldb::Iterator* it = NewIterator();
    for(it->SeekToFirst(); it->Valid(); it->Next()) {
        ++count;
        PrintToConsole("entry #%8d= %s:%s\n", count, it->key().ToString(), it->value().ToString());
    }
    delete it;
}

