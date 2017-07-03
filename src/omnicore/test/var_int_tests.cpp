#include "omnicore/varint.h"

#include "test/test_bitcoin.h"
#include "utilstrencodings.h"

#include <boost/test/unit_test.hpp>

#include <stdint.h>
#include <vector>
#include <string>

// Returns true if a byte has the MSB set
//bool IsMSBSet(unsigned char* byte);

BOOST_FIXTURE_TEST_SUITE(omnicore_var_int_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(var_int_msb)
{
    unsigned char a = 0x00;
    unsigned char b = 0x7f;
    unsigned char c = 0x80;
    unsigned char d = 0xff;
    BOOST_CHECK(!IsMSBSet(&a));
    BOOST_CHECK(!IsMSBSet(&b));
    BOOST_CHECK(IsMSBSet(&c));
    BOOST_CHECK(IsMSBSet(&d));
}

BOOST_AUTO_TEST_CASE(var_int_compress)
{
    // 1 byte
    BOOST_CHECK_EQUAL(HexStr(CompressInteger(0)), "00");
    BOOST_CHECK_EQUAL(HexStr(CompressInteger(1)), "01");
    BOOST_CHECK_EQUAL(HexStr(CompressInteger(127)), "7f");
    // 2 bytes
    BOOST_CHECK_EQUAL(HexStr(CompressInteger(128)), "8001");
    BOOST_CHECK_EQUAL(HexStr(CompressInteger(16383)), "ff7f");
    // 3 bytes
    BOOST_CHECK_EQUAL(HexStr(CompressInteger(16384)), "808001");
    BOOST_CHECK_EQUAL(HexStr(CompressInteger(2097151)), "ffff7f");
    // 4 bytes
    BOOST_CHECK_EQUAL(HexStr(CompressInteger(2097152)), "80808001");
    BOOST_CHECK_EQUAL(HexStr(CompressInteger(268435455)), "ffffff7f");
    // 5 bytes
    BOOST_CHECK_EQUAL(HexStr(CompressInteger(268435456)), "8080808001");
    BOOST_CHECK_EQUAL(HexStr(CompressInteger(34359738367)), "ffffffff7f");
    // 6 bytes
    BOOST_CHECK_EQUAL(HexStr(CompressInteger(34359738368)), "808080808001");
    BOOST_CHECK_EQUAL(HexStr(CompressInteger(4398046511103)), "ffffffffff7f");
    // 7 bytes
    BOOST_CHECK_EQUAL(HexStr(CompressInteger(4398046511104)), "80808080808001");
    BOOST_CHECK_EQUAL(HexStr(CompressInteger(562949953421311)), "ffffffffffff7f");
    // 8 bytes
    BOOST_CHECK_EQUAL(HexStr(CompressInteger(562949953421312)), "8080808080808001");
    BOOST_CHECK_EQUAL(HexStr(CompressInteger(72057594037927935)), "ffffffffffffff7f");
    // 9 bytes
    BOOST_CHECK_EQUAL(HexStr(CompressInteger(72057594037927936)), "808080808080808001");
    BOOST_CHECK_EQUAL(HexStr(CompressInteger(9223372036854775807)), "ffffffffffffffff7f");
}

BOOST_AUTO_TEST_CASE(var_int_decompress)
{
    // 1 byte
    BOOST_CHECK_EQUAL(DecompressInteger(ParseHex("00")), 0);
    BOOST_CHECK_EQUAL(DecompressInteger(ParseHex("01")), 1);
    BOOST_CHECK_EQUAL(DecompressInteger(ParseHex("7f")), 127);
    // 2 bytes
    BOOST_CHECK_EQUAL(DecompressInteger(ParseHex("8001")), 128);
    BOOST_CHECK_EQUAL(DecompressInteger(ParseHex("ff7f")), 16383);
    // 3 bytes
    BOOST_CHECK_EQUAL(DecompressInteger(ParseHex("808001")), 16384);
    BOOST_CHECK_EQUAL(DecompressInteger(ParseHex("ffff7f")), 2097151);
    // 4 bytes
    BOOST_CHECK_EQUAL(DecompressInteger(ParseHex("80808001")), 2097152);
    BOOST_CHECK_EQUAL(DecompressInteger(ParseHex("ffffff7f")), 268435455);
    // 5 bytes
    BOOST_CHECK_EQUAL(DecompressInteger(ParseHex("8080808001")), 268435456);
    BOOST_CHECK_EQUAL(DecompressInteger(ParseHex("ffffffff7f")), 34359738367);
    // 6 bytes
    BOOST_CHECK_EQUAL(DecompressInteger(ParseHex("808080808001")), 34359738368);
    BOOST_CHECK_EQUAL(DecompressInteger(ParseHex("ffffffffff7f")), 4398046511103);
    // 7 bytes
    BOOST_CHECK_EQUAL(DecompressInteger(ParseHex("80808080808001")), 4398046511104);
    BOOST_CHECK_EQUAL(DecompressInteger(ParseHex("ffffffffffff7f")), 562949953421311);
    // 8 bytes
    BOOST_CHECK_EQUAL(DecompressInteger(ParseHex("8080808080808001")), 562949953421312);
    BOOST_CHECK_EQUAL(DecompressInteger(ParseHex("ffffffffffffff7f")), 72057594037927935);
    // 9 bytes
    BOOST_CHECK_EQUAL(DecompressInteger(ParseHex("808080808080808001")), 72057594037927936);
    BOOST_CHECK_EQUAL(DecompressInteger(ParseHex("ffffffffffffffff7f")), 9223372036854775807);
}

BOOST_AUTO_TEST_SUITE_END()
