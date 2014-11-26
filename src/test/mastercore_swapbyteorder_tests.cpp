#include "mastercore_convert.h"

#include <endian.h>

#include <boost/test/unit_test.hpp>

using namespace mastercore;

BOOST_AUTO_TEST_SUITE(mastercore_swapbyteorder_tests)

BOOST_AUTO_TEST_CASE(mastercore_swapbyteorder_should_be_equal)
{
    uint16_t a = 258; 
    uint32_t b = 16909060; 
    uint64_t c = 722385979038285;

    swapByteOrder16(a);
    swapByteOrder32(b);
    swapByteOrder64(c);

    BOOST_CHECK_EQUAL(a, htobe16(258));
    BOOST_CHECK_EQUAL(b, htobe32(16909060));
    BOOST_CHECK_EQUAL(c, htobe64(722385979038285));
}

BOOST_AUTO_TEST_CASE(mastercore_swapbyteorder_should_not_cycle)
{
    uint16_t a1 = 41959;
    uint16_t a2 = 41959;
    swapByteOrder16(a1);
    swapByteOrder16(a2);
    swapByteOrder16(a2);
    BOOST_CHECK_EQUAL(a1, a2); // Should (?) be equal, but is not

    uint32_t b1 = 16909060;
    uint32_t b2 = 16909060;
    swapByteOrder32(b1);
    swapByteOrder32(b2);
    swapByteOrder32(b2);    
    BOOST_CHECK_EQUAL(b1, b2); // Should (?) be equal, but is not

    uint64_t c1 = 722385979038285;
    uint64_t c2 = 722385979038285;
    swapByteOrder64(c1);
    swapByteOrder64(c2);
    swapByteOrder64(c2);
    BOOST_CHECK_EQUAL(c1, c2); // Should (?) be equal, but is not

    // To compare:
    uint16_t d1 = 41959;
    uint16_t d2 = 41959;
    d1 = htole16(d1);
    d2 = htole16(d2);
    d2 = htole16(d2);
    BOOST_CHECK_EQUAL(d1, d2);

    uint32_t e1 = 16909060;
    uint32_t e2 = 16909060;
    e1 = htole32(e1);
    e2 = htole32(e2);
    e2 = htole32(e2);
    BOOST_CHECK_EQUAL(e1, e2);

    uint64_t f1 = 722385979038285;
    uint64_t f2 = 722385979038285;
    f1 = htole64(f1);
    f2 = htole64(f2);
    f2 = htole64(f2);  
    BOOST_CHECK_EQUAL(f1, f2);
}

BOOST_AUTO_TEST_CASE(mastercore_swapbyteorder_should_differ)
{
    uint16_t a = 4660; 
    uint32_t b = 305419896; 
    uint64_t c = 1311768467463790320;

    swapByteOrder16(a);
    swapByteOrder32(b);
    swapByteOrder64(c);

    // swapByteOrder should convert to big endian, htole explicitly converts to little endian
    BOOST_CHECK(a != htole16(4660));
    BOOST_CHECK(b != htole32(305419896));
    BOOST_CHECK(c != htole64(1311768467463790320));
}

BOOST_AUTO_TEST_SUITE_END()
