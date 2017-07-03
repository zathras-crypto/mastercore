#include "omnicore/consensushash.h"
#include "omnicore/sp.h"
#include "omnicore/omnicore.h"
#include "omnicore/rules.h"
#include "omnicore/tally.h"

#include "arith_uint256.h"
#include "sync.h"
#include "test/test_bitcoin.h"
#include "uint256.h"

#include <boost/test/unit_test.hpp>

#include <stdint.h>
#include <string>

namespace mastercore
{
extern std::string GenerateConsensusString(const CMPTally& tallyObj, const std::string& address, const uint32_t propertyId); // done
extern std::string GenerateConsensusString(const CMPCrowd& crowdObj);
extern std::string GenerateConsensusString(const uint32_t propertyId, const std::string& address);
}

extern void clear_all_state();

using namespace mastercore;

BOOST_FIXTURE_TEST_SUITE(omnicore_checkpoint_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(consensus_string_tally)
{
    CMPTally tally;
    BOOST_CHECK_EQUAL("", GenerateConsensusString(tally, "LPxYdTq2dbExmdnUTYwYmWtRy2zpBDyVHy", 1));
    BOOST_CHECK_EQUAL("", GenerateConsensusString(tally, "LPxYdTq2dbExmdnUTYwYmWtRy2zpBDyVHy", 3));

    BOOST_CHECK(tally.updateMoney(3, 7, BALANCE));
    BOOST_CHECK_EQUAL("LPxYdTq2dbExmdnUTYwYmWtRy2zpBDyVHy|3|7",
            GenerateConsensusString(tally, "LPxYdTq2dbExmdnUTYwYmWtRy2zpBDyVHy", 3));

    BOOST_CHECK(tally.updateMoney(3, 7, BALANCE));
    BOOST_CHECK(tally.updateMoney(3, (-int64_t(9223372036854775807LL)-1), PENDING)); // ignored
    BOOST_CHECK_EQUAL("LPxYdTq2dbExmdnUTYwYmWtRy2zpBDyVHy|3|14",
            GenerateConsensusString(tally, "LPxYdTq2dbExmdnUTYwYmWtRy2zpBDyVHy", 3));
}

BOOST_AUTO_TEST_CASE(consensus_string_crowdsale)
{
    CMPCrowd crowdsaleA;
    BOOST_CHECK_EQUAL("0|0|0|0|0",
            GenerateConsensusString(crowdsaleA));

    CMPCrowd crowdsaleB(77, 500000, 3, 1514764800, 10, 255, 10000, 25500);
    BOOST_CHECK_EQUAL("77|3|1514764800|10000|25500",
            GenerateConsensusString(crowdsaleB));
}

BOOST_AUTO_TEST_CASE(consensus_string_property_issuer)
{
    BOOST_CHECK_EQUAL("5|LPxYdTq2dbExmdnUTYwYmWtRy2zpBDyVHy",
            GenerateConsensusString(5, "LPxYdTq2dbExmdnUTYwYmWtRy2zpBDyVHy"));
}

BOOST_AUTO_TEST_CASE(get_checkpoints)
{
    // TODO - Re-enable this test once there are checkpoints on the Litecoin network
    // There are consensus checkpoints for mainnet:
    // BOOST_CHECK(!ConsensusParams("main").GetCheckpoints().empty());

    // ... but no checkpoints for regtest mode are defined:
    BOOST_CHECK(ConsensusParams("regtest").GetCheckpoints().empty());
}


BOOST_AUTO_TEST_SUITE_END()
