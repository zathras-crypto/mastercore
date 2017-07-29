/**
 * @file utdb.cpp
 *
 * This file contains functionality for the unique tokens database.
 */

#include "omnicore/utdb.h"

#include "omnicore/omnicore.h"
#include "omnicore/errors.h"
#include "omnicore/log.h"

#include "leveldb/db.h"

#include <stdint.h>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

 // TODO: getTotalTokens and sanity check tokenIdEnd is not > the total number of tokens, can only be added once getTotalTokens supports unique tokens
 // TODO: optimize - start the iterator at leveldb key propertyId_1* as this will position the iterator at the start of the ranges for the property
 // TODO: re-enable & increase logging

/* Gets the range a unique token is in
 */
std::pair<int64_t,int64_t> CMPUniqueTokensDB::GetRange(const uint32_t &propertyId, const int64_t &tokenId)
{
    assert(pdb);

    leveldb::Iterator* it = NewIterator();
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        std::string key = it->key().ToString();
        std::vector<std::string> vPropertyId;
        boost::split(vPropertyId, key, boost::is_any_of("_"), boost::token_compress_on);
        if (2 != vPropertyId.size()) continue; // unexpected - TODO: log this error
        std::string strPropId = vPropertyId[0];
        strPropId.erase(0, strPropId.find_first_not_of('0'));
        uint32_t dbPropertyId = boost::lexical_cast<uint32_t>(strPropId);
        if (dbPropertyId != propertyId) continue;
        std::vector<std::string> vRanges;
        boost::split(vRanges, vPropertyId[1], boost::is_any_of("-"), boost::token_compress_on);
        if (2 != vRanges.size()) continue; // unexpected - TODO: log this error
        std::string strIdStart = vRanges[0];
        std::string strIdEnd = vRanges[1];
        strIdStart.erase(0, strIdStart.find_first_not_of('0'));
        strIdEnd.erase(0, strIdEnd.find_first_not_of('0'));
        int64_t dbTokenIdStart = boost::lexical_cast<int64_t>(strIdStart);
        int64_t dbTokenIdEnd = boost::lexical_cast<int64_t>(strIdEnd);
        // check if the ID supplied to the function is within this range in the DB
        if (tokenId >= dbTokenIdStart && tokenId <= dbTokenIdEnd) {
            return std::make_pair(dbTokenIdStart, dbTokenIdEnd);
        }
    }
    delete it;
    return std::make_pair(0,0); // token not found, return zero'd range
}

/* Checks if the range of tokens is contiguous (ie owned by a single address)
 */
bool CMPUniqueTokensDB::IsRangeContiguous(const uint32_t &propertyId, const int64_t &tokenIdStart, const int64_t &tokenIdEnd)
{
    assert(pdb); // not safe to continue without access to utdb

    leveldb::Iterator* it = NewIterator();
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        std::string key = it->key().ToString();
        // break into component integers
        std::vector<std::string> vPropertyId;
        boost::split(vPropertyId, key, boost::is_any_of("_"), boost::token_compress_on);
        if (2 != vPropertyId.size()) continue; // unexpected - TODO: log this error
        std::string strPropId = vPropertyId[0];
        strPropId.erase(0, strPropId.find_first_not_of('0'));
        uint32_t dbPropertyId = boost::lexical_cast<uint32_t>(strPropId);
        if (dbPropertyId != propertyId) continue;
        std::vector<std::string> vRanges;
        boost::split(vRanges, vPropertyId[1], boost::is_any_of("-"), boost::token_compress_on);
        if (2 != vRanges.size()) continue; // unexpected - TODO: log this error
        std::string strIdStart = vRanges[0];
        std::string strIdEnd = vRanges[1];
        strIdStart.erase(0, strIdStart.find_first_not_of('0'));
        strIdEnd.erase(0, strIdEnd.find_first_not_of('0'));
        int64_t dbTokenIdStart = boost::lexical_cast<int64_t>(strIdStart);
        int64_t dbTokenIdEnd = boost::lexical_cast<int64_t>(strIdEnd);
        // check if the range supplied to the function is within this range in the DB
        if (tokenIdStart >= dbTokenIdStart && tokenIdStart <= dbTokenIdEnd) {
            if (tokenIdEnd <= dbTokenIdEnd) { // the start ID falls within this range
                return true; // the end ID falls within this range - owned by a single address
            } else {
                // the start ID falls within this range but the end ID does not - not owned by a single address
                return false;
            }
        }
    }
    delete it;
    return true; // range not found, so cannot be fragmented across multiple owners
}

/* Moves a range of tokens (returns false if not able to move)
 */
