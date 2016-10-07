#ifndef OMNICORE_OMNICORE_H
#define OMNICORE_OMNICORE_H

/** Global handler to initialize Omni Core. */
int mastercore_init();

/** Global handler to shut down Omni Core. */
int mastercore_shutdown();

/** Global handler to total wallet balances. */
void CheckWalletUpdate(bool forceUpdate = false);


#endif // OMNICORE_OMNICORE_H
