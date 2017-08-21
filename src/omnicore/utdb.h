#ifndef OMNICORE_UTDB_H
#define OMNICORE_UTDB_H

#include "leveldb/db.h"

#include "omnicore/log.h"
#include "omnicore/persistence.h"

#include <stdint.h>
#include <boost/filesystem.hpp>

/** LevelDB based storage for unique tokens, with uid range (propertyid_tokenidstart-tokenidend) as key and token owner (address) as value.
 */
class CMPUniqueTokensDB : public CDBBase
{
// helper for setting owner on range of tokens
// helper for getting owner on range of tokens
// helper for checking if owner=x on range of tokens
// helper for checking if token exists
// helper for getting all tokens an owner has

public:
    CMPUniqueTokensDB(const boost::filesystem::path& path, bool fWipe)
    {
        leveldb::Status status = Open(path, fWipe);
        PrintToConsole("Loading unique tokens database: %s\n", status.ToString());
    }

    virtual ~CMPUniqueTokensDB()
    {
        if (msc_debug_persistence) PrintToLog("CMPUniqueTokensDB closed\n");
    }

    void printStats();
    void printAll();

    // Gets the owner of a range of unique tokens
    std::string GetUniqueTokenOwner(const uint32_t &propertyId, const int64_t &tokenId);
    // Checks if the range of tokens is contiguous (ie owned by a single address)
    bool IsRangeContiguous(const uint32_t &propertyId, const int64_t &tokenIdStart, const int64_t &tokenIdEnd);
    // Counts the highest token range end (which is thus the total number of tokens)
    int64_t GetHighestRangeEnd(const uint32_t &propertyId);
    // Creates a range of unique tokens
    std::pair<int64_t,int64_t> CreateUniqueTokens(const uint32_t &propertyId, const int64_t &amount, const std::string &owner);
    // Gets the range a unique token is in
    std::pair<int64_t,int64_t> GetRange(const uint32_t &propertyId, const int64_t &tokenId);
    // Deletes a range of unique tokens
    void DeleteRange(const uint32_t &propertyId, const int64_t &tokenIdStart, const int64_t &tokenIdEnd);
    // Moves a range of unique tokens
    bool MoveUniqueTokens(const uint32_t &propertyId, const int64_t &tokenIdStart, const int64_t &tokenIdEnd, const std::string &from, const std::string &to);
    // Adds a range of unique tokens
    void AddRange(const uint32_t &propertyId, const int64_t &tokenIdStart, const int64_t &tokenIdEnd, const std::string &owner);
    // Gets the unique token ranges for a property ID and address
    std::vector<std::pair<int64_t,int64_t> > GetAddressUniqueTokens(const uint32_t &propertyId, const std::string &address);

};

namespace mastercore
{
extern CMPUniqueTokensDB *p_utdb;
}

#endif // OMNICORE_UTDB_H