bool CMPUniqueTokensDB::MoveUniqueTokens(const uint32_t &propertyId, const int64_t &tokenIdStart, const int64_t &tokenIdEnd, const std::string &from, const std::string &to)
{
    assert(pdb);

    // check that 'from' owns both the start and end token and that the range is contiguous (owns the entire range)
    std::string startOwner = GetUniqueTokenOwner(propertyId, tokenIdStart);
    std::string endOwner = GetUniqueTokenOwner(propertyId, tokenIdEnd);
    bool contiguous = IsRangeContiguous(propertyId, tokenIdStart, tokenIdEnd);
    if (startOwner != from || endOwner != from || !contiguous) return false;

    // are we moving the complete range from 'from'?
    bool bMovingCompleteRange = false;
    std::pair<int64_t,int64_t> startTokenRange = GetRange(propertyId, tokenIdStart);
    std::pair<int64_t,int64_t> endTokenRange = GetRange(propertyId, tokenIdEnd);
    if (startTokenRange.first == tokenIdStart && startTokenRange.second == tokenIdEnd) {
        if (endTokenRange.first == tokenIdStart && endTokenRange.second == tokenIdEnd) {
            bMovingCompleteRange = true;
        }
    }

    // does 'to' have adjacent ranges that need to be merged?
    bool bToAdjacentRangeBefore = false;
    bool bToAdjacentRangeAfter = false;
    std::string rangeBelowOwner = GetUniqueTokenOwner(propertyId, tokenIdStart-1);
    std::string rangeAfterOwner = GetUniqueTokenOwner(propertyId, tokenIdEnd+1);
    if (rangeBelowOwner == to) {
        bToAdjacentRangeBefore = true;
    }
    if (rangeAfterOwner == to) {
        bToAdjacentRangeAfter = true;
    }

    // adjust 'from' ranges
    if (bMovingCompleteRange == true) {
        DeleteRange(propertyId, tokenIdStart, tokenIdEnd);
    } else {
        DeleteRange(propertyId, startTokenRange.first, endTokenRange.second);
        if (startTokenRange.first < tokenIdStart) {
            AddRange(propertyId, startTokenRange.first, tokenIdStart - 1, from);
        }
        if (endTokenRange.second > tokenIdEnd) {
            AddRange(propertyId, tokenIdEnd + 1, endTokenRange.second, from);
        }
    }

    // adjust 'to' ranges
    if (bToAdjacentRangeBefore == false && bToAdjacentRangeAfter == false) {
        AddRange(propertyId, tokenIdStart, tokenIdEnd, to);
    } else {
        int64_t newTokenIdStart = tokenIdStart;
        int64_t newTokenIdEnd = tokenIdEnd;
        if (bToAdjacentRangeBefore) {
            std::pair<int64_t,int64_t> oldRange = GetRange(propertyId, tokenIdStart-1);
            newTokenIdStart = oldRange.first;
            DeleteRange(propertyId, oldRange.first, oldRange.second);
        }
        if (bToAdjacentRangeAfter) {
            std::pair<int64_t,int64_t> oldRange = GetRange(propertyId, tokenIdEnd+1);
            newTokenIdEnd = oldRange.second;
            DeleteRange(propertyId, oldRange.first, oldRange.second);
        }
        AddRange(propertyId, newTokenIdStart, newTokenIdEnd, to);
    }

    return true;
}

/* Counts the highest token range end (which is thus the total number of tokens)
 */
int64_t CMPUniqueTokensDB::GetHighestRangeEnd(const uint32_t &propertyId)
{
    assert(pdb);

    int64_t tokenCount = 0;
    leveldb::Iterator* it = NewIterator();
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        std::string key = it->key().ToString();
        std::vector<std::string> vPropertyId;
        boost::split(vPropertyId, key, boost::is_any_of("_"), boost::token_compress_on);
        if (2 != vPropertyId.size()) continue; // unexpected - TODO: log this error
        std::string strPropId = vPropertyId[0];
        strPropId.erase(0, strPropId.find_first_not_of('0'));
        uint32_t dbPropertyId = boost::lexical_cast<uint32_t>(strPropId);
        if (dbPropertyId != propertyId) continue;
        std::vector<std::string> vRanges;
        boost::split(vRanges, vPropertyId[1], boost::is_any_of("-"), boost::token_compress_on);
        if (2 != vRanges.size()) continue; // unexpected - TODO: log this error
        std::string strIdEnd = vRanges[1];
        strIdEnd.erase(0, strIdEnd.find_first_not_of('0'));
        int64_t dbTokenIdEnd = boost::lexical_cast<int64_t>(strIdEnd);
        if (dbTokenIdEnd > tokenCount) {
            tokenCount = dbTokenIdEnd;
        }
    }
    delete it;
    return tokenCount;
}

