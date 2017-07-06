#include "omnicore/createpayload.h"

#include "test/test_bitcoin.h"
#include "utilstrencodings.h"

#include <boost/test/unit_test.hpp>

#include <stdint.h>
#include <vector>
#include <string>

BOOST_FIXTURE_TEST_SUITE(omnicore_create_payload_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(payload_simple_send)
{
    // Simple send [type 0, version 0]
    std::vector<unsigned char> vch = CreatePayload_SimpleSend(
        static_cast<uint32_t>(1),          // property: MSC
        static_cast<int64_t>(100000000));  // amount to transfer: 1.0 MSC (in willets)

    BOOST_CHECK_EQUAL(HexStr(vch), "00000180c2d72f");
}

BOOST_AUTO_TEST_CASE(payload_send_all)
{
    // Send to owners [type 4, version 0]
    std::vector<unsigned char> vch = CreatePayload_SendAll(
        static_cast<uint8_t>(2));          // ecosystem: Test

    BOOST_CHECK_EQUAL(HexStr(vch), "000402");
}

BOOST_AUTO_TEST_CASE(payload_create_property)
{
    // Create property [type 50, version 0]
    std::vector<unsigned char> vch = CreatePayload_IssuanceFixed(
        static_cast<uint8_t>(1),             // ecosystem: main
        static_cast<uint16_t>(1),            // property type: indivisible tokens
        static_cast<uint32_t>(0),            // previous property: none
        std::string("Quantum Miner"),        // label
        std::string("builder.bitwatch.co"),  // website
        std::string(""),                     // additional information
        static_cast<int64_t>(1000000));      // number of units to create

    BOOST_CHECK_EQUAL(HexStr(vch),
        "00320101005175616e74756d204d696e6572006275696c6465722e6269747761746368"
        "2e636f0000c0843d");
}

BOOST_AUTO_TEST_CASE(payload_create_property_empty)
{
    // Create property [type 50, version 0]
    std::vector<unsigned char> vch = CreatePayload_IssuanceFixed(
        static_cast<uint8_t>(1),         // ecosystem: main
        static_cast<uint16_t>(1),        // property type: indivisible tokens
        static_cast<uint32_t>(0),        // previous property: none
        std::string(""),                 // label
        std::string(""),                 // website
        std::string(""),                 // additional information
        static_cast<int64_t>(1000000));  // number of units to create

    BOOST_CHECK_EQUAL(vch.size(), 11);
}

BOOST_AUTO_TEST_CASE(payload_create_property_full)
{
    // Unlikely to be able to send a full payload like this due to datacarrier rules, but maintain test

    // Create property [type 50, version 0]
    std::vector<unsigned char> vch = CreatePayload_IssuanceFixed(
        static_cast<uint8_t>(1),         // ecosystem: main
        static_cast<uint16_t>(1),        // property type: indivisible tokens
        static_cast<uint32_t>(0),        // previous property: none
        std::string(700, 'x'),           // label
        std::string(700, 'x'),           // website
        std::string(700, 'x'),           // additional information
        static_cast<int64_t>(1000000));  // number of units to create

    BOOST_CHECK_EQUAL(vch.size(), 776);
}

BOOST_AUTO_TEST_CASE(payload_create_crowdsale)
{
    // Create crowdsale [type 51, version 0]
    std::vector<unsigned char> vch = CreatePayload_IssuanceVariable(
        static_cast<uint8_t>(1),             // ecosystem: main
        static_cast<uint16_t>(1),            // property type: indivisible tokens
        static_cast<uint32_t>(0),            // previous property: none
        std::string("Quantum Miner"),        // label
        std::string("builder.bitwatch.co"),  // website
        std::string(""),                     // additional information
        static_cast<uint32_t>(1),            // property desired: MSC
        static_cast<int64_t>(100),           // tokens per unit vested
        static_cast<uint64_t>(7731414000L),  // deadline: 31 Dec 2214 23:00:00 UTC
        static_cast<uint8_t>(10),            // early bird bonus: 10 % per week
        static_cast<uint8_t>(12));           // issuer bonus: 12 %

    BOOST_CHECK_EQUAL(HexStr(vch),

        "00330101005175616e74756d204d696e6572006275696c6465722e6269747761746368"
        "2e636f00000164f087d0e61c0a0c");
}

BOOST_AUTO_TEST_CASE(payload_create_crowdsale_empty)
{
    // Create crowdsale [type 51, version 0]
    std::vector<unsigned char> vch = CreatePayload_IssuanceVariable(
        static_cast<uint8_t>(1),            // ecosystem: main
        static_cast<uint16_t>(1),           // property type: indivisible tokens
        static_cast<uint32_t>(0),           // previous property: none
        std::string(""),                    // label
        std::string(""),                    // website
        std::string(""),                    // additional information
        static_cast<uint32_t>(1),           // property desired: MSC
        static_cast<int64_t>(100),          // tokens per unit vested
        static_cast<uint64_t>(7731414000L), // deadline: 31 Dec 2214 23:00:00 UTC
        static_cast<uint8_t>(10),           // early bird bonus: 10 % per week
        static_cast<uint8_t>(12));          // issuer bonus: 12 %

    BOOST_CHECK_EQUAL(vch.size(), 17);
}

BOOST_AUTO_TEST_CASE(payload_create_crowdsale_full)
{
    // Unlikely to be able to send a full payload like this due to datacarrier rules, but maintain test

    // Create crowdsale [type 51, version 0]
    std::vector<unsigned char> vch = CreatePayload_IssuanceVariable(
        static_cast<uint8_t>(1),            // ecosystem: main
        static_cast<uint16_t>(1),           // property type: indivisible tokens
        static_cast<uint32_t>(0),           // previous property: none
        std::string(700, 'x'),              // label
        std::string(700, 'x'),              // website
        std::string(700, 'x'),              // additional information
        static_cast<uint32_t>(1),           // property desired: MSC
        static_cast<int64_t>(100),          // tokens per unit vested
        static_cast<uint64_t>(7731414000L), // deadline: 31 Dec 2214 23:00:00 UTC
        static_cast<uint8_t>(10),           // early bird bonus: 10 % per week
        static_cast<uint8_t>(12));          // issuer bonus: 12 %

    BOOST_CHECK_EQUAL(vch.size(), 782);
}

BOOST_AUTO_TEST_CASE(payload_close_crowdsale)
{
    // Close crowdsale [type 53, version 0]
    std::vector<unsigned char> vch = CreatePayload_CloseCrowdsale(
        static_cast<uint32_t>(9));  // property: SP #9

    BOOST_CHECK_EQUAL(HexStr(vch), "003509");
}

BOOST_AUTO_TEST_CASE(payload_create_managed_property)
{
    // create managed property [type 54, version 0]
    std::vector<unsigned char> vch = CreatePayload_IssuanceManaged(
        static_cast<uint8_t>(1),             // ecosystem: main
        static_cast<uint16_t>(1),            // property type: indivisible tokens
        static_cast<uint32_t>(0),            // previous property: none
        std::string("Quantum Miner"),        // label
        std::string("builder.bitwatch.co"),  // website
        std::string(""));                    // additional information

    BOOST_CHECK_EQUAL(HexStr(vch),
        "00360101005175616e74756d204d696e6572006275696c6465722e6269747761746368"
        "2e636f0000");
}

BOOST_AUTO_TEST_CASE(payload_create_managed_property_empty)
{
    // create managed property [type 54, version 0]
    std::vector<unsigned char> vch = CreatePayload_IssuanceManaged(
        static_cast<uint8_t>(1),   // ecosystem: main
        static_cast<uint16_t>(1),  // property type: indivisible tokens
        static_cast<uint32_t>(0),  // previous property: none
        std::string(""),           // label
        std::string(""),           // website
        std::string(""));          // additional information

    BOOST_CHECK_EQUAL(vch.size(), 8);
}

BOOST_AUTO_TEST_CASE(payload_create_managed_property_full)
{
    // Unlikely to be able to send a full payload like this due to datacarrier rules, but maintain test

    // create managed property [type 54, version 0]
    std::vector<unsigned char> vch = CreatePayload_IssuanceManaged(
        static_cast<uint8_t>(1),   // ecosystem: main
        static_cast<uint16_t>(1),  // property type: indivisible tokens
        static_cast<uint32_t>(0),  // previous property: none
        std::string(700, 'x'),     // label
        std::string(700, 'x'),     // website
        std::string(700, 'x'));    // additional information

    BOOST_CHECK_EQUAL(vch.size(), 773);
}

BOOST_AUTO_TEST_CASE(payload_grant_tokens)
{
    // Grant tokens [type 55, version 0]
    std::vector<unsigned char> vch = CreatePayload_Grant(
        static_cast<uint32_t>(8),                  // property: SP #8
        static_cast<int64_t>(1000));               // number of units to issue

    BOOST_CHECK_EQUAL(HexStr(vch), "003708e807");
}

BOOST_AUTO_TEST_CASE(payload_revoke_tokens)
{
    // Revoke tokens [type 56, version 0]
    std::vector<unsigned char> vch = CreatePayload_Revoke(
        static_cast<uint32_t>(8),                                   // property: SP #8
        static_cast<int64_t>(1000));                                // number of units to revoke

    BOOST_CHECK_EQUAL(HexStr(vch), "003808e807");
}

BOOST_AUTO_TEST_CASE(payload_change_property_manager)
{
    // Change property manager [type 70, version 0]
    std::vector<unsigned char> vch = CreatePayload_ChangeIssuer(
        static_cast<uint32_t>(13));  // property: SP #13

    BOOST_CHECK_EQUAL(HexStr(vch), "00460d");
}

BOOST_AUTO_TEST_CASE(payload_feature_deactivation)
{
    // Omni Core feature activation [type 65533, version 65535]
    std::vector<unsigned char> vch = CreatePayload_DeactivateFeature(
        static_cast<uint16_t>(1));        // feature identifier: 1 (OP_RETURN)

    BOOST_CHECK_EQUAL(HexStr(vch), "ffff03fdff0301");
}

BOOST_AUTO_TEST_CASE(payload_feature_activation)
{
    // Omni Core feature activation [type 65534, version 65535]
    std::vector<unsigned char> vch = CreatePayload_ActivateFeature(
        static_cast<uint16_t>(1),        // feature identifier: 1 (OP_RETURN)
        static_cast<uint32_t>(370000),   // activation block
        static_cast<uint32_t>(999));     // min client version

    BOOST_CHECK_EQUAL(HexStr(vch), "ffff03feff0301d0ca16e707");
}

BOOST_AUTO_TEST_CASE(payload_omnicore_alert_block)
{
    // Omni Core client notification [type 65535, version 65535]
    std::vector<unsigned char> vch = CreatePayload_OmniCoreAlert(
        static_cast<int32_t>(1),            // alert target: by block number
        static_cast<uint64_t>(300000),      // expiry value: 300000
        static_cast<std::string>("test"));  // alert message: test

    BOOST_CHECK_EQUAL(HexStr(vch), "ffff03ffff0301e0a7127465737400");
}

BOOST_AUTO_TEST_CASE(payload_omnicore_alert_blockexpiry)
{
    // Omni Core client notification [type 65535, version 65535]
    std::vector<unsigned char> vch = CreatePayload_OmniCoreAlert(
        static_cast<int32_t>(2),            // alert target: by block time
        static_cast<uint64_t>(1439528630),  // expiry value: 1439528630
        static_cast<std::string>("test"));  // alert message: test

    BOOST_CHECK_EQUAL(HexStr(vch), "ffff03ffff0302b6edb5ae057465737400");
}

BOOST_AUTO_TEST_CASE(payload_omnicore_alert_minclient)
{
    // Omni Core client notification [type 65535, version 65535]
    std::vector<unsigned char> vch = CreatePayload_OmniCoreAlert(
        static_cast<int32_t>(3),            // alert target: by client version
        static_cast<uint64_t>(900100),      // expiry value: v0.0.9.1
        static_cast<std::string>("test"));  // alert message: test

    BOOST_CHECK_EQUAL(HexStr(vch), "ffff03ffff030384f8367465737400");
}

BOOST_AUTO_TEST_SUITE_END()
