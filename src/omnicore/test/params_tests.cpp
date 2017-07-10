#include "omnicore/omnicore.h"
#include "omnicore/rules.h"

#include "chainparams.h"
#include "test/test_bitcoin.h"

#include <boost/test/unit_test.hpp>

#include <stdint.h>
#include <string>

using namespace mastercore;

BOOST_FIXTURE_TEST_SUITE(omnicore_params_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(network_restrictions_main)
{
    const CConsensusParams& params = ConsensusParams("main");
    BOOST_CHECK_EQUAL(params.MSC_SEND_ALL_BLOCK, 9999999);
}

BOOST_AUTO_TEST_CASE(network_restrictions_test)
{
    const CConsensusParams& params = ConsensusParams("test");
    BOOST_CHECK_EQUAL(params.MSC_SEND_ALL_BLOCK, 0);
}

BOOST_AUTO_TEST_CASE(ecosystem_restrictions_main)
{
    // Unit tests and mainnet use the same params
    BOOST_CHECK(!IsTransactionTypeAllowed(0, OMNI_ECOSYSTEM_MAIN, MSC_TYPE_SEND_ALL, MP_TX_PKT_V0));
    BOOST_CHECK(IsTransactionTypeAllowed(0, OMNI_ECOSYSTEM_TEST, MSC_TYPE_SEND_ALL, MP_TX_PKT_V0));
}

BOOST_AUTO_TEST_CASE(ecosystem_restrictions_test)
{
    SelectParams(CBaseChainParams::TESTNET);
    BOOST_CHECK(IsTransactionTypeAllowed(0, OMNI_ECOSYSTEM_MAIN, MSC_TYPE_SEND_ALL, MP_TX_PKT_V0));
    BOOST_CHECK(IsTransactionTypeAllowed(0, OMNI_ECOSYSTEM_TEST, MSC_TYPE_SEND_ALL, MP_TX_PKT_V0));
    // Restore original
    SelectParams(CBaseChainParams::MAIN);
}

BOOST_AUTO_TEST_CASE(update_feature_network)
{
    const std::string& network = Params().NetworkIDString();

    int oldActivationBlock = ConsensusParams(network).MSC_SEND_ALL_BLOCK;
    int newActivationBlock = 123;

    // Before updated
    BOOST_CHECK(oldActivationBlock != newActivationBlock);
    BOOST_CHECK_EQUAL(oldActivationBlock, ConsensusParams().MSC_SEND_ALL_BLOCK);
    BOOST_CHECK(!IsTransactionTypeAllowed(newActivationBlock, OMNI_ECOSYSTEM_MAIN, MSC_TYPE_SEND_ALL, MP_TX_PKT_V0));

    // Update
    ConsensusParams(network).MSC_SEND_ALL_BLOCK = newActivationBlock;
    BOOST_CHECK_EQUAL(newActivationBlock, ConsensusParams().MSC_SEND_ALL_BLOCK);
    BOOST_CHECK(IsTransactionTypeAllowed(newActivationBlock, OMNI_ECOSYSTEM_MAIN, MSC_TYPE_SEND_ALL, MP_TX_PKT_V0));

    // Restore original
    ConsensusParams(network).MSC_SEND_ALL_BLOCK = oldActivationBlock;
    BOOST_CHECK_EQUAL(oldActivationBlock, ConsensusParams().MSC_SEND_ALL_BLOCK);
}

BOOST_AUTO_TEST_CASE(update_feature)
{
    int oldActivationBlock = ConsensusParams().MSC_SEND_ALL_BLOCK;
    int newActivationBlock = 123;

    // Before updated
    BOOST_CHECK(oldActivationBlock != newActivationBlock);
    BOOST_CHECK(!IsTransactionTypeAllowed(newActivationBlock, OMNI_ECOSYSTEM_MAIN, MSC_TYPE_SEND_ALL, MP_TX_PKT_V0));

    // Update
    MutableConsensusParams().MSC_SEND_ALL_BLOCK = newActivationBlock;
    BOOST_CHECK_EQUAL(newActivationBlock, ConsensusParams().MSC_SEND_ALL_BLOCK);
    BOOST_CHECK(IsTransactionTypeAllowed(newActivationBlock, OMNI_ECOSYSTEM_MAIN, MSC_TYPE_SEND_ALL, MP_TX_PKT_V0));

    // Restore original
    MutableConsensusParams().MSC_SEND_ALL_BLOCK = oldActivationBlock;
    BOOST_CHECK_EQUAL(oldActivationBlock, ConsensusParams().MSC_SEND_ALL_BLOCK);
}


BOOST_AUTO_TEST_SUITE_END()
