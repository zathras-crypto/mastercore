Omni Core version 0.0.9.1-rel is now available from:

  {insert URL once ready to release}

0.0.9.1-rel is a minor version release.  It is not a required upgrade from 0.0.9-rel, however if you have not already upgraded to 0.0.9-rel or above you must do so immediately (versions prior to 0.0.9-rel are already out of consensus and will provide incorrect data).

Please report bugs using the issue tracker at github:

  https://github.com/mastercoin-MSC/mastercore/issues

IMPORTANT
=========

- This is the first experimental release of Omni Layer support in the QT UI, please be vigilant with testing and do not risk large amounts of Bitcoin and Omni Layer tokens.
- The transaction index is no longer defaulted to enabled.  You will need to ensure you have "txindex=1" (without the quotes) in your configuration file.
- If you are upgrading from a version earlier than 0.0.9-rel you must start with the --startclean parameter at least once to refresh your persistence files.
- The first time Omni Core is run the startup process may take an hour or more as existing Omni Layer transactions are parsed.  This is normal and should only be required the first time Omni Core is run.

CHANGELOG
=========
GENERAL
-------
- Extra console debugging removed
- Bitcoin 0.10 blockchain detection (will refuse to start if out of order block storage is detected)
- txindex default value now matches Bitcoin Core (false)
- Update authorized alert senders
- Added support for TX70 to RPC output
- Fix missing LOCK of cs_main in selectCoins()

UI
--
- New signal added for changes to Omni state (emitted from block handler for blocks containing Omni transactions)
- Fix double clicking a transaction in overview does not activate the Bitcoin history tab
- Splash screen updated to reflect new branding
- Fix frame alignment in overview page
- Update send page behaviour and layout per feedback
- Fix column resizing on balances tab
- Right align amounts in balances tab
- Various rebranding to Omni Core
- Rewritten Omni transaction history tab
- Add protection against long labels growing the UI size to ridiculous proportions
- Update signalling to all Omni pages to ensure up to date info
- Override display of Mastercoin metadata for rebrand (RPC unchanged)
- Acknowledgement of disclaimer will now be remembered
