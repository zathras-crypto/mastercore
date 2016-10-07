/**
 * @file omnicore.cpp
 *
 * This file contains the core of Omni Core.
 */

#include "omnicore/omnicore.h"

#include "omnicore/log.h"


/**
 * Global handler to initialize Omni Core.
 *
 * @return An exit code, indicating success or failure
 */
int mastercore_init()
{
    // TODO
    PrintToConsole("Omni Core initialization completed\n");

    return 0;
}

/**
 * Global handler to shut down Omni Core.
 *
 * In particular, the LevelDB databases of the global state objects are closed
 * properly.
 *
 * @return An exit code, indicating success or failure
 */
int mastercore_shutdown()
{
    // TODO
    PrintToConsole("Omni Core shutdown completed\n");

    return 0;
}

void CheckWalletUpdate(bool forceUpdate)
{
    // TODO
}
