#include "omnicore/notifications.h"
#include "omnicore/version.h"

#include "util.h"
#include "test/test_bitcoin.h"
#include "tinyformat.h"

#include <boost/test/unit_test.hpp>

#include <stdint.h>
#include <map>
#include <string>
#include <vector>

using namespace mastercore;

// Is only temporarily modified and restored after each test
extern std::map<std::string, std::string> mapArgs;
extern std::map<std::string, std::vector<std::string> > mapMultiArgs;

BOOST_FIXTURE_TEST_SUITE(omnicore_alert_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(alert_positive_authorization)
{
    // Confirm authorized sources for mainnet
    BOOST_CHECK(CheckAlertAuthorization("LgcdyjEUH822bqqq1YwJFoBrWT3SrHPd8P"));  // Zathras <zathras@omni.foundation>
    BOOST_CHECK(CheckAlertAuthorization("LbVT6BCkqexvBiVvbaH4x4sarytigihCtF"));  // dexX7   <dexx@bitwatch.co>
    // BOOST_CHECK(CheckAlertAuthorization("TODO")); // Craig   <craig@omni.foundation>
    // BOOST_CHECK(CheckAlertAuthorization("TODO")); // Adam    <adam@omni.foundation>
}

BOOST_AUTO_TEST_CASE(alert_unauthorized_source)
{
    // Confirm unauthorized sources are not accepted
    BOOST_CHECK(!CheckAlertAuthorization("LPxYdTq2dbExmdnUTYwYmWtRy2zpBDyVHy"));
}

BOOST_AUTO_TEST_CASE(alert_manual_sources)
{
    std::map<std::string, std::string> mapArgsOriginal = mapArgs;
    std::map<std::string, std::vector<std::string> > mapMultiArgsOriginal = mapMultiArgs;

    mapArgs["-omnialertallowsender"] = "";
    mapArgs["-omnialertignoresender"] = "";

    // Add LPxYdT as allowed source for alerts
    mapMultiArgs["-omnialertallowsender"].push_back("LPxYdTq2dbExmdnUTYwYmWtRy2zpBDyVHy");
    BOOST_CHECK(CheckAlertAuthorization("LPxYdTq2dbExmdnUTYwYmWtRy2zpBDyVHy"));

    // Then ignore a source explicitly
    mapMultiArgs["-omnialertignoresender"].push_back("LgcdyjEUH822bqqq1YwJFoBrWT3SrHPd8P");
    BOOST_CHECK(CheckAlertAuthorization("LbVT6BCkqexvBiVvbaH4x4sarytigihCtF")); // should still be authorized
    BOOST_CHECK(!CheckAlertAuthorization("LgcdyjEUH822bqqq1YwJFoBrWT3SrHPd8P"));

    mapMultiArgs = mapMultiArgsOriginal;
    mapArgs = mapArgsOriginal;
}

BOOST_AUTO_TEST_CASE(alert_authorize_any_source)
{
    std::map<std::string, std::string> mapArgsOriginal = mapArgs;
    std::map<std::string, std::vector<std::string> > mapMultiArgsOriginal = mapMultiArgs;

    mapArgs["-omnialertallowsender"] = "";

    // Allow any source (e.g. for tests!)
    mapMultiArgs["-omnialertallowsender"].push_back("any");
    BOOST_CHECK(CheckAlertAuthorization("LgcdyjEUH822bqqq1YwJFoBrWT3SrHPd8P"));
    BOOST_CHECK(CheckAlertAuthorization("LddZvzT8mpgSAx7gm78TrwWbPNp7PreDRf"));

    mapMultiArgs = mapMultiArgsOriginal;
    mapArgs = mapArgsOriginal;
}

BOOST_AUTO_TEST_SUITE_END()