/* Deletes a range of unique tokens
 */
void CMPUniqueTokensDB::DeleteRange(const uint32_t &propertyId, const int64_t &tokenIdStart, const int64_t &tokenIdEnd)
{
    assert(pdb);
    const std::string key = strprintf("%010d_%020d-%020d", propertyId, tokenIdStart, tokenIdEnd);
    pdb->Delete(leveldb::WriteOptions(), key);
//    if (msc_debug_utdb) PrintToLog("%s():%s, line %d, file: %s\n", __FUNCTION__, key, __LINE__, __FILE__);
}

/* Adds a range of unique tokens
 */
void CMPUniqueTokensDB::AddRange(const uint32_t &propertyId, const int64_t &tokenIdStart, const int64_t &tokenIdEnd, const std::string &owner)
{
    assert(pdb);

    const std::string key = strprintf("%010d_%020d-%020d", propertyId, tokenIdStart, tokenIdEnd);
    leveldb::Status status = pdb->Put(writeoptions, key, owner);
    ++nWritten;
//    if (msc_debug_utdb) PrintToLog("%s():%s=%s:%s, line %d, file: %s\n", __FUNCTION__, key, owner, status.ToString(), __LINE__, __FILE__);
}

/* Creates a range of unique tokens
 */
std::pair<int64_t,int64_t> CMPUniqueTokensDB::CreateUniqueTokens(const uint32_t &propertyId, const int64_t &amount, const std::string &owner)
{
    int64_t highestId = GetHighestRangeEnd(propertyId);
    int64_t newTokenStartId = highestId + 1;
    int64_t newTokenEndId = highestId + amount; // TODO: overflow protection
    std::pair<int64_t,int64_t> newRange = std::make_pair(newTokenStartId, newTokenEndId);
    std::string highestRangeOwner = GetUniqueTokenOwner(propertyId, highestId);
    if (highestRangeOwner == owner) {
        std::pair<int64_t,int64_t> oldRange = GetRange(propertyId, highestId);
        DeleteRange(propertyId, oldRange.first, oldRange.second);
        newTokenStartId = oldRange.first; // override range start to merge ranges from same owner
    }
    AddRange(propertyId, newTokenStartId, newTokenEndId, owner);
    return newRange;
}


/* Gets the owner of a range of unique tokens
 *
 * Note: If the specified range is not owned in it's entirety by a single owner (or unassigned) an empty string is returned
 */
std::string CMPUniqueTokensDB::GetUniqueTokenOwner(const uint32_t &propertyId, const int64_t &tokenId)
{
    assert(pdb);
    leveldb::Iterator* it = NewIterator();
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        std::string key = it->key().ToString();
        // break into component integers
        std::vector<std::string> vPropertyId;
        boost::split(vPropertyId, key, boost::is_any_of("_"), boost::token_compress_on);
        if (2 != vPropertyId.size()) continue; // unexpected - TODO: log this error
        std::string strPropId = vPropertyId[0];
        strPropId.erase(0, strPropId.find_first_not_of('0'));
        uint32_t dbPropertyId = boost::lexical_cast<uint32_t>(strPropId);
        if (dbPropertyId != propertyId) continue;
        std::vector<std::string> vRanges;
        boost::split(vRanges, vPropertyId[1], boost::is_any_of("-"), boost::token_compress_on);
        if (2 != vRanges.size()) continue; // unexpected - TODO: log this error
        std::string strIdStart = vRanges[0];
        std::string strIdEnd = vRanges[1];
        strIdStart.erase(0, strIdStart.find_first_not_of('0'));
        strIdEnd.erase(0, strIdEnd.find_first_not_of('0'));
        int64_t dbTokenIdStart = boost::lexical_cast<int64_t>(strIdStart);
        int64_t dbTokenIdEnd = boost::lexical_cast<int64_t>(strIdEnd);
        // check if the ID supplied to the function is within this range in the DB
        if (tokenId >= dbTokenIdStart && tokenId <= dbTokenIdEnd) {
            return it->value().ToString();
        }
    }

    delete it;
    return ""; // not found
}

void CMPUniqueTokensDB::printStats()
{
    PrintToLog("CMPTxList stats: nWritten= %d , nRead= %d\n", nWritten, nRead);
}

void CMPUniqueTokensDB::printAll()
{
    int count = 0;
    leveldb::Slice skey, svalue;
    leveldb::Iterator* it = NewIterator();

    for(it->SeekToFirst(); it->Valid(); it->Next()) {
        skey = it->key();
        svalue = it->value();
        ++count;
        PrintToConsole("entry #%8d= %s:%s\n", count, skey.ToString(), svalue.ToString());
    }

    delete it;
}

