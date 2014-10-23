#include "mastercore_convert.h"

#include <stdint.h>

#include <boost/test/unit_test.hpp>

using namespace mastercore;

BOOST_AUTO_TEST_SUITE(mastercore_rounduint64_tests)

BOOST_AUTO_TEST_CASE(mastercore_rounduint64_simple)
{
    const int64_t COIN = 100000000;
    double unit_price = 23.45678923999;
    BOOST_CHECK_EQUAL(UINT64_C(2345678924), rounduint64(COIN * unit_price));
}

BOOST_AUTO_TEST_CASE(mastercore_rounduint64_whole_units)
{
    BOOST_CHECK_EQUAL(UINT64_C(0), rounduint64(0.0));
    BOOST_CHECK_EQUAL(UINT64_C(1), rounduint64(1.0));
    BOOST_CHECK_EQUAL(UINT64_C(2), rounduint64(2.0));
    BOOST_CHECK_EQUAL(UINT64_C(3), rounduint64(3.0));
}

BOOST_AUTO_TEST_CASE(mastercore_rounduint64_round_below_point_5)
{    
    BOOST_CHECK_EQUAL(UINT64_C(0), rounduint64(0.49999999));
    BOOST_CHECK_EQUAL(UINT64_C(1), rounduint64(1.49999999));
    BOOST_CHECK_EQUAL(UINT64_C(2), rounduint64(2.49999999));
    BOOST_CHECK_EQUAL(UINT64_C(3), rounduint64(3.49999999));
}

BOOST_AUTO_TEST_CASE(mastercore_rounduint64_round_point_5)
{    
    BOOST_CHECK_EQUAL(UINT64_C(1), rounduint64(0.5));
    BOOST_CHECK_EQUAL(UINT64_C(2), rounduint64(1.5));
    BOOST_CHECK_EQUAL(UINT64_C(3), rounduint64(2.5));
    BOOST_CHECK_EQUAL(UINT64_C(4), rounduint64(3.5));
}

BOOST_AUTO_TEST_CASE(mastercore_rounduint64_round_over_point_5)
{    
    BOOST_CHECK_EQUAL(UINT64_C(1), rounduint64(0.50000001));
    BOOST_CHECK_EQUAL(UINT64_C(2), rounduint64(1.50000001));
    BOOST_CHECK_EQUAL(UINT64_C(3), rounduint64(2.50000001));
    BOOST_CHECK_EQUAL(UINT64_C(4), rounduint64(3.50000001));
}

BOOST_AUTO_TEST_CASE(mastercore_rounduint64_limits)
{    
    BOOST_CHECK_EQUAL( // 1 byte signed
            UINT64_C(127),
            rounduint64(127.0));    
    BOOST_CHECK_EQUAL( // 1 byte unsigned
            UINT64_C(255),
            rounduint64(255.0));    
    BOOST_CHECK_EQUAL( // 2 byte signed
            UINT64_C(32767),
            rounduint64(32767.0));    
    BOOST_CHECK_EQUAL( // 2 byte unsigned
            UINT64_C(65535),
            rounduint64(65535.0));    
    BOOST_CHECK_EQUAL( // 4 byte signed
            UINT64_C(2147483647),
            rounduint64(2147483647.0));    
    BOOST_CHECK_EQUAL( // 4 byte unsigned
            UINT64_C(4294967295),
            rounduint64(4294967295.0L));    
    BOOST_CHECK_EQUAL( // 8 byte signed
            UINT64_C(9223372036854775807),
            rounduint64(9223372036854775807.0L));    
    BOOST_CHECK_EQUAL( // 8 byte unsigned
            UINT64_C(18446744073709551615),
            rounduint64(18446744073709551615.0L));
}

BOOST_AUTO_TEST_CASE(mastercore_rounduint64_types)
{
    BOOST_CHECK_EQUAL(
            rounduint64(static_cast<float>(1.23456789)),
            rounduint64(static_cast<float>(1.23456789)));
    BOOST_CHECK_EQUAL(
            rounduint64(static_cast<float>(1.23456789)),
            rounduint64(static_cast<double>(1.23456789)));
    BOOST_CHECK_EQUAL(
            rounduint64(static_cast<float>(1.23456789)),
            rounduint64(static_cast<long double>(1.23456789)));
    BOOST_CHECK_EQUAL(
            rounduint64(static_cast<double>(1.23456789)),
            rounduint64(static_cast<double>(1.23456789)));
    BOOST_CHECK_EQUAL(
            rounduint64(static_cast<double>(1.23456789)),
            rounduint64(static_cast<long double>(1.23456789)));
    BOOST_CHECK_EQUAL(
            rounduint64(static_cast<long double>(1.23456789)),
            rounduint64(static_cast<long double>(1.23456789)));
}

BOOST_AUTO_TEST_CASE(mastercore_rounduint64_promotion)
{
    BOOST_CHECK_EQUAL(UINT64_C(42), rounduint64(static_cast<int8_t>(42)));
    BOOST_CHECK_EQUAL(UINT64_C(42), rounduint64(static_cast<uint8_t>(42)));
    BOOST_CHECK_EQUAL(UINT64_C(42), rounduint64(static_cast<int16_t>(42)));    
    BOOST_CHECK_EQUAL(UINT64_C(42), rounduint64(static_cast<uint16_t>(42)));
    BOOST_CHECK_EQUAL(UINT64_C(42), rounduint64(static_cast<int32_t>(42)));
    BOOST_CHECK_EQUAL(UINT64_C(42), rounduint64(static_cast<uint32_t>(42)));
    BOOST_CHECK_EQUAL(UINT64_C(42), rounduint64(static_cast<int64_t>(42)));    
    BOOST_CHECK_EQUAL(UINT64_C(42), rounduint64(static_cast<uint64_t>(42)));
    BOOST_CHECK_EQUAL(UINT64_C(42), rounduint64(static_cast<float>(42)));
    BOOST_CHECK_EQUAL(UINT64_C(42), rounduint64(static_cast<double>(42)));
    BOOST_CHECK_EQUAL(UINT64_C(42), rounduint64(static_cast<long double>(42)));
}

BOOST_AUTO_TEST_CASE(mastercore_rounduint64_absolute)
{
    BOOST_CHECK_EQUAL(
            rounduint64(-128.0f),
            rounduint64(128.0f));
    BOOST_CHECK_EQUAL(
            rounduint64(-32768.0),
            rounduint64(32768.0));
    BOOST_CHECK_EQUAL(
            rounduint64(-65536.0),
            rounduint64(65536.0));
    BOOST_CHECK_EQUAL(
            rounduint64(-2147483648.0L),
            rounduint64(2147483648.0L));
    BOOST_CHECK_EQUAL(            
            rounduint64(-4294967296.0L),
            rounduint64(4294967296.0L));
    BOOST_CHECK_EQUAL(
            rounduint64(-9223372036854775807.0L),
            rounduint64(9223372036854775807.0L));
}

BOOST_AUTO_TEST_CASE(mastercore_rounduint64_special_cases)
{    
    BOOST_CHECK_EQUAL(UINT64_C(0), rounduint64(static_cast<double>(0.49999999999999994)));
    BOOST_CHECK_EQUAL(UINT64_C(2147483648), rounduint64(static_cast<int32_t>(-2147483648)));
}

BOOST_AUTO_TEST_SUITE_END()
