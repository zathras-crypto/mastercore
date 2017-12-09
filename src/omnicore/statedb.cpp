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

#include "utilstrencodings.h"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <stdint.h>

#include <string>
#include <vector>

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

// Roll back the state - iterate backwards to block N reversing each action
void COmniStateDB::rollBackState(int block)
{
    if (msc_debug_verbose) {
        PrintToLog("%s() called for block %d - current state:\n",  __FUNCTION__, block);
        printAll();
        PrintFreezeState();
    }

    int actionsTaken = 0;

    leveldb::Iterator* it = NewIterator(); // NewIterator is effectively a lightweight snapshot, believe we can delete from the DB without affecting the iterator
    for (it->SeekToLast(); it->Valid(); it->Prev()) {
        std::string valueStr = it->value().ToString();
        std::vector<std::string> vecValueStr;
        boost::split(vecValueStr, valueStr, boost::is_any_of(":"), boost::token_compress_on);
        assert(vecValueStr.size() == 3);

        int entryBlock = atoi(vecValueStr[1]);
        if (entryBlock < block) continue;

        if (msc_debug_verbose) PrintToLog("%s() Reversing sequence %s with action %s\n", __FUNCTION__, it->key().ToString(), vecValueStr[2]);
        ++actionsTaken;

        std::vector<std::string> vecActionStr;
        boost::split(vecActionStr, vecValueStr[2], boost::is_any_of(","), boost::token_compress_on);
        assert(vecActionStr.size() > 1);
        std::string action = vecActionStr[0];

        if (action == "freeze" || action == "unfreeze") {
            assert(vecActionStr.size() == 3); // "freeze/unfreeze, address, prop"
            uint32_t propertyId = boost::lexical_cast<uint32_t>(vecActionStr[2]);
            if (action == "freeze") unfreezeAddress(vecActionStr[1], propertyId);
            if (action == "unfreeze") freezeAddress(vecActionStr[1], propertyId);
        }

        if (action == "disablefreezing") {
            assert(vecActionStr.size() == 3); // "disable, prop, blockFreezingPreviouslyEnabled"
            uint32_t propertyId = boost::lexical_cast<uint32_t>(vecActionStr[1]);
            int liveBlock = atoi(vecActionStr[2]);
            enableFreezing(propertyId, liveBlock);
        }

        if (action == "enablefreezing") {
            assert(vecActionStr.size() == 2); // "enable, prop"
            uint32_t propertyId = boost::lexical_cast<uint32_t>(vecActionStr[1]);
            int liveBlock = 0;
            uint256 emptyTxid;
            disableFreezing(propertyId, &liveBlock, emptyTxid, block);
        }

        pdb->Delete(writeoptions, it->key());
    }

    if (msc_debug_verbose) {
        PrintToLog("%s() ending for block %d - there were %d actions reversed - new state:\n",  __FUNCTION__, block, actionsTaken);
        printAll();
        PrintFreezeState();
    }

    delete it;
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

