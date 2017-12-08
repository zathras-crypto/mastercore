#ifndef OMNICORE_STATEDB_H
#define OMNICORE_STATEDB_H

#include "leveldb/db.h"

#include "omnicore/log.h"
#include "omnicore/persistence.h"

#include "uint256.h"

#include <boost/filesystem.hpp>

/** LevelDB based storage for the database of state actions
 */
class COmniStateDB : public CDBBase
{
public:
    COmniStateDB(const boost::filesystem::path& path, bool fWipe)
    {
        leveldb::Status status = Open(path, fWipe);
        PrintToConsole("Loading state actions database: %s\n", status.ToString());
    }

    virtual ~COmniStateDB()
    {
        PrintToLog("COmniStateDB closed\n");
    }

    // Get the next sequence number
    int64_t getNextSequenceNo();
    // Write a new state entry
    void writeStateEntry(const uint256& txid, int block, const std::string& action);

    // Show DB statistics
    void printStats();
    // Show DB records
    void printAll();
};

namespace mastercore
{
    extern COmniStateDB *p_stateDB;
}

#endif // OMNICORE_STATEDB_H
