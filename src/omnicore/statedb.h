#ifndef OMNICORE_STATEDB_H
#define OMNICORE_STATEDB_H

#include "leveldb/db.h"

#include "omnicore/log.h"
#include "omnicore/persistence.h"

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
