#include "test.h"
#include <vector>

static uint8_t noise = 0;

static void memnoise(void* dst, size_t n)
{
    for (size_t i = 0; i < n; ++i)
        ((uint8_t*)dst)[i] = noise++;
}

TEST_CASE("Address Init", "[address]")
{
    clarinet_addr src = {0};
    REQUIRE(src.family == 0);
    REQUIRE(src.as.ipv4.u.dword[0] == 0);
    REQUIRE(src.as.ipv6.flowinfo == 0);
    REQUIRE(src.as.ipv6.u.dword[0] == 0);
    REQUIRE(src.as.ipv6.u.dword[1] == 0);
    REQUIRE(src.as.ipv6.u.dword[2] == 0);
    REQUIRE(src.as.ipv6.u.dword[3] == 0);
    REQUIRE(src.as.ipv6.scope_id == 0);
}

TEST_CASE("Address Size", "[address]")
{
    SECTION("Size")
    {
        REQUIRE(sizeof(clarinet_addr) == 28);
        REQUIRE(sizeof(((clarinet_addr*)0)->family) == sizeof(uint16_t));
        REQUIRE(sizeof(((clarinet_addr*)0)->as.ipv4.u.byte[0]) == sizeof(uint8_t));
        REQUIRE(sizeof(((clarinet_addr*)0)->as.ipv4.u.word[0]) == sizeof(uint16_t));
        REQUIRE(sizeof(((clarinet_addr*)0)->as.ipv4.u.dword[0]) == sizeof(uint32_t));
        REQUIRE(sizeof(((clarinet_addr*)0)->as.ipv6.u.byte[0]) == sizeof(uint8_t));
        REQUIRE(sizeof(((clarinet_addr*)0)->as.ipv6.u.word[0]) == sizeof(uint16_t));
        REQUIRE(sizeof(((clarinet_addr*)0)->as.ipv6.u.dword[0]) == sizeof(uint32_t));
        REQUIRE(sizeof(((clarinet_addr*)0)->as.ipv6.flowinfo) == sizeof(uint32_t));
        REQUIRE(sizeof(((clarinet_addr*)0)->as.ipv6.scope_id) == sizeof(uint32_t));
        REQUIRE(sizeof(struct clarinet_addr_ipv4) == 20);
        REQUIRE(sizeof(struct clarinet_addr_ipv6) == 24);
        REQUIRE(sizeof(((clarinet_addr*)0)->as.ipv4) == sizeof(struct clarinet_addr_ipv4));
        REQUIRE(sizeof(((clarinet_addr*)0)->as.ipv6) == sizeof(struct clarinet_addr_ipv6));
    }
}

TEST_CASE("Address Max Strlen", "[address]")
{
    REQUIRE(CLARINET_ADDR_STRLEN == 57); /* "0000:0000:0000:0000:0000:ffff:255.255.255.255%4294967295" */
}

TEST_CASE("IPv4 Address Any", "[address][ipv4][any]")
{
    clarinet_addr actual = clarinet_addr_ipv4_any;
    REQUIRE(actual.family == CLARINET_AF_INET);
    REQUIRE(actual.as.ipv4.u.dword[0] == 0);
    REQUIRE((actual.as.ipv4.u.word[0]
           | actual.as.ipv4.u.word[1]) == 0);
    REQUIRE((actual.as.ipv4.u.byte[0]
           | actual.as.ipv4.u.byte[1]
           | actual.as.ipv4.u.byte[2]
           | actual.as.ipv4.u.byte[3]) == 0);


    SECTION("Check constant CLARINET_ADDR_IPV4_ANY")
    {
        clarinet_addr expected = clarinet_addr_none;
        expected.family = CLARINET_AF_INET;
        REQUIRE_THAT(memory(&expected, sizeof(expected)), Equals(memory(&actual, sizeof(actual))));
    }

    SECTION("0.0.0.0 is an ipv4 any")
    {
        REQUIRE(clarinet_addr_is_ipv4(&actual));
        REQUIRE(clarinet_addr_is_ipv4_any(&actual));
        REQUIRE(clarinet_addr_is_any(&actual));
    }
}

TEST_CASE("IPv4 Address Loopback", "[address][ipv4][loopback]")
{
    clarinet_addr actual = clarinet_addr_ipv4_loopback;
    REQUIRE(actual.family == CLARINET_AF_INET);
    REQUIRE(actual.as.ipv4.u.byte[0] == 127);
    REQUIRE(actual.as.ipv4.u.byte[1] == 0);
    REQUIRE(actual.as.ipv4.u.byte[2] == 0);
    REQUIRE(actual.as.ipv4.u.byte[3] == 1);

    SECTION("Check constant CLARINET_ADDR_IPV4_LOOPBACK")
    {
        clarinet_addr expected = clarinet_addr_none;
        expected.family = CLARINET_AF_INET;
        expected.as.ipv4.u.byte[0] = 127;
        expected.as.ipv4.u.byte[1] = 0;
        expected.as.ipv4.u.byte[2] = 0;
        expected.as.ipv4.u.byte[3] = 1;

        REQUIRE_THAT(memory(&expected, sizeof(expected)), Equals(memory(&actual, sizeof(actual))));
    }

    SECTION("127.0.0.1 is an ipv4 looback")
    {
        REQUIRE(clarinet_addr_is_ipv4(&actual));
        REQUIRE(clarinet_addr_is_ipv4_loopback(&actual));
        REQUIRE(clarinet_addr_is_loopback(&actual));
    }

    SECTION("127.0.0.0 is NOT an ipv4 loopback")
    {
        actual.as.ipv4.u.byte[3] = 0;
        REQUIRE_FALSE(clarinet_addr_is_ipv4_loopback(&actual));
        REQUIRE_FALSE(clarinet_addr_is_loopback(&actual));
    }

    SECTION("127.0.0.255 is NOT an ipv4 loopback")
    {
        actual.as.ipv4.u.byte[3] = 255;
        REQUIRE_FALSE(clarinet_addr_is_ipv4_loopback(&actual));
        REQUIRE_FALSE(clarinet_addr_is_loopback(&actual));
    }

    /* Technically we should assert the whole range 127.0.0.1 to 127.255.255.254 but sampling must suffice. */

    SECTION("127.0.0.2 is an ipv4 loopback")
    {
        actual.as.ipv4.u.byte[3] = 2;
        REQUIRE(clarinet_addr_is_ipv4_loopback(&actual));
        REQUIRE(clarinet_addr_is_loopback(&actual));
    }

    SECTION("127.0.0.254 is an ipv4 loopback")
    {
        actual.as.ipv4.u.byte[3] = 254;
        REQUIRE(clarinet_addr_is_ipv4_loopback(&actual));
        REQUIRE(clarinet_addr_is_loopback(&actual));
    }

    SECTION("127.255.255.254 is an ipv4 loopback")
    {
        actual.as.ipv4.u.byte[1] = 255;
        actual.as.ipv4.u.byte[2] = 255;
        actual.as.ipv4.u.byte[3] = 254;
        REQUIRE(clarinet_addr_is_ipv4_loopback(&actual));
        REQUIRE(clarinet_addr_is_loopback(&actual));
    }
}

TEST_CASE("IPv4 Address Broadcast", "[address][ipv4][broadcast]")
{
    clarinet_addr actual = clarinet_addr_ipv4_broadcast;
    REQUIRE(actual.family == CLARINET_AF_INET);
    REQUIRE(actual.as.ipv4.u.byte[0] == 255);
    REQUIRE(actual.as.ipv4.u.byte[1] == 255);
    REQUIRE(actual.as.ipv4.u.byte[2] == 255);
    REQUIRE(actual.as.ipv4.u.byte[3] == 255);

    SECTION("Check constant clarinet_addr_ipv4_broadcast")
    {
        clarinet_addr expected = clarinet_addr_none;
        expected.family = CLARINET_AF_INET;
        expected.as.ipv4.u.byte[0] = 255;
        expected.as.ipv4.u.byte[1] = 255;
        expected.as.ipv4.u.byte[2] = 255;
        expected.as.ipv4.u.byte[3] = 255;

        REQUIRE_THAT(memory(&expected, sizeof(expected)), Equals(memory(&actual, sizeof(actual))));
    }

    SECTION("255.255.255.255 is an ipv4 broadcast")
    {
        REQUIRE(clarinet_addr_is_ipv4(&actual));
        REQUIRE(clarinet_addr_is_ipv4_broadcast(&actual));
        REQUIRE(clarinet_addr_is_broadcast(&actual));
    }

    SECTION("255.255.255.0 is NOT an ipv4 broadcast")
    {
        actual.as.ipv4.u.byte[3] = 0;
        REQUIRE_FALSE(clarinet_addr_is_ipv4_broadcast(&actual));
    }

    SECTION("255.255.255.127 is NOT an ipv4 broadcast")
    {
        actual.as.ipv4.u.byte[3] = 127;
        REQUIRE_FALSE(clarinet_addr_is_ipv4_broadcast(&actual));
    }

    SECTION("255.255.255.2 is NOT an ipv4 broadcast")
    {
        actual.as.ipv4.u.byte[3] = 2;
        REQUIRE_FALSE(clarinet_addr_is_ipv4_broadcast(&actual));
    }

    SECTION("255.255.255.127 is NOT an ipv4 broadcast")
    {
        actual.as.ipv4.u.byte[3] = 127;
        REQUIRE_FALSE(clarinet_addr_is_ipv4_broadcast(&actual));
    }

    SECTION("::FFFF:255.255.255.255 is NOT an ipv4 broadcast")
    {
        actual.family = CLARINET_AF_INET6;
        actual.as.ipv6.u.byte[10] = 255;
        actual.as.ipv6.u.byte[11] = 255;
        actual.as.ipv6.u.byte[12] = 255;
        actual.as.ipv6.u.byte[13] = 255;
        actual.as.ipv6.u.byte[14] = 255;
        actual.as.ipv6.u.byte[15] = 255;
        REQUIRE_FALSE(clarinet_addr_is_ipv4_broadcast(&actual));
    }
}

TEST_CASE("IPv4 Mapped Loopback", "[address][ipv6][ipv4][loopback]")
{
    clarinet_addr actual = clarinet_addr_ipv4mapped_loopback;
    REQUIRE(actual.family == CLARINET_AF_INET6);
    REQUIRE(actual.as.ipv6.flowinfo == 0);
    REQUIRE(actual.as.ipv6.scope_id == 0);
    REQUIRE((actual.as.ipv6.u.dword[0]
           | actual.as.ipv6.u.dword[1]) == 0);
    REQUIRE((actual.as.ipv6.u.word[0]
           | actual.as.ipv6.u.word[1]
           | actual.as.ipv6.u.word[2]
           | actual.as.ipv6.u.word[3]
           | actual.as.ipv6.u.word[4]) == 0);
    REQUIRE((actual.as.ipv6.u.byte[0]
           | actual.as.ipv6.u.byte[1]
           | actual.as.ipv6.u.byte[2]
           | actual.as.ipv6.u.byte[3]
           | actual.as.ipv6.u.byte[4]
           | actual.as.ipv6.u.byte[5]
           | actual.as.ipv6.u.byte[6]
           | actual.as.ipv6.u.byte[7]
           | actual.as.ipv6.u.byte[8]
           | actual.as.ipv6.u.byte[9]) == 0);

    REQUIRE(actual.as.ipv6.u.byte[10] == 255);
    REQUIRE(actual.as.ipv6.u.byte[11] == 255);
    REQUIRE(actual.as.ipv6.u.byte[12] == 127);
    REQUIRE(actual.as.ipv6.u.byte[13] == 0);
    REQUIRE(actual.as.ipv6.u.byte[14] == 0);
    REQUIRE(actual.as.ipv6.u.byte[15] == 1);

    SECTION("Check constant clarinet_addr_ipv4mapped_loopback")
    {
        clarinet_addr expected = clarinet_addr_none;
        expected.family = CLARINET_AF_INET6;
        expected.as.ipv6.u.byte[10] = 255;
        expected.as.ipv6.u.byte[11] = 255;
        expected.as.ipv6.u.byte[12] = 127;
        expected.as.ipv6.u.byte[13] = 0;
        expected.as.ipv6.u.byte[14] = 0;
        expected.as.ipv6.u.byte[15] = 1;

        REQUIRE_THAT(memory(&expected, sizeof(expected)), Equals(memory(&actual, sizeof(actual))));
    }

    SECTION("::FFFF:127.0.0.1 is an ipv4 mapped loopback")
    {
        REQUIRE(clarinet_addr_is_ipv4mapped(&actual));
        REQUIRE(clarinet_addr_is_ipv4mapped_loopback(&actual));
        REQUIRE(clarinet_addr_is_loopback(&actual));
    }

    SECTION("::FFFF:127.0.0.0 is NOT an ipv4 mapped loopback")
    {
        actual.as.ipv6.u.byte[15] = 0;
        REQUIRE_FALSE(clarinet_addr_is_ipv4mapped_loopback(&actual));
    }

    SECTION("::FFFF:127.0.0.255 is NOT an ipv4 mapped loopback")
    {
        actual.as.ipv6.u.byte[15] = 255;
        REQUIRE_FALSE(clarinet_addr_is_ipv4mapped_loopback(&actual));
    }

    SECTION("::FFFF:127.0.0.2 is an ipv4 mapped loopback")
    {
        actual.as.ipv6.u.byte[15] = 2;
        REQUIRE(clarinet_addr_is_ipv4mapped_loopback(&actual));
    }

    SECTION("::FFFF:127.255.255.254 is an ipv4 mapped loopback")
    {
        actual.as.ipv6.u.byte[13] = 255;
        actual.as.ipv6.u.byte[14] = 255;
        actual.as.ipv6.u.byte[14] = 254;
        REQUIRE(clarinet_addr_is_ipv4mapped_loopback(&actual));
    }
}

TEST_CASE("IPv6 Address Any", "[address][ipv6][any]")
{
    clarinet_addr actual = clarinet_addr_ipv6_any;
    REQUIRE(actual.family == CLARINET_AF_INET6);
    REQUIRE(actual.as.ipv6.flowinfo == 0);
    REQUIRE(actual.as.ipv6.scope_id == 0);
    REQUIRE((actual.as.ipv6.u.dword[0]
           | actual.as.ipv6.u.dword[1]
           | actual.as.ipv6.u.dword[2]
           | actual.as.ipv6.u.dword[3]) == 0);
    REQUIRE((actual.as.ipv6.u.word[0]
           | actual.as.ipv6.u.word[1]
           | actual.as.ipv6.u.word[2]
           | actual.as.ipv6.u.word[3]
           | actual.as.ipv6.u.word[4]
           | actual.as.ipv6.u.word[5]
           | actual.as.ipv6.u.word[6]
           | actual.as.ipv6.u.word[7]) == 0);
    REQUIRE((actual.as.ipv6.u.byte[0]
           | actual.as.ipv6.u.byte[1]
           | actual.as.ipv6.u.byte[2]
           | actual.as.ipv6.u.byte[3]
           | actual.as.ipv6.u.byte[4]
           | actual.as.ipv6.u.byte[5]
           | actual.as.ipv6.u.byte[6]
           | actual.as.ipv6.u.byte[7]
           | actual.as.ipv6.u.byte[8]
           | actual.as.ipv6.u.byte[9]
           | actual.as.ipv6.u.byte[10]
           | actual.as.ipv6.u.byte[11]
           | actual.as.ipv6.u.byte[12]
           | actual.as.ipv6.u.byte[13]
           | actual.as.ipv6.u.byte[14]
           | actual.as.ipv6.u.byte[15]) == 0);

    clarinet_addr expected = clarinet_addr_none;
    expected.family = CLARINET_AF_INET6;

    REQUIRE_THAT(memory(&expected, sizeof(expected)), Equals(memory(&actual, sizeof(actual))));

    REQUIRE(clarinet_addr_is_ipv6(&actual));
    REQUIRE(clarinet_addr_is_ipv6_any(&actual));
    REQUIRE(clarinet_addr_is_any(&actual));
}

TEST_CASE("IPv6 Address Loopback", "[address][ipv6][loopback]")
{
    clarinet_addr actual = clarinet_addr_ipv6_loopback;
    REQUIRE(actual.family == CLARINET_AF_INET6);
    REQUIRE(actual.as.ipv6.flowinfo == 0);
    REQUIRE(actual.as.ipv6.scope_id == 0);
    REQUIRE((actual.as.ipv6.u.dword[0]
           | actual.as.ipv6.u.dword[1]
           | actual.as.ipv6.u.dword[2]) == 0);
    REQUIRE((actual.as.ipv6.u.word[0]
           | actual.as.ipv6.u.word[1]
           | actual.as.ipv6.u.word[2]
           | actual.as.ipv6.u.word[3]
           | actual.as.ipv6.u.word[4]
           | actual.as.ipv6.u.word[5]
           | actual.as.ipv6.u.word[6]) == 0);
    REQUIRE((actual.as.ipv6.u.byte[0]
           | actual.as.ipv6.u.byte[1]
           | actual.as.ipv6.u.byte[2]
           | actual.as.ipv6.u.byte[3]
           | actual.as.ipv6.u.byte[4]
           | actual.as.ipv6.u.byte[5]
           | actual.as.ipv6.u.byte[6]
           | actual.as.ipv6.u.byte[7]
           | actual.as.ipv6.u.byte[8]
           | actual.as.ipv6.u.byte[9]
           | actual.as.ipv6.u.byte[10]
           | actual.as.ipv6.u.byte[11]
           | actual.as.ipv6.u.byte[12]
           | actual.as.ipv6.u.byte[13]
           | actual.as.ipv6.u.byte[14]) == 0);
    REQUIRE(actual.as.ipv6.u.byte[15] == 1);

    SECTION("Check clarinet_addr_ipv6_loopback constant")
    {
        clarinet_addr expected = clarinet_addr_none;
        expected.family = CLARINET_AF_INET6;
        expected.as.ipv6.u.byte[15] = 1;

        REQUIRE_THAT(memory(&expected, sizeof(expected)), Equals(memory(&actual, sizeof(clarinet_addr))));
    }

    SECTION("::1 is an ipv6 looback")
    {
        REQUIRE(clarinet_addr_is_ipv6(&actual));
        REQUIRE(clarinet_addr_is_ipv6_loopback(&actual));
        REQUIRE(clarinet_addr_is_loopback(&actual));
    }

    SECTION(":: is NOT an ipv6 looback")
    {
        actual.as.ipv6.u.byte[15] = 0;
        REQUIRE_FALSE(clarinet_addr_is_ipv6_loopback(&actual));
    }

    SECTION("::00ff is NOT an ipv6 looback")
    {
        actual.as.ipv6.u.byte[15] = 255;
        REQUIRE_FALSE(clarinet_addr_is_ipv6_loopback(&actual));
    }

    SECTION("::2 is NOT an ipv6 looback")
    {
        actual.as.ipv6.u.byte[15] = 2;
        REQUIRE_FALSE(clarinet_addr_is_ipv6_loopback(&actual));
    }

    SECTION("::00fe is NOT an ipv6 looback")
    {
        actual.as.ipv6.u.byte[15] = 254;
        REQUIRE_FALSE(clarinet_addr_is_ipv6_loopback(&actual));
    }

    SECTION("::ffff:127.0.0.1 is NOT an ipv6 looback")
    {
        actual.as.ipv6.u.byte[10] = 255;
        actual.as.ipv6.u.byte[11] = 255;
        actual.as.ipv6.u.byte[12] = 127;
        actual.as.ipv6.u.byte[13] = 0;
        actual.as.ipv6.u.byte[14] = 0;
        actual.as.ipv6.u.byte[15] = 1;
        REQUIRE_FALSE(clarinet_addr_is_ipv6_loopback(&actual));
    }
}

TEST_CASE("Address Is Equal", "[address]")
{
    SECTION("Address None (Equal)")
    {
        /* using two distinct allocations for the same contant on purpose */
        int index;
        clarinet_addr a, b;
        std::tie(index, a, b) = GENERATE(table<int, clarinet_addr, clarinet_addr>({
                { 0, clarinet_addr_none, clarinet_addr_none }
            }));

        FROM(index);

        REQUIRE(clarinet_addr_is_equal(&a, &b));
    }

    SECTION("Address None (Not Equal)")
    {
        /* using two distinct allocations for the same contant on purpose */
        int index;
        clarinet_addr a, b;
        std::tie(index, a, b) = GENERATE(table<int, clarinet_addr, clarinet_addr>({
                { 0, clarinet_addr_none, clarinet_addr_ipv4_any },
                { 1, clarinet_addr_none, clarinet_addr_ipv4_loopback },
                { 2, clarinet_addr_none, clarinet_addr_ipv4_broadcast }
            }));

        FROM(index);

        REQUIRE_FALSE(clarinet_addr_is_equal(&a, &b));
    }
}

TEST_CASE("IPv4 Address Is Equal", "[address][ipv4]")
{
    SECTION("Constant ipv4 addresses (Equal)")
    {
        /* using two distinct allocations for the same contant on purpose */
        int index;
        clarinet_addr a, b;
        std::tie(index, a, b) = GENERATE(table<int, clarinet_addr, clarinet_addr>({
                { 0, clarinet_addr_ipv4_any, clarinet_addr_ipv4_any },
                { 1, clarinet_addr_ipv4_loopback, clarinet_addr_ipv4_loopback },
                { 2, clarinet_addr_ipv4_broadcast, clarinet_addr_ipv4_broadcast },
            }));

        FROM(index);

        REQUIRE(clarinet_addr_is_equal(&a, &b));
    }

    SECTION("Constant ipv4 addresses (Not Equal)")
    {
        int index;
        clarinet_addr a, b;
        std::tie(index, a, b) = GENERATE(table<int, clarinet_addr, clarinet_addr>({
                { 0, clarinet_addr_ipv4_any, clarinet_addr_ipv4_loopback },
                { 1, clarinet_addr_ipv4_any, clarinet_addr_ipv4_broadcast },
                { 2, clarinet_addr_ipv4_loopback, clarinet_addr_ipv4_broadcast }
            }));

        FROM(index);

        REQUIRE_FALSE(clarinet_addr_is_equal(&a, &b));
    }

    SECTION("Custom ipv4 addresses (Equal)")
    {
        clarinet_addr a;
        memnoise(&a, sizeof(a));
        a.family = CLARINET_AF_INET;
        a.as.ipv4.u.byte[0] = 0x00;
        a.as.ipv4.u.byte[1] = 0x11;
        a.as.ipv4.u.byte[2] = 0x22;
        a.as.ipv4.u.byte[3] = 0x33;

        clarinet_addr b;
        memnoise(&b, sizeof(b));
        b.family = CLARINET_AF_INET;
        b.as.ipv4.u.byte[0] = 0x00;
        b.as.ipv4.u.byte[1] = 0x11;
        b.as.ipv4.u.byte[2] = 0x22;
        b.as.ipv4.u.byte[3] = 0x33;

        REQUIRE(clarinet_addr_is_equal(&a, &b));
    }

    SECTION("Same ipv4 address but one has family NONE")
    {
        clarinet_addr a;
        memnoise(&a, sizeof(a));
        a.family = CLARINET_AF_NONE;
        a.as.ipv4.u.byte[0] = 0x00;
        a.as.ipv4.u.byte[1] = 0x11;
        a.as.ipv4.u.byte[2] = 0x22;
        a.as.ipv4.u.byte[3] = 0x33;

        clarinet_addr b;
        memnoise(&b, sizeof(b));
        b.family = CLARINET_AF_INET;
        b.as.ipv4.u.byte[0] = 0x00;
        b.as.ipv4.u.byte[1] = 0x11;
        b.as.ipv4.u.byte[2] = 0x22;
        b.as.ipv4.u.byte[3] = 0x33;

        REQUIRE_FALSE(clarinet_addr_is_equal(&a, &b));
    }
}

TEST_CASE("IPv6 Address Is Equal", "[address][ipv6]")
{
    SECTION("Constant ipv6 addresses (Equal)")
    {
        /* using two distinct allocations for the same contant on purpose */
        int index;
        clarinet_addr a, b;
        std::tie(index, a, b) = GENERATE(table<int, clarinet_addr, clarinet_addr>({
                { 0, clarinet_addr_ipv6_any, clarinet_addr_ipv6_any },
                { 1, clarinet_addr_ipv6_loopback, clarinet_addr_ipv6_loopback },
                { 2, clarinet_addr_ipv4mapped_loopback, clarinet_addr_ipv4mapped_loopback}
            }));

        FROM(index);

        REQUIRE(clarinet_addr_is_equal(&a, &b));
    }

    SECTION("Constant ipv6 addresses (Not Equal)")
    {
        int index;
        clarinet_addr a, b;
        std::tie(index, a, b) = GENERATE(table<int, clarinet_addr, clarinet_addr>({
                { 0, clarinet_addr_ipv6_any, clarinet_addr_ipv4_any },
                { 1, clarinet_addr_ipv6_loopback, clarinet_addr_ipv4_loopback },
                { 2, clarinet_addr_ipv6_any, clarinet_addr_ipv6_loopback },
                { 3, clarinet_addr_ipv6_any, clarinet_addr_ipv4mapped_loopback },
                { 4, clarinet_addr_ipv6_loopback, clarinet_addr_ipv4mapped_loopback }
            }));

        FROM(index);

        REQUIRE_FALSE(clarinet_addr_is_equal(&a, &b));
    }
    
    SECTION("Custom ipv6 addresses (Equal)")
    {
        clarinet_addr a;
        memnoise(&a, sizeof(a));
        a.family = CLARINET_AF_INET6;
        a.as.ipv6.flowinfo = 0xAABBCCDD;
        a.as.ipv6.u.byte[0] = 0x00;
        a.as.ipv6.u.byte[1] = 0x11;
        a.as.ipv6.u.byte[2] = 0x22;
        a.as.ipv6.u.byte[3] = 0x33;
        a.as.ipv6.u.byte[4] = 0x44;
        a.as.ipv6.u.byte[5] = 0x55;
        a.as.ipv6.u.byte[6] = 0x66;
        a.as.ipv6.u.byte[7] = 0x77;
        a.as.ipv6.u.byte[8] = 0x88;
        a.as.ipv6.u.byte[9] = 0x99;
        a.as.ipv6.u.byte[10] = 0xAA;
        a.as.ipv6.u.byte[11] = 0xBB;
        a.as.ipv6.u.byte[12] = 0xCC;
        a.as.ipv6.u.byte[13] = 0xDD;
        a.as.ipv6.u.byte[14] = 0xEE;
        a.as.ipv6.u.byte[15] = 0xFF;
        a.as.ipv6.scope_id = 0xF1F2F3F4;

        clarinet_addr b;
        memnoise(&b, sizeof(b));
        b.family = CLARINET_AF_INET6;
        b.as.ipv6.flowinfo = 0xAABBCCDD;
        b.as.ipv6.u.byte[0] = 0x00;
        b.as.ipv6.u.byte[1] = 0x11;
        b.as.ipv6.u.byte[2] = 0x22;
        b.as.ipv6.u.byte[3] = 0x33;
        b.as.ipv6.u.byte[4] = 0x44;
        b.as.ipv6.u.byte[5] = 0x55;
        b.as.ipv6.u.byte[6] = 0x66;
        b.as.ipv6.u.byte[7] = 0x77;
        b.as.ipv6.u.byte[8] = 0x88;
        b.as.ipv6.u.byte[9] = 0x99;
        b.as.ipv6.u.byte[10] = 0xAA;
        b.as.ipv6.u.byte[11] = 0xBB;
        b.as.ipv6.u.byte[12] = 0xCC;
        b.as.ipv6.u.byte[13] = 0xDD;
        b.as.ipv6.u.byte[14] = 0xEE;
        b.as.ipv6.u.byte[15] = 0xFF;
        b.as.ipv6.scope_id = 0xF1F2F3F4;

        REQUIRE(clarinet_addr_is_equal(&a, &b));

        clarinet_addr c = b;
        c.as.ipv6.flowinfo = 0;
        REQUIRE(clarinet_addr_is_equal(&b, &c));
    }

    SECTION("Custom ipv6 addresses (Not Equal)")
    {
        SECTION("Same ipv6 address but one has family NONE")
        {
            clarinet_addr a;
            memnoise(&a, sizeof(a));
            a.family = CLARINET_AF_NONE;
            a.as.ipv6.flowinfo = 0xAABBCCDD;
            a.as.ipv6.u.byte[0] = 0x00;
            a.as.ipv6.u.byte[1] = 0x11;
            a.as.ipv6.u.byte[2] = 0x22;
            a.as.ipv6.u.byte[3] = 0x33;
            a.as.ipv6.u.byte[4] = 0x44;
            a.as.ipv6.u.byte[5] = 0x55;
            a.as.ipv6.u.byte[6] = 0x66;
            a.as.ipv6.u.byte[7] = 0x77;
            a.as.ipv6.u.byte[8] = 0x88;
            a.as.ipv6.u.byte[9] = 0x99;
            a.as.ipv6.u.byte[10] = 0xAA;
            a.as.ipv6.u.byte[11] = 0xBB;
            a.as.ipv6.u.byte[12] = 0xCC;
            a.as.ipv6.u.byte[13] = 0xDD;
            a.as.ipv6.u.byte[14] = 0xEE;
            a.as.ipv6.u.byte[15] = 0xFF;
            a.as.ipv6.scope_id = 0xF1F2F3F4;

            clarinet_addr b;
            memnoise(&b, sizeof(b));
            b.family = CLARINET_AF_INET6;
            b.as.ipv6.flowinfo = 0xAABBCCDD;
            b.as.ipv6.u.byte[0] = 0x00;
            b.as.ipv6.u.byte[1] = 0x11;
            b.as.ipv6.u.byte[2] = 0x22;
            b.as.ipv6.u.byte[3] = 0x33;
            b.as.ipv6.u.byte[4] = 0x44;
            b.as.ipv6.u.byte[5] = 0x55;
            b.as.ipv6.u.byte[6] = 0x66;
            b.as.ipv6.u.byte[7] = 0x77;
            b.as.ipv6.u.byte[8] = 0x88;
            b.as.ipv6.u.byte[9] = 0x99;
            b.as.ipv6.u.byte[10] = 0xAA;
            b.as.ipv6.u.byte[11] = 0xBB;
            b.as.ipv6.u.byte[12] = 0xCC;
            b.as.ipv6.u.byte[13] = 0xDD;
            b.as.ipv6.u.byte[14] = 0xEE;
            b.as.ipv6.u.byte[15] = 0xFF;
            b.as.ipv6.scope_id = 0xF1F2F3F4;

            REQUIRE_FALSE(clarinet_addr_is_equal(&a, &b));
        }

        SECTION("Same ipv6 address but one has family INET")
        {
            clarinet_addr a;
            memnoise(&a, sizeof(a));
            a.family = CLARINET_AF_INET6;
            a.as.ipv6.flowinfo = 0xAABBCCDD;
            a.as.ipv6.u.byte[0] = 0x00;
            a.as.ipv6.u.byte[1] = 0x11;
            a.as.ipv6.u.byte[2] = 0x22;
            a.as.ipv6.u.byte[3] = 0x33;
            a.as.ipv6.u.byte[4] = 0x44;
            a.as.ipv6.u.byte[5] = 0x55;
            a.as.ipv6.u.byte[6] = 0x66;
            a.as.ipv6.u.byte[7] = 0x77;
            a.as.ipv6.u.byte[8] = 0x88;
            a.as.ipv6.u.byte[9] = 0x99;
            a.as.ipv6.u.byte[10] = 0xAA;
            a.as.ipv6.u.byte[11] = 0xBB;
            a.as.ipv6.u.byte[12] = 0xCC;
            a.as.ipv6.u.byte[13] = 0xDD;
            a.as.ipv6.u.byte[14] = 0xEE;
            a.as.ipv6.u.byte[15] = 0xFF;
            a.as.ipv6.scope_id = 0xF1F2F3F4;

            clarinet_addr b;
            memnoise(&b, sizeof(b));
            b.family = CLARINET_AF_INET;
            b.as.ipv6.flowinfo = 0xAABBCCDD;
            b.as.ipv6.u.byte[0] = 0x00;
            b.as.ipv6.u.byte[1] = 0x11;
            b.as.ipv6.u.byte[2] = 0x22;
            b.as.ipv6.u.byte[3] = 0x33;
            b.as.ipv6.u.byte[4] = 0x44;
            b.as.ipv6.u.byte[5] = 0x55;
            b.as.ipv6.u.byte[6] = 0x66;
            b.as.ipv6.u.byte[7] = 0x77;
            b.as.ipv6.u.byte[8] = 0x88;
            b.as.ipv6.u.byte[9] = 0x99;
            b.as.ipv6.u.byte[10] = 0xAA;
            b.as.ipv6.u.byte[11] = 0xBB;
            b.as.ipv6.u.byte[12] = 0xCC;
            b.as.ipv6.u.byte[13] = 0xDD;
            b.as.ipv6.u.byte[14] = 0xEE;
            b.as.ipv6.u.byte[15] = 0xFF;
            b.as.ipv6.scope_id = 0xF1F2F3F4;

            REQUIRE_FALSE(clarinet_addr_is_equal(&a, &b));
        }

        SECTION("Same ipv6 address but different flow info")
        {
            clarinet_addr a;
            memnoise(&a, sizeof(a));
            a.family = CLARINET_AF_INET6;
            a.as.ipv6.flowinfo = 0xAABBCCDD;
            a.as.ipv6.u.byte[0] = 0x00;
            a.as.ipv6.u.byte[1] = 0x11;
            a.as.ipv6.u.byte[2] = 0x22;
            a.as.ipv6.u.byte[3] = 0x33;
            a.as.ipv6.u.byte[4] = 0x44;
            a.as.ipv6.u.byte[5] = 0x55;
            a.as.ipv6.u.byte[6] = 0x66;
            a.as.ipv6.u.byte[7] = 0x77;
            a.as.ipv6.u.byte[8] = 0x88;
            a.as.ipv6.u.byte[9] = 0x99;
            a.as.ipv6.u.byte[10] = 0xAA;
            a.as.ipv6.u.byte[11] = 0xBB;
            a.as.ipv6.u.byte[12] = 0xCC;
            a.as.ipv6.u.byte[13] = 0xDD;
            a.as.ipv6.u.byte[14] = 0xEE;
            a.as.ipv6.u.byte[15] = 0xFF;
            a.as.ipv6.scope_id = 0xF1F2F3F4;

            clarinet_addr b;
            memnoise(&b, sizeof(b));
            b.family = CLARINET_AF_INET6;
            b.as.ipv6.flowinfo = 0x0A0B0C0D;
            b.as.ipv6.u.byte[0] = 0x00;
            b.as.ipv6.u.byte[1] = 0x11;
            b.as.ipv6.u.byte[2] = 0x22;
            b.as.ipv6.u.byte[3] = 0x33;
            b.as.ipv6.u.byte[4] = 0x44;
            b.as.ipv6.u.byte[5] = 0x55;
            b.as.ipv6.u.byte[6] = 0x66;
            b.as.ipv6.u.byte[7] = 0x77;
            b.as.ipv6.u.byte[8] = 0x88;
            b.as.ipv6.u.byte[9] = 0x99;
            b.as.ipv6.u.byte[10] = 0xAA;
            b.as.ipv6.u.byte[11] = 0xBB;
            b.as.ipv6.u.byte[12] = 0xCC;
            b.as.ipv6.u.byte[13] = 0xDD;
            b.as.ipv6.u.byte[14] = 0xEE;
            b.as.ipv6.u.byte[15] = 0xFF;
            b.as.ipv6.scope_id = 0xF1F2F3F4;

            REQUIRE(clarinet_addr_is_equal(&a, &b));
        }

        SECTION("Same ipv6 address but different scope id")
        {
            clarinet_addr a;
            memnoise(&a, sizeof(a));
            a.family = CLARINET_AF_INET6;
            a.as.ipv6.flowinfo = 0xAABBCCDD;
            a.as.ipv6.u.byte[0] = 0x00;
            a.as.ipv6.u.byte[1] = 0x11;
            a.as.ipv6.u.byte[2] = 0x22;
            a.as.ipv6.u.byte[3] = 0x33;
            a.as.ipv6.u.byte[4] = 0x44;
            a.as.ipv6.u.byte[5] = 0x55;
            a.as.ipv6.u.byte[6] = 0x66;
            a.as.ipv6.u.byte[7] = 0x77;
            a.as.ipv6.u.byte[8] = 0x88;
            a.as.ipv6.u.byte[9] = 0x99;
            a.as.ipv6.u.byte[10] = 0xAA;
            a.as.ipv6.u.byte[11] = 0xBB;
            a.as.ipv6.u.byte[12] = 0xCC;
            a.as.ipv6.u.byte[13] = 0xDD;
            a.as.ipv6.u.byte[14] = 0xEE;
            a.as.ipv6.u.byte[15] = 0xFF;
            a.as.ipv6.scope_id = 0x01020304;

            clarinet_addr b;
            memnoise(&b, sizeof(b));
            b.family = CLARINET_AF_INET6;
            b.as.ipv6.flowinfo = 0xAABBCCDD;
            b.as.ipv6.u.byte[0] = 0x00;
            b.as.ipv6.u.byte[1] = 0x11;
            b.as.ipv6.u.byte[2] = 0x22;
            b.as.ipv6.u.byte[3] = 0x33;
            b.as.ipv6.u.byte[4] = 0x44;
            b.as.ipv6.u.byte[5] = 0x55;
            b.as.ipv6.u.byte[6] = 0x66;
            b.as.ipv6.u.byte[7] = 0x77;
            b.as.ipv6.u.byte[8] = 0x88;
            b.as.ipv6.u.byte[9] = 0x99;
            b.as.ipv6.u.byte[10] = 0xAA;
            b.as.ipv6.u.byte[11] = 0xBB;
            b.as.ipv6.u.byte[12] = 0xCC;
            b.as.ipv6.u.byte[13] = 0xDD;
            b.as.ipv6.u.byte[14] = 0xEE;
            b.as.ipv6.u.byte[15] = 0xFF;
            b.as.ipv6.scope_id = 0xF1F2F3F4;

            REQUIRE_FALSE(clarinet_addr_is_equal(&a, &b));
        }

        SECTION("Same ipv4 address but different but one is ipv4 mapped to ipv6 ")
        {
            clarinet_addr a;
            memnoise(&a, sizeof(a));
            a.family = CLARINET_AF_INET;
            a.as.ipv4.u.byte[0] = 0x00;
            a.as.ipv4.u.byte[1] = 0x11;
            a.as.ipv4.u.byte[2] = 0x22;
            a.as.ipv4.u.byte[3] = 0x33;

            clarinet_addr b;
            memnoise(&b, sizeof(b));
            b.family = CLARINET_AF_INET6;
            b.as.ipv6.flowinfo = 0;
            b.as.ipv6.u.byte[0] = 0;
            b.as.ipv6.u.byte[1] = 0;
            b.as.ipv6.u.byte[2] = 0;
            b.as.ipv6.u.byte[3] = 0;
            b.as.ipv6.u.byte[4] = 0;
            b.as.ipv6.u.byte[5] = 0;
            b.as.ipv6.u.byte[6] = 0;
            b.as.ipv6.u.byte[7] = 0;
            b.as.ipv6.u.byte[8] = 0;
            b.as.ipv6.u.byte[9] = 0;
            b.as.ipv6.u.byte[10] = 0xFF;
            b.as.ipv6.u.byte[11] = 0xFF;
            b.as.ipv6.u.byte[12] = 0x00;
            b.as.ipv6.u.byte[13] = 0x11;
            b.as.ipv6.u.byte[14] = 0x22;
            b.as.ipv6.u.byte[15] = 0x33;
            b.as.ipv6.scope_id = 0;

            REQUIRE_FALSE(clarinet_addr_is_equal(&a, &b));
        }
    }
}

TEST_CASE("IPv4 Address Is Equivalent", "[address][ipv4]")
{
    SECTION("Constant ipv4 addresses (Equivalent)")
    {
        /* using two distinct allocations for the same contant on purpose */
        int index;
        clarinet_addr a, b;
        std::tie(index, a, b) = GENERATE(table<int, clarinet_addr, clarinet_addr>({
                { 0, clarinet_addr_ipv4_any, clarinet_addr_ipv4_any },
                { 1, clarinet_addr_ipv4_loopback, clarinet_addr_ipv4_loopback },
                { 2, clarinet_addr_ipv4_broadcast, clarinet_addr_ipv4_broadcast }
            }));

        FROM(index);

        REQUIRE(clarinet_addr_is_equivalent(&a, &b));
    }

    SECTION("Constant ipv4 addresses (Not Equivalent)")
    {
        int index;
        clarinet_addr a, b;
        std::tie(index, a, b) = GENERATE(table<int, clarinet_addr, clarinet_addr>({
                { 0, clarinet_addr_ipv4_any, clarinet_addr_ipv4_loopback },
                { 1, clarinet_addr_ipv4_any, clarinet_addr_ipv4_broadcast },
                { 2, clarinet_addr_ipv4_loopback, clarinet_addr_ipv4_broadcast }
            }));

        FROM(index);

        REQUIRE_FALSE(clarinet_addr_is_equivalent(&a, &b));
    }

    SECTION("Custom ipv4 addresses (Equivalent)")
    {
        clarinet_addr a;
        memnoise(&a, sizeof(a));
        a.family = CLARINET_AF_INET;
        a.as.ipv4.u.byte[0] = 0x00;
        a.as.ipv4.u.byte[1] = 0x11;
        a.as.ipv4.u.byte[2] = 0x22;
        a.as.ipv4.u.byte[3] = 0x33;

        clarinet_addr b;
        memnoise(&b, sizeof(b));
        b.family = CLARINET_AF_INET;
        b.as.ipv4.u.byte[0] = 0x00;
        b.as.ipv4.u.byte[1] = 0x11;
        b.as.ipv4.u.byte[2] = 0x22;
        b.as.ipv4.u.byte[3] = 0x33;

        REQUIRE(clarinet_addr_is_equivalent(&a, &b));
    }
}

TEST_CASE("IPv6 Address Is Equivalent", "[address][ipv6]")
{
    SECTION("Constant ipv6 addresses (Equivalent)")
    {
        /* using two distinct allocations for the same contant on purpose */
        int index;
        clarinet_addr a, b;
        std::tie(index, a, b) = GENERATE(table<int, clarinet_addr, clarinet_addr>({
                { 0, clarinet_addr_ipv6_any, clarinet_addr_ipv6_any },
                { 1, clarinet_addr_ipv6_loopback, clarinet_addr_ipv6_loopback  },
                { 2, clarinet_addr_ipv4mapped_loopback, clarinet_addr_ipv4mapped_loopback  },
                { 3, clarinet_addr_ipv4_loopback, clarinet_addr_ipv4mapped_loopback  }
            }));

        FROM(index);

        REQUIRE(clarinet_addr_is_equivalent(&a, &b));
    }

    SECTION("Constant ipv6 addresses (Not Equivalent)")
    {
        int index;
        clarinet_addr a, b;
        std::tie(index, a, b) = GENERATE(table<int, clarinet_addr, clarinet_addr>({
                { 0, clarinet_addr_ipv6_any, clarinet_addr_ipv6_loopback },
                { 1, clarinet_addr_ipv6_any, clarinet_addr_ipv4mapped_loopback },
                { 2, clarinet_addr_ipv6_loopback, clarinet_addr_ipv4mapped_loopback }
            }));

        FROM(index);

        REQUIRE_FALSE(clarinet_addr_is_equivalent(&a, &b));
    }

    SECTION("Custom ipv6 addresses (Equivalent)")
    {
        SECTION("Same ipv6 addresses")
        {
            clarinet_addr a;
            memnoise(&a, sizeof(a));
            a.family = CLARINET_AF_INET6;
            a.as.ipv6.flowinfo = 0xAABBCCDD;
            a.as.ipv6.u.byte[0] = 0x00;
            a.as.ipv6.u.byte[1] = 0x11;
            a.as.ipv6.u.byte[2] = 0x22;
            a.as.ipv6.u.byte[3] = 0x33;
            a.as.ipv6.u.byte[4] = 0x44;
            a.as.ipv6.u.byte[5] = 0x55;
            a.as.ipv6.u.byte[6] = 0x66;
            a.as.ipv6.u.byte[7] = 0x77;
            a.as.ipv6.u.byte[8] = 0x88;
            a.as.ipv6.u.byte[9] = 0x99;
            a.as.ipv6.u.byte[10] = 0xAA;
            a.as.ipv6.u.byte[11] = 0xBB;
            a.as.ipv6.u.byte[12] = 0xCC;
            a.as.ipv6.u.byte[13] = 0xDD;
            a.as.ipv6.u.byte[14] = 0xEE;
            a.as.ipv6.u.byte[15] = 0xFF;
            a.as.ipv6.scope_id = 0xF1F2F3F4;

            clarinet_addr b;
            memnoise(&b, sizeof(b));
            b.family = CLARINET_AF_INET6;
            b.as.ipv6.flowinfo = 0xAABBCCDD;
            b.as.ipv6.u.byte[0] = 0x00;
            b.as.ipv6.u.byte[1] = 0x11;
            b.as.ipv6.u.byte[2] = 0x22;
            b.as.ipv6.u.byte[3] = 0x33;
            b.as.ipv6.u.byte[4] = 0x44;
            b.as.ipv6.u.byte[5] = 0x55;
            b.as.ipv6.u.byte[6] = 0x66;
            b.as.ipv6.u.byte[7] = 0x77;
            b.as.ipv6.u.byte[8] = 0x88;
            b.as.ipv6.u.byte[9] = 0x99;
            b.as.ipv6.u.byte[10] = 0xAA;
            b.as.ipv6.u.byte[11] = 0xBB;
            b.as.ipv6.u.byte[12] = 0xCC;
            b.as.ipv6.u.byte[13] = 0xDD;
            b.as.ipv6.u.byte[14] = 0xEE;
            b.as.ipv6.u.byte[15] = 0xFF;
            b.as.ipv6.scope_id = 0xF1F2F3F4;

            REQUIRE(clarinet_addr_is_equivalent(&a, &b));

            clarinet_addr c = b;
            c.as.ipv6.flowinfo = 0;
            REQUIRE(clarinet_addr_is_equivalent(&b, &c));
        }

        SECTION("Same ipv4 address one is an ipv4 mapped to ipv6")
        {
            clarinet_addr a;
            memnoise(&a, sizeof(a));
            a.family = CLARINET_AF_INET;
            a.as.ipv4.u.byte[0] = 0x00;
            a.as.ipv4.u.byte[1] = 0x11;
            a.as.ipv4.u.byte[2] = 0x22;
            a.as.ipv4.u.byte[3] = 0x33;

            clarinet_addr b;
            memnoise(&b, sizeof(b));
            b.family = CLARINET_AF_INET6;
            b.as.ipv6.flowinfo = 0;
            b.as.ipv6.u.byte[0] = 0;
            b.as.ipv6.u.byte[1] = 0;
            b.as.ipv6.u.byte[2] = 0;
            b.as.ipv6.u.byte[3] = 0;
            b.as.ipv6.u.byte[4] = 0;
            b.as.ipv6.u.byte[5] = 0;
            b.as.ipv6.u.byte[6] = 0;
            b.as.ipv6.u.byte[7] = 0;
            b.as.ipv6.u.byte[8] = 0;
            b.as.ipv6.u.byte[9] = 0;
            b.as.ipv6.u.byte[10] = 0xFF;
            b.as.ipv6.u.byte[11] = 0xFF;
            b.as.ipv6.u.byte[12] = 0x00;
            b.as.ipv6.u.byte[13] = 0x11;
            b.as.ipv6.u.byte[14] = 0x22;
            b.as.ipv6.u.byte[15] = 0x33;
            b.as.ipv6.scope_id = 0;

            REQUIRE(clarinet_addr_is_equivalent(&a, &b));
        }
    }

    SECTION("Custom ipv6 addresses (Not Equivalent)")
    {
        SECTION("Same ipv6 address but one has family NONE")
        {
            clarinet_addr a;
            memnoise(&a, sizeof(a));
            a.family = CLARINET_AF_INET6;
            a.as.ipv6.flowinfo = 0xAABBCCDD;
            a.as.ipv6.u.byte[0] = 0x00;
            a.as.ipv6.u.byte[1] = 0x11;
            a.as.ipv6.u.byte[2] = 0x22;
            a.as.ipv6.u.byte[3] = 0x33;
            a.as.ipv6.u.byte[4] = 0x44;
            a.as.ipv6.u.byte[5] = 0x55;
            a.as.ipv6.u.byte[6] = 0x66;
            a.as.ipv6.u.byte[7] = 0x77;
            a.as.ipv6.u.byte[8] = 0x88;
            a.as.ipv6.u.byte[9] = 0x99;
            a.as.ipv6.u.byte[10] = 0xAA;
            a.as.ipv6.u.byte[11] = 0xBB;
            a.as.ipv6.u.byte[12] = 0xCC;
            a.as.ipv6.u.byte[13] = 0xDD;
            a.as.ipv6.u.byte[14] = 0xEE;
            a.as.ipv6.u.byte[15] = 0xFF;
            a.as.ipv6.scope_id = 0xF1F2F3F4;

            clarinet_addr b;
            memnoise(&b, sizeof(b));
            b.family = CLARINET_AF_INET;
            b.as.ipv6.flowinfo = 0xAABBCCDD;
            b.as.ipv6.u.byte[0] = 0x00;
            b.as.ipv6.u.byte[1] = 0x11;
            b.as.ipv6.u.byte[2] = 0x22;
            b.as.ipv6.u.byte[3] = 0x33;
            b.as.ipv6.u.byte[4] = 0x44;
            b.as.ipv6.u.byte[5] = 0x55;
            b.as.ipv6.u.byte[6] = 0x66;
            b.as.ipv6.u.byte[7] = 0x77;
            b.as.ipv6.u.byte[8] = 0x88;
            b.as.ipv6.u.byte[9] = 0x99;
            b.as.ipv6.u.byte[10] = 0xAA;
            b.as.ipv6.u.byte[11] = 0xBB;
            b.as.ipv6.u.byte[12] = 0xCC;
            b.as.ipv6.u.byte[13] = 0xDD;
            b.as.ipv6.u.byte[14] = 0xEE;
            b.as.ipv6.u.byte[15] = 0xFF;
            b.as.ipv6.scope_id = 0xF1F2F3F4;

            REQUIRE_FALSE(clarinet_addr_is_equivalent(&a, &b));
        }

        SECTION("Same ipv6 address but different flow info")
        {
            clarinet_addr a;
            memnoise(&a, sizeof(a));
            a.family = CLARINET_AF_INET6;
            a.as.ipv6.flowinfo = 0xAABBCCDD;
            a.as.ipv6.u.byte[0] = 0x00;
            a.as.ipv6.u.byte[1] = 0x11;
            a.as.ipv6.u.byte[2] = 0x22;
            a.as.ipv6.u.byte[3] = 0x33;
            a.as.ipv6.u.byte[4] = 0x44;
            a.as.ipv6.u.byte[5] = 0x55;
            a.as.ipv6.u.byte[6] = 0x66;
            a.as.ipv6.u.byte[7] = 0x77;
            a.as.ipv6.u.byte[8] = 0x88;
            a.as.ipv6.u.byte[9] = 0x99;
            a.as.ipv6.u.byte[10] = 0xAA;
            a.as.ipv6.u.byte[11] = 0xBB;
            a.as.ipv6.u.byte[12] = 0xCC;
            a.as.ipv6.u.byte[13] = 0xDD;
            a.as.ipv6.u.byte[14] = 0xEE;
            a.as.ipv6.u.byte[15] = 0xFF;
            a.as.ipv6.scope_id = 0xF1F2F3F4;

            clarinet_addr b;
            memnoise(&b, sizeof(b));
            b.family = CLARINET_AF_INET6;
            b.as.ipv6.flowinfo = 0x0A0B0C0D;
            b.as.ipv6.u.byte[0] = 0x00;
            b.as.ipv6.u.byte[1] = 0x11;
            b.as.ipv6.u.byte[2] = 0x22;
            b.as.ipv6.u.byte[3] = 0x33;
            b.as.ipv6.u.byte[4] = 0x44;
            b.as.ipv6.u.byte[5] = 0x55;
            b.as.ipv6.u.byte[6] = 0x66;
            b.as.ipv6.u.byte[7] = 0x77;
            b.as.ipv6.u.byte[8] = 0x88;
            b.as.ipv6.u.byte[9] = 0x99;
            b.as.ipv6.u.byte[10] = 0xAA;
            b.as.ipv6.u.byte[11] = 0xBB;
            b.as.ipv6.u.byte[12] = 0xCC;
            b.as.ipv6.u.byte[13] = 0xDD;
            b.as.ipv6.u.byte[14] = 0xEE;
            b.as.ipv6.u.byte[15] = 0xFF;
            b.as.ipv6.scope_id = 0xF1F2F3F4;

            REQUIRE(clarinet_addr_is_equivalent(&a, &b));
        }

        SECTION("Same ipv6 address but different cope id")
        {
            clarinet_addr a;
            memnoise(&a, sizeof(a));
            a.family = CLARINET_AF_INET6;
            a.as.ipv6.flowinfo = 0xAABBCCDD;
            a.as.ipv6.u.byte[0] = 0x00;
            a.as.ipv6.u.byte[1] = 0x11;
            a.as.ipv6.u.byte[2] = 0x22;
            a.as.ipv6.u.byte[3] = 0x33;
            a.as.ipv6.u.byte[4] = 0x44;
            a.as.ipv6.u.byte[5] = 0x55;
            a.as.ipv6.u.byte[6] = 0x66;
            a.as.ipv6.u.byte[7] = 0x77;
            a.as.ipv6.u.byte[8] = 0x88;
            a.as.ipv6.u.byte[9] = 0x99;
            a.as.ipv6.u.byte[10] = 0xAA;
            a.as.ipv6.u.byte[11] = 0xBB;
            a.as.ipv6.u.byte[12] = 0xCC;
            a.as.ipv6.u.byte[13] = 0xDD;
            a.as.ipv6.u.byte[14] = 0xEE;
            a.as.ipv6.u.byte[15] = 0xFF;
            a.as.ipv6.scope_id = 0x01020304;

            clarinet_addr b;
            memnoise(&b, sizeof(b));
            b.family = CLARINET_AF_INET6;
            b.as.ipv6.flowinfo = 0xAABBCCDD;
            b.as.ipv6.u.byte[0] = 0x00;
            b.as.ipv6.u.byte[1] = 0x11;
            b.as.ipv6.u.byte[2] = 0x22;
            b.as.ipv6.u.byte[3] = 0x33;
            b.as.ipv6.u.byte[4] = 0x44;
            b.as.ipv6.u.byte[5] = 0x55;
            b.as.ipv6.u.byte[6] = 0x66;
            b.as.ipv6.u.byte[7] = 0x77;
            b.as.ipv6.u.byte[8] = 0x88;
            b.as.ipv6.u.byte[9] = 0x99;
            b.as.ipv6.u.byte[10] = 0xAA;
            b.as.ipv6.u.byte[11] = 0xBB;
            b.as.ipv6.u.byte[12] = 0xCC;
            b.as.ipv6.u.byte[13] = 0xDD;
            b.as.ipv6.u.byte[14] = 0xEE;
            b.as.ipv6.u.byte[15] = 0xFF;
            b.as.ipv6.scope_id = 0xF1F2F3F4;

            REQUIRE_FALSE(clarinet_addr_is_equivalent(&a, &b));
        }
    }
}

TEST_CASE("Map to IPv4", "[address][ipv4]")
{
    SECTION("Valid map to ipv4")
    {
        const std::vector<std::tuple<int, clarinet_addr, clarinet_addr>> data =
        {
#if defined(CLARINET_ENABLE_IPV6)
            { 0, clarinet_addr_ipv4mapped_loopback, clarinet_addr_ipv4_loopback },
#endif
            { 1, clarinet_addr_ipv4_loopback, clarinet_addr_ipv4_loopback }
        };

        int index;
        clarinet_addr src, expected;
        std::tie(index, src, expected) = GENERATE_REF(from_range(data));

        FROM(index);

        clarinet_addr dst;
        const int errcode = clarinet_addr_map_to_ipv4(&dst, &src);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        REQUIRE(clarinet_addr_is_equal(&dst, &expected));
    }

    SECTION("Invalid map To ipv4")
    {
        const std::vector<std::tuple<int, clarinet_addr>> data =
        {
#if !defined(CLARINET_ENABLE_IPV6)
            { 0, clarinet_addr_ipv6_loopback },
            { 1, clarinet_addr_ipv4mapped_loopback },
#endif
            { 2, clarinet_addr_none }
        };

        int index;
        clarinet_addr src;
        std::tie(index, src) = GENERATE_REF(from_range(data));
        
        FROM(index);

        clarinet_addr dst;
        const int errcode = clarinet_addr_map_to_ipv4(&dst, &src);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
    }
}

TEST_CASE("Map to IPv6", "[address][ipv6]")
{
    SECTION("Valid map to ipv6")
    {
#if defined(CLARINET_ENABLE_IPV6)
        int index;
        clarinet_addr src, expected;
        std::tie(index, src, expected) = GENERATE(table<int, clarinet_addr, clarinet_addr>({
                { 0, clarinet_addr_ipv4_loopback, clarinet_addr_ipv4mapped_loopback },
                { 1, clarinet_addr_ipv6_loopback, clarinet_addr_ipv6_loopback }
            }));
        
        FROM(index);

        clarinet_addr dst;
        const int errcode = clarinet_addr_map_to_ipv6(&dst, &src);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        REQUIRE(clarinet_addr_is_equal(&dst, &expected));
#endif
    }

    SECTION("Invalid map To ipv6")
    {
        const std::vector<std::tuple<int, clarinet_addr>> data =
        {
#if !defined(CLARINET_ENABLE_IPV6)
            {1, clarinet_addr_ipv4mapped_loopback },
            {2, clarinet_addr_ipv6_loopback },
#endif
            {3, clarinet_addr_none }
        };

        int index;
        clarinet_addr src;
        std::tie(index, src) = GENERATE_REF(from_range(data));

        FROM(index);

        clarinet_addr dst;
        const int errcode = clarinet_addr_map_to_ipv6(&dst, &src);
        REQUIRE_FALSE(Error(errcode) == Error(CLARINET_ENONE));
    }
}

TEST_CASE("Address To String", "[address]")
{
    SECTION("With invalid arguments")
    {
        SECTION("NULL dst")
        {
            clarinet_addr src = clarinet_addr_ipv4_any;
            const int errcode = clarinet_addr_to_string(NULL, 0, &src);
            REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
        }

        SECTION("NULL src")
        {
            char dst[CLARINET_ADDR_STRLEN];
            const int errcode = clarinet_addr_to_string(dst, sizeof(dst), NULL);
            REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
        }

        SECTION("Not enough space on dst for ipv4")
        {
            clarinet_addr src = clarinet_addr_ipv4_any;
            char dst[4];
            const int errcode = clarinet_addr_to_string(dst, sizeof(dst), &src);
            REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
        }

        SECTION("Not enough space on dst for ipv6")
        {
            clarinet_addr src = {0};
            memnoise(&src, sizeof(src));
            src.family = CLARINET_AF_INET6;
            src.as.ipv6.flowinfo = 0xA0A1A2A3;
            src.as.ipv6.u.byte[0] = 0xB0;
            src.as.ipv6.u.byte[1] = 0xB1;
            src.as.ipv6.u.byte[2] = 0xB2;
            src.as.ipv6.u.byte[3] = 0xB3;
            src.as.ipv6.u.byte[4] = 0xB4;
            src.as.ipv6.u.byte[5] = 0xB5;
            src.as.ipv6.u.byte[6] = 0xB6;
            src.as.ipv6.u.byte[7] = 0xB7;
            src.as.ipv6.u.byte[8] = 0xB8;
            src.as.ipv6.u.byte[9] = 0xB9;
            src.as.ipv6.u.byte[10] = 0xBA;
            src.as.ipv6.u.byte[11] = 0xBB;
            src.as.ipv6.u.byte[12] = 0xBC;
            src.as.ipv6.u.byte[13] = 0xBD;
            src.as.ipv6.u.byte[14] = 0xBE;
            src.as.ipv6.u.byte[15] = 0xBF;
            src.as.ipv6.scope_id = 3233923779;
            char dst[1] = {0};
            memnoise(dst, sizeof(dst));
            const int errcode = clarinet_addr_to_string(dst, sizeof(dst), &src);
            REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
        }

        SECTION("Family not supported")
        {
            clarinet_addr custom_ipv6 = {0};
            memnoise(&custom_ipv6, sizeof(custom_ipv6));
            custom_ipv6.family = CLARINET_AF_INET6;
            custom_ipv6.as.ipv6.flowinfo = 0xA0A1A2A3;
            custom_ipv6.as.ipv6.u.byte[0] = 0xB0;
            custom_ipv6.as.ipv6.u.byte[1] = 0xB1;
            custom_ipv6.as.ipv6.u.byte[2] = 0xB2;
            custom_ipv6.as.ipv6.u.byte[3] = 0xB3;
            custom_ipv6.as.ipv6.u.byte[4] = 0xB4;
            custom_ipv6.as.ipv6.u.byte[5] = 0xB5;
            custom_ipv6.as.ipv6.u.byte[6] = 0xB6;
            custom_ipv6.as.ipv6.u.byte[7] = 0xB7;
            custom_ipv6.as.ipv6.u.byte[8] = 0xB8;
            custom_ipv6.as.ipv6.u.byte[9] = 0xB9;
            custom_ipv6.as.ipv6.u.byte[10] = 0xBA;
            custom_ipv6.as.ipv6.u.byte[11] = 0xBB;
            custom_ipv6.as.ipv6.u.byte[12] = 0xBC;
            custom_ipv6.as.ipv6.u.byte[13] = 0xBD;
            custom_ipv6.as.ipv6.u.byte[14] = 0xBE;
            custom_ipv6.as.ipv6.u.byte[15] = 0xBF;
            custom_ipv6.as.ipv6.scope_id = 3233923779;

            clarinet_addr custom_none = custom_ipv6;
            custom_none.family = CLARINET_AF_NONE;

            const std::vector<std::tuple<int, clarinet_addr>> data =
            {
#if !defined(CLARINET_ENABLE_IPV6)
                { 0, custom_ipv6 },
#endif
                { 1, custom_none }
            };

            int index;
            clarinet_addr src;
            std::tie(index, src) = GENERATE_REF(from_range(data));
            
            FROM(index);

            char dst[CLARINET_ADDR_STRLEN] = {0};
            memnoise(dst, sizeof(dst));
            const int errcode = clarinet_addr_to_string(dst, sizeof(dst), &src);
            REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
        }
    }

    SECTION("Wiht valid arguments")
    {
        SECTION("Ipv4 to string")
        {
            int index;
            clarinet_addr src;
            const char* expected;
            std::tie(index, src, expected) = GENERATE(table<int, clarinet_addr, const char*>({
                    { 0, clarinet_addr_ipv4_any, "0.0.0.0" },
                    { 1, clarinet_addr_ipv4_loopback, "127.0.0.1" },
                    { 2, clarinet_addr_ipv4_broadcast, "255.255.255.255" }
                }));

            FROM(index);

            char dst[CLARINET_ADDR_STRLEN] = {0};
            memnoise(dst, sizeof(dst));
            const int n = clarinet_addr_to_string(dst, sizeof(dst), &src);
            REQUIRE(n > 0);
            EXPLAIN("Expected: '%s'", expected);
            EXPLAIN("Actual: '%s'", dst);
            REQUIRE((size_t)n == strlen(expected));
            REQUIRE_THAT(std::string(dst), Equals(expected));
        }

        SECTION("Ipv6 to string")
        {
#if defined(CLARINET_ENABLE_IPV6)
            clarinet_addr ipv4mapped_with_scope_id = clarinet_addr_ipv4mapped_loopback;
            ipv4mapped_with_scope_id.as.ipv6.scope_id = 1234567890;

            clarinet_addr custom_ipv6 = {0};
            memnoise(&custom_ipv6, sizeof(custom_ipv6));
            custom_ipv6.family = CLARINET_AF_INET6;
            custom_ipv6.as.ipv6.flowinfo = 0xA0A1A2A3;
            custom_ipv6.as.ipv6.u.byte[0] = 0xB0;
            custom_ipv6.as.ipv6.u.byte[1] = 0xB1;
            custom_ipv6.as.ipv6.u.byte[2] = 0xB2;
            custom_ipv6.as.ipv6.u.byte[3] = 0xB3;
            custom_ipv6.as.ipv6.u.byte[4] = 0xB4;
            custom_ipv6.as.ipv6.u.byte[5] = 0xB5;
            custom_ipv6.as.ipv6.u.byte[6] = 0xB6;
            custom_ipv6.as.ipv6.u.byte[7] = 0xB7;
            custom_ipv6.as.ipv6.u.byte[8] = 0xB8;
            custom_ipv6.as.ipv6.u.byte[9] = 0xB9;
            custom_ipv6.as.ipv6.u.byte[10] = 0xBA;
            custom_ipv6.as.ipv6.u.byte[11] = 0xBB;
            custom_ipv6.as.ipv6.u.byte[12] = 0xBC;
            custom_ipv6.as.ipv6.u.byte[13] = 0xBD;
            custom_ipv6.as.ipv6.u.byte[14] = 0xBE;
            custom_ipv6.as.ipv6.u.byte[15] = 0xBF;
            custom_ipv6.as.ipv6.scope_id = 3233923779;
            
            int index;
            clarinet_addr src;
            const char* expected;
            std::tie(index, src, expected) = GENERATE_REF(table<int, clarinet_addr, const char*>({
                    {0, clarinet_addr_ipv6_any, "::" },
                    {1, clarinet_addr_ipv6_loopback, "::1" },
                    {2, clarinet_addr_ipv4mapped_loopback, "::ffff:127.0.0.1" },
                    {3, ipv4mapped_with_scope_id, "::ffff:127.0.0.1%1234567890" },
                    {4, custom_ipv6, "b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779" }
                }));

            FROM(index);

            char dst[CLARINET_ADDR_STRLEN] = {0};
            memnoise(dst, sizeof(dst));
            const int n = clarinet_addr_to_string(dst, sizeof(dst), &src);
            REQUIRE(n > 0);
            EXPLAIN("Expected: '%s'", expected);
            EXPLAIN("Actual: '%s'", dst);
            REQUIRE((size_t)n == strlen(expected));
            REQUIRE_THAT(std::string(dst), Equals(expected));
#endif
        }
    }
}

TEST_CASE("Address From String", "[address]")
{
    SECTION("NULL dst")
    {
        const char* src = "192.168.0.1";
        const int errcode = clarinet_addr_from_string(NULL, src, strlen(src));
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
    }

    SECTION("NULL src")
    {
        clarinet_addr dst = {0};
        const int errcode = clarinet_addr_from_string(&dst, NULL, 0);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
    }

    SECTION("Zero srclen")
    {
        clarinet_addr dst = {0};
        const char* src = "192.168.0.1";
        const int errcode = clarinet_addr_from_string(&dst, src, 0);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
    }

    SECTION("Insufficient srclen")
    {
        clarinet_addr dst = {0};
        const char* src = "192.168.0.1";
        const int errcode = clarinet_addr_from_string(&dst, src, strlen(src) >> 1);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
    }

    SECTION("Invalid addresses")
    {
        const std::vector<std::tuple<int, const char*>> data =
        {
            {  0, "" },
            {  1, "0" },
            {  2, "0." },
            {  3, "0.0" },
            {  4, "0.0.0" },
            {  5, "127.0.0.a" },
            {  6, "255.255.255.255a" },
            {  7, "255.255.255.ff" },
            {  8, "10.0.0.-1" },
            {  9, "ff.ff.ff.ff" },
            { 10, "a0.a1.a2.a3" },
            { 11, "0a.1a.2a.3a" },
#if !defined(CLARINET_ENABLE_IPV6)
            { 12, "::ffff:127.0.0.1" }, 
            { 13, "::ffff:127.0.0.1%1234567890" },
            { 14, "b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf" },
            { 15, "b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebfK" },
            { 16, "b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779" },
#endif
            { 17, ":" },
            { 18, "::%" },
            { 19, "::1%" },
            { 20, "::ffff::1" },
            { 21, "::ffff::1" },
            { 22, "::1%4294969999" },
            { 23, "::1%-100" },
            { 24, "::1%0x12345" },
            { 25, "b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%00003233923779" },
            { 26, "10.0.0.1a" },
            { 27, "127.0.0.001" },   
            { 28, "::1%00012345" },
            { 29, "0000:0000:0000:0000:0000:ffff:127.0.0.333" },
            /* inet_pton(3) on Linux does not accept the IPv4 dot-decimal notation in the last dword containing leading 
             * zeros like it does not accepted for IPv4 addresses in general, but on Windows it is valid to have 
             * decimals with leading zeros inside an IPv6 address string (though not when inet_pton converts a pure 
             * IPv4 address). */
#if 0
            {30, "0000:0000:0000:0000:0000:ffff:127.0.0.001"},
#endif
            { 31, "0000:0000:0000:0000:0000:ffff:127.0.0.ff" },
            { 32, "0000:0000:0000:0000:0000:ffff:127.0.0.1%0000000000" },
            { 33, "127.0.0.333" },
            { 34, "[::]" },
            { 35, "[::1]" },
            { 36, "[::1]:0" },
            { 37, "[::1]:12345" },
            { 38, "127.0.0.1:0" },
            { 39, "127.0.0.1:12345" },
        };

        int index;
        const char* src;
        std::tie(index, src) = GENERATE_REF(from_range(data));

        FROM(index);

        clarinet_addr dst = {0};
        memnoise(&dst, sizeof(dst));
        const int errcode = clarinet_addr_from_string(&dst, src, strlen(src));
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
    }

    SECTION("Valid addresses")
    {
#if defined(CLARINET_ENABLE_IPV6)
        clarinet_addr ipv4mapped_with_scope_id = clarinet_addr_ipv4mapped_loopback;
        ipv4mapped_with_scope_id.as.ipv6.scope_id = 1234567890;

        clarinet_addr custom_ipv6 = {0};
        memnoise(&custom_ipv6, sizeof(custom_ipv6));
        custom_ipv6.family = CLARINET_AF_INET6;
        custom_ipv6.as.ipv6.flowinfo = 0;
        custom_ipv6.as.ipv6.u.byte[0] = 0xB0;
        custom_ipv6.as.ipv6.u.byte[1] = 0xB1;
        custom_ipv6.as.ipv6.u.byte[2] = 0xB2;
        custom_ipv6.as.ipv6.u.byte[3] = 0xB3;
        custom_ipv6.as.ipv6.u.byte[4] = 0xB4;
        custom_ipv6.as.ipv6.u.byte[5] = 0xB5;
        custom_ipv6.as.ipv6.u.byte[6] = 0xB6;
        custom_ipv6.as.ipv6.u.byte[7] = 0xB7;
        custom_ipv6.as.ipv6.u.byte[8] = 0xB8;
        custom_ipv6.as.ipv6.u.byte[9] = 0xB9;
        custom_ipv6.as.ipv6.u.byte[10] = 0xBA;
        custom_ipv6.as.ipv6.u.byte[11] = 0xBB;
        custom_ipv6.as.ipv6.u.byte[12] = 0xBC;
        custom_ipv6.as.ipv6.u.byte[13] = 0xBD;
        custom_ipv6.as.ipv6.u.byte[14] = 0xBE;
        custom_ipv6.as.ipv6.u.byte[15] = 0xBF;
        custom_ipv6.as.ipv6.scope_id = 3233923779;

        /* flowinfo is not compared */
        clarinet_addr custom_ipv6_with_flowinfo = custom_ipv6;
        custom_ipv6_with_flowinfo.as.ipv6.flowinfo = 0xFF;
        
        /* leading zeros ignored */
        clarinet_addr custom_ipv6_with_short_scope_id = custom_ipv6;
        custom_ipv6_with_short_scope_id.as.ipv6.scope_id = 12345;
        
        clarinet_addr custom_ipv6_f = clarinet_addr_ipv6_any;
        custom_ipv6_f.as.ipv6.u.byte[15] = 0x0F;
        
        clarinet_addr custom_ipv6_feef_1886 = clarinet_addr_ipv6_any;
        custom_ipv6_feef_1886.as.ipv6.u.byte[12] = 0xFE;
        custom_ipv6_feef_1886.as.ipv6.u.byte[13] = 0xEF;
        custom_ipv6_feef_1886.as.ipv6.u.byte[14] = 0x18;
        custom_ipv6_feef_1886.as.ipv6.u.byte[15] = 0x86;
       
        
#if 0
        /* Referenced in test instance 14 - keep this commented while the issue remains open */
        clarinet_addr custom_ipv6_ffff_127_0_0_10 = clarinet_addr_ipv4mapped_loopback;
        custom_ipv6_ffff_127_0_0_10.as.ipv6.u.byte[15] = 10;
#endif
        
       
#endif

        const std::vector<std::tuple<int, const char*, const char*, clarinet_addr>> data =
        {
            {  0, "0.0.0.0",                                            "0.0.0.0",                                            clarinet_addr_ipv4_any },
            {  1, "127.0.0.1",                                          "127.0.0.1",                                          clarinet_addr_ipv4_loopback },
            {  2, "255.255.255.255",                                    "255.255.255.255",                                    clarinet_addr_ipv4_broadcast },
#if defined(CLARINET_ENABLE_IPV6)                           
            {  3, "::",                                                 "::",                                                 clarinet_addr_ipv6_any },
            {  4, "0000:0000:0000:0000:0000:0000:0000:0000",            "::",                                                 clarinet_addr_ipv6_any },
            {  5, "::1",                                                "::1",                                                clarinet_addr_ipv6_loopback },
            {  6, "::ffff:127.0.0.1",                                   "::ffff:127.0.0.1",                                   clarinet_addr_ipv4mapped_loopback },
            {  7, "0000:0000:0000:0000:0000:ffff:127.0.0.1%0",          "::ffff:127.0.0.1",                                   clarinet_addr_ipv4mapped_loopback },
            {  8, "::ffff:127.0.0.1%1234567890",                        "::ffff:127.0.0.1%1234567890",                        ipv4mapped_with_scope_id },
            {  9, "b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779", "b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779", custom_ipv6 },
            { 10, "b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779", "b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779", custom_ipv6_with_flowinfo },
            { 11, "b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%12345",      "b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%12345",      custom_ipv6_with_short_scope_id },
            { 12, "::F",                                                "::f",                                                custom_ipv6_f },
            { 13, "::FEEF:1886",                                        "::254.239.24.134",                                   custom_ipv6_feef_1886 }, /* CHECK: shouldn't ::FEEF:1886 be reconstructed as ::feef:1886 instead of ::254.239.24.134 ? */
            /* inet_pton(3) on Linux does not accept the IPv4 dot-decimal notation in the last dword containing leading
             * zeros like it does not accepted for IPv4 addresses in general, but on Windows it is valid to have
             * decimals with leading zeros inside an IPv6 address string (though not when inet_pton converts a pure
             * IPv4 address). */
#if 0
            {14, "0000:0000:0000:0000:0000:ffff:127.0.0.010",          "::ffff:127.0.0.10",                                  custom_ipv6_ffff_127_0_0_10},
#endif

#endif
        };

        int index;
        const char* src;
        const char* reconstruction;
        clarinet_addr expected;
        std::tie(index, src, reconstruction, expected) = GENERATE_REF(from_range(data));

        FROM(index);

        clarinet_addr dst = {0};
        memnoise(&dst, sizeof(dst));
        const int errcode = clarinet_addr_from_string(&dst, src, strlen(src));
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        REQUIRE(clarinet_addr_is_equal(&dst, &expected));

        char s[CLARINET_ADDR_STRLEN] = {0};
        memnoise(&s, sizeof(s));
        const int n = clarinet_addr_to_string(s, sizeof(s), &dst);
        REQUIRE(n > 0);
        EXPLAIN("Expected: %s", reconstruction);
        EXPLAIN("Actual: %s", s);       
        REQUIRE((size_t)n == strlen(reconstruction));
        REQUIRE_THAT(std::string(s), Equals(reconstruction));
    }
}

TEST_CASE("Endpoint Size", "[endpoint]")
{

    REQUIRE(sizeof(clarinet_endpoint) == 32);
    REQUIRE(sizeof(((clarinet_endpoint*)0)->addr) == sizeof(clarinet_addr));
    REQUIRE(sizeof(((clarinet_endpoint*)0)->port) == sizeof(uint16_t));
}

TEST_CASE("Endpoint Max Strlen", "[endpoint]")
{
    REQUIRE(CLARINET_ENDPOINT_STRLEN == 65); /* "[0000:0000:0000:0000:0000:ffff:255.255.255.255%4294967295]:65535" */
}

TEST_CASE("Endpoint To String", "[endpoint]")
{
    SECTION("With invalid arguments")
    {
        SECTION("NULL dst")
        {
            clarinet_endpoint src = {clarinet_addr_ipv4_any, 0};
            const int errcode = clarinet_endpoint_to_string(NULL, 0, &src);
            REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
        }

        SECTION("NULL src")
        {
            char dst[CLARINET_ENDPOINT_STRLEN];
            const int errcode = clarinet_endpoint_to_string(dst, sizeof(dst), NULL);
            REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
        }

        SECTION("Not enough space on dst for ipv4")
        {
            clarinet_endpoint src = {clarinet_addr_ipv4_any, 0};
            char dst[4];
            const int errcode = clarinet_endpoint_to_string(dst, sizeof(dst), &src);
            REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
        }

        SECTION("Not enough space on dst for ipv6")
        {
            clarinet_endpoint src = {{0}};
            memnoise(&src, sizeof(src));
            src.addr.family = CLARINET_AF_INET6;
            src.addr.as.ipv6.flowinfo = 0xA0A1A2A3;
            src.addr.as.ipv6.u.byte[0] = 0xB0;
            src.addr.as.ipv6.u.byte[1] = 0xB1;
            src.addr.as.ipv6.u.byte[2] = 0xB2;
            src.addr.as.ipv6.u.byte[3] = 0xB3;
            src.addr.as.ipv6.u.byte[4] = 0xB4;
            src.addr.as.ipv6.u.byte[5] = 0xB5;
            src.addr.as.ipv6.u.byte[6] = 0xB6;
            src.addr.as.ipv6.u.byte[7] = 0xB7;
            src.addr.as.ipv6.u.byte[8] = 0xB8;
            src.addr.as.ipv6.u.byte[9] = 0xB9;
            src.addr.as.ipv6.u.byte[10] = 0xBA;
            src.addr.as.ipv6.u.byte[11] = 0xBB;
            src.addr.as.ipv6.u.byte[12] = 0xBC;
            src.addr.as.ipv6.u.byte[13] = 0xBD;
            src.addr.as.ipv6.u.byte[14] = 0xBE;
            src.addr.as.ipv6.u.byte[15] = 0xBF;
            src.addr.as.ipv6.scope_id = 3233923779;

            char dst[1] = {0};
            memnoise(dst, sizeof(dst));
            const int errcode = clarinet_endpoint_to_string(dst, sizeof(dst), &src);
            REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
        }

        SECTION("Family not supported")
        {
            clarinet_endpoint custom_ipv6 = {{0}};
            memnoise(&custom_ipv6, sizeof(custom_ipv6));
            custom_ipv6.addr.family = CLARINET_AF_INET6;
            custom_ipv6.addr.as.ipv6.flowinfo = 0xA0A1A2A3;
            custom_ipv6.addr.as.ipv6.u.byte[0] = 0xB0;
            custom_ipv6.addr.as.ipv6.u.byte[1] = 0xB1;
            custom_ipv6.addr.as.ipv6.u.byte[2] = 0xB2;
            custom_ipv6.addr.as.ipv6.u.byte[3] = 0xB3;
            custom_ipv6.addr.as.ipv6.u.byte[4] = 0xB4;
            custom_ipv6.addr.as.ipv6.u.byte[5] = 0xB5;
            custom_ipv6.addr.as.ipv6.u.byte[6] = 0xB6;
            custom_ipv6.addr.as.ipv6.u.byte[7] = 0xB7;
            custom_ipv6.addr.as.ipv6.u.byte[8] = 0xB8;
            custom_ipv6.addr.as.ipv6.u.byte[9] = 0xB9;
            custom_ipv6.addr.as.ipv6.u.byte[10] = 0xBA;
            custom_ipv6.addr.as.ipv6.u.byte[11] = 0xBB;
            custom_ipv6.addr.as.ipv6.u.byte[12] = 0xBC;
            custom_ipv6.addr.as.ipv6.u.byte[13] = 0xBD;
            custom_ipv6.addr.as.ipv6.u.byte[14] = 0xBE;
            custom_ipv6.addr.as.ipv6.u.byte[15] = 0xBF;
            custom_ipv6.addr.as.ipv6.scope_id = 3233923779;

            clarinet_endpoint custom_none = custom_ipv6;
            custom_none.addr.family = CLARINET_AF_NONE;

            const std::vector<std::tuple<int, clarinet_endpoint>> data =
            {
#if !defined(CLARINET_ENABLE_IPV6)
                { 0, custom_ipv6 },
#endif
                { 1, custom_none }
            };

            int index;
            clarinet_endpoint src;
            std::tie(index, src) = GENERATE_REF(from_range(data));

            FROM(index);

            char dst[CLARINET_ENDPOINT_STRLEN] = {0};
            memnoise(dst, sizeof(dst));
            const int errcode = clarinet_endpoint_to_string(dst, sizeof(dst), &src);
            REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
        }
    }

    SECTION("With valid arguments")
    {
        SECTION("IPv4 to string")
        {
            int index;
            clarinet_endpoint src;
            const char* expected;
            std::tie(index, src, expected) = GENERATE(table<int, clarinet_endpoint, const char*>({
                    { 0, { clarinet_addr_ipv4_any, 0 }, "0.0.0.0:0" },
                    { 1, { clarinet_addr_ipv4_loopback, 0 }, "127.0.0.1:0" },
                    { 2, { clarinet_addr_ipv4_broadcast, 0 }, "255.255.255.255:0" },
                    { 3, { clarinet_addr_ipv4_any, 65535 }, "0.0.0.0:65535" },
                    { 4, { clarinet_addr_ipv4_loopback, 65535 }, "127.0.0.1:65535" },
                    { 5, { clarinet_addr_ipv4_broadcast, 65535 }, "255.255.255.255:65535" }
                }));

            FROM(index);

            char dst[CLARINET_ENDPOINT_STRLEN] = {0};
            memnoise(dst, sizeof(dst));
            const int n = clarinet_endpoint_to_string(dst, sizeof(dst), &src);
            REQUIRE(n > 0);
            EXPLAIN("Expected: '%s'", expected);
            EXPLAIN("Actual: '%s'", dst);
            REQUIRE((size_t)n == strlen(expected));
            REQUIRE_THAT(std::string(dst), Equals(expected));
        }

        SECTION("IPV6 to string")
        {
#if defined(CLARINET_ENABLE_IPV6)
            clarinet_addr ipv4mapped_with_scope_id = clarinet_addr_ipv4mapped_loopback;
            ipv4mapped_with_scope_id.as.ipv6.scope_id = 1234567890;

            clarinet_addr custom_ipv6 = {0};
            memnoise(&custom_ipv6, sizeof(custom_ipv6));
            custom_ipv6.family = CLARINET_AF_INET6;
            custom_ipv6.as.ipv6.flowinfo = 0xA0A1A2A3;
            custom_ipv6.as.ipv6.u.byte[0] = 0xB0;
            custom_ipv6.as.ipv6.u.byte[1] = 0xB1;
            custom_ipv6.as.ipv6.u.byte[2] = 0xB2;
            custom_ipv6.as.ipv6.u.byte[3] = 0xB3;
            custom_ipv6.as.ipv6.u.byte[4] = 0xB4;
            custom_ipv6.as.ipv6.u.byte[5] = 0xB5;
            custom_ipv6.as.ipv6.u.byte[6] = 0xB6;
            custom_ipv6.as.ipv6.u.byte[7] = 0xB7;
            custom_ipv6.as.ipv6.u.byte[8] = 0xB8;
            custom_ipv6.as.ipv6.u.byte[9] = 0xB9;
            custom_ipv6.as.ipv6.u.byte[10] = 0xBA;
            custom_ipv6.as.ipv6.u.byte[11] = 0xBB;
            custom_ipv6.as.ipv6.u.byte[12] = 0xBC;
            custom_ipv6.as.ipv6.u.byte[13] = 0xBD;
            custom_ipv6.as.ipv6.u.byte[14] = 0xBE;
            custom_ipv6.as.ipv6.u.byte[15] = 0xBF;
            custom_ipv6.as.ipv6.scope_id = 3233923779;

            int index;
            clarinet_endpoint src;
            const char* expected;
            std::tie(index, src, expected) = GENERATE_REF(table<int, clarinet_endpoint, const char*>({
                    { 0, { clarinet_addr_ipv6_any,                   0 }, "[::]:0" },
                    { 1, { clarinet_addr_ipv6_loopback,              0 }, "[::1]:0" },
                    { 2, { clarinet_addr_ipv4mapped_loopback,        0 }, "[::ffff:127.0.0.1]:0" },
                    { 3, { ipv4mapped_with_scope_id,                 0 }, "[::ffff:127.0.0.1%1234567890]:0" },
                    { 4, { custom_ipv6,                              0 }, "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779]:0" },
                    { 5, { clarinet_addr_ipv6_any,               65535 }, "[::]:65535" },
                    { 6, { clarinet_addr_ipv6_loopback,          65535 }, "[::1]:65535" },
                    { 7, { clarinet_addr_ipv4mapped_loopback,    65535 }, "[::ffff:127.0.0.1]:65535" },
                    { 8, { ipv4mapped_with_scope_id,             65535 }, "[::ffff:127.0.0.1%1234567890]:65535" },
                    { 9, { custom_ipv6,                          65535 }, "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779]:65535" }
                }));

            FROM(index);

            char dst[CLARINET_ENDPOINT_STRLEN] = {0};
            memnoise(dst, sizeof(dst));
            const int n = clarinet_endpoint_to_string(dst, sizeof(dst), &src);
            REQUIRE(n > 0);
            EXPLAIN("Expected: '%s'", expected);
            EXPLAIN("Actual: '%s'", dst);
            REQUIRE((size_t)n == strlen(expected));
            REQUIRE_THAT(std::string(dst), Equals(expected));
#endif
        }
    }
}
    
TEST_CASE("Endpoint From string", "[endpoint]")
{
    SECTION("NULL dst")
    {
        const char* src = "192.168.0.1:0";
        const int errcode = clarinet_endpoint_from_string(NULL, src, strlen(src));
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
    }

    SECTION("NULL src")
    {
        clarinet_endpoint dst = {{0}};
        const int errcode = clarinet_endpoint_from_string(&dst, NULL, 0);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
    }

    SECTION("Zero srclen")
    {
        clarinet_endpoint dst = {{0}};
        const char* src = "192.168.0.1:0";
        const int errcode = clarinet_endpoint_from_string(&dst, src, 0);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
    }

    SECTION("Insufficient srclen")
    {
        clarinet_endpoint dst = {{0}};
        const char* src = "192.168.0.1:0";
        const int errcode = clarinet_endpoint_from_string(&dst, src, strlen(src) >> 1);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
    }

    SECTION("Invalid endpoints")
    {
        const std::vector<std::tuple<int, const char*>> data =
        {
            {  0, "" },
            {  1, "0" },
            {  2, "0." },
            {  3, "0.0" },
            {  4, "0.0.0" },
            {  5, "0.0.0.0" },
            {  6, "0.0.0.0:" },
            {  7, "0.0.0.0::13" },
            {  8, "127.0.0.1" },
            {  9, "127.0.0.001" },
            { 10, "127.0.0.1:" },
            { 11, "127.0.0.1::9" },
            { 12, "127.0.0.1:0x1234" },
            { 13, "127.0.0.1:99999" },
            { 14, "127.0.0.1:00999" },
            { 15, "127.0.0.1:12345FooBar" },
            { 16, "FooBar127.0.0.1:12345" },
            { 17, "127.0.0.a" },
            { 18, "255.255.255.255" },
            { 19, "255.255.255.255a:1234" },
            { 20, "255.255.255.ff:1234" },
            { 21, "10.0.0.-1:1234" },
            { 22, "ff.ff.ff.ff:1234" },
            { 23, "a0.a1.a2.a3:1234" },
            { 24, "0a.1a.2a.3a:1234" },
#if !defined(CLARINET_ENABLE_IPV6)
            { 25, "[::ffff:127.0.0.1]:1234" },
            { 26, "[::ffff:127.0.0.1%1234567890]:1234" },
            { 27, "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf]:1234" },
            { 28, "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779]:1234" },
#endif
            { 29, "[::ffff:127.0.0.1]" },
            { 30, "[::ffff:127.0.0.1%1234567890]" },
            { 31, "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf]" },
            { 32, "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779]" },
            { 33, "[::ffff:127.0.0.1]:" },
            { 34, "[::ffff:127.0.0.1%1234567890]:" },
            { 35, "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf]:" },
            { 36, "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779]:" },
            { 37, "[::ffff:127.0.0.1:" },
            { 38, "[::ffff:127.0.0.1%1234567890:" },
            { 39, "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf" },
            { 40, "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779:" },
            { 41, "::ffff:127.0.0.1:1234" },
            { 42, "::ffff:127.0.0.1%1234567890:1234"  },
            { 43, "b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf:1234" },
            { 44, "b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779:1234" },
            { 45, ":" },
            { 46, "::%" },
            { 47, "::1%" },
            { 48, "::ffff::1" },
            { 49, "::ffff::1" },
            { 50, "::1%4294969999" },
            { 51, "::1%-100" },
            { 52, "::1%0x12345" },
            { 53, "b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%00003233923779" },
            { 54, "[:" },
            { 55, "[::%" },
            { 56, "[::1%" },
            { 57, "[::ffff::1" },
            { 58, "[::ffff::1" },
            { 59, "[::1%4294969999" },
            { 60, "[::1%-100" },
            { 61, "[::1%0x12345" },
            { 62, "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%00003233923779" },
            { 63, "[:]" },
            { 64, "[::%]" },
            { 65, "[::1%]" },
            { 66, "[::ffff::1]" },
            { 67, "[::ffff::1]" },
            { 68, "[::1%4294969999]" },
            { 69, "[::1%-100]" },
            { 70, "[::1%0x12345]" },
            { 71, "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%000012345]" },
            { 72, "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779]:09" },
            { 72, "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779]:99999" },
            { 72, "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779]:00999" },
            { 73, "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779]:12345FooBar" },
            { 74, "FooBar[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779]:12345" },
            { 75, "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779]:-12345" },
            { 76, "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779]:0012345" },
        };

        int index;
        const char* src;
        std::tie(index, src) = GENERATE_REF(from_range(data));

        FROM(index);

        clarinet_endpoint dst = {{0}};
        memnoise(&dst, sizeof(dst));
        const int errcode = clarinet_endpoint_from_string(&dst, src, strlen(src));
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
    }

    SECTION("Valid endpoints")
    {
#if defined(CLARINET_ENABLE_IPV6)
        clarinet_addr ipv4mapped_with_scope_id = clarinet_addr_ipv4mapped_loopback;
        ipv4mapped_with_scope_id.as.ipv6.scope_id = 1234567890;

        clarinet_addr ipv4mapped_with_short_scope_id = clarinet_addr_ipv4mapped_loopback;
        ipv4mapped_with_short_scope_id.as.ipv6.scope_id = 12345;
        
        clarinet_addr custom_ipv6 = {0};
        memnoise(&custom_ipv6, sizeof(custom_ipv6));
        custom_ipv6.family = CLARINET_AF_INET6;
        custom_ipv6.as.ipv6.flowinfo = 0;
        custom_ipv6.as.ipv6.u.byte[0] = 0xB0;
        custom_ipv6.as.ipv6.u.byte[1] = 0xB1;
        custom_ipv6.as.ipv6.u.byte[2] = 0xB2;
        custom_ipv6.as.ipv6.u.byte[3] = 0xB3;
        custom_ipv6.as.ipv6.u.byte[4] = 0xB4;
        custom_ipv6.as.ipv6.u.byte[5] = 0xB5;
        custom_ipv6.as.ipv6.u.byte[6] = 0xB6;
        custom_ipv6.as.ipv6.u.byte[7] = 0xB7;
        custom_ipv6.as.ipv6.u.byte[8] = 0xB8;
        custom_ipv6.as.ipv6.u.byte[9] = 0xB9;
        custom_ipv6.as.ipv6.u.byte[10] = 0xBA;
        custom_ipv6.as.ipv6.u.byte[11] = 0xBB;
        custom_ipv6.as.ipv6.u.byte[12] = 0xBC;
        custom_ipv6.as.ipv6.u.byte[13] = 0xBD;
        custom_ipv6.as.ipv6.u.byte[14] = 0xBE;
        custom_ipv6.as.ipv6.u.byte[15] = 0xBF;
        custom_ipv6.as.ipv6.scope_id = 3233923779;

        /* flowinfo is not compared */
        clarinet_addr custom_ipv6_with_flowinfo = custom_ipv6;
        custom_ipv6_with_flowinfo.as.ipv6.flowinfo = 0xFF;
#endif

        const std::vector<std::tuple<int, const char*, const char*, clarinet_endpoint>> data =
        {
            {  0, "0.0.0.0:0",                                                  "0.0.0.0:0",                                                  { clarinet_addr_ipv4_any,                0 } },
            {  1, "0.0.0.0:1234",                                               "0.0.0.0:1234",                                               { clarinet_addr_ipv4_any,             1234 } },
            {  2, "127.0.0.1:0",                                                "127.0.0.1:0",                                                { clarinet_addr_ipv4_loopback,           0 } },
            {  3, "127.0.0.1:65535",                                            "127.0.0.1:65535",                                            { clarinet_addr_ipv4_loopback,       65535 } },
            {  4, "255.255.255.255:9",                                          "255.255.255.255:9",                                          { clarinet_addr_ipv4_broadcast,          9 } },
#if defined(CLARINET_ENABLE_IPV6)                                                                                                            
            {  5, "[::]:0",                                                     "[::]:0",                                                     { clarinet_addr_ipv6_any,                0 } },
            {  6, "[::]:1234",                                                  "[::]:1234",                                                  { clarinet_addr_ipv6_any,             1234 } },
            {  7, "[::1]:0",                                                    "[::1]:0",                                                    { clarinet_addr_ipv6_loopback,           0 } },
            {  8, "[::1]:65535",                                                "[::1]:65535",                                                { clarinet_addr_ipv6_loopback,       65535 } },
            { 19, "[::ffff:127.0.0.1]:0",                                       "[::ffff:127.0.0.1]:0",                                       { clarinet_addr_ipv4mapped_loopback,     0 } },
            { 20, "[0000:0000:0000:0000:0000:ffff:127.0.0.1%0]:0",              "[::ffff:127.0.0.1]:0",                                       { clarinet_addr_ipv4mapped_loopback,     0 } },
            { 21, "[::ffff:127.0.0.1]:65535",                                   "[::ffff:127.0.0.1]:65535",                                   { clarinet_addr_ipv4mapped_loopback, 65535 } },
            { 22, "[::ffff:127.0.0.1%12345]:8",                                 "[::ffff:127.0.0.1%12345]:8",                                 { ipv4mapped_with_short_scope_id,        8 } },
            { 23, "[::ffff:127.0.0.1%1234567890]:65535",                        "[::ffff:127.0.0.1%1234567890]:65535",                        { ipv4mapped_with_scope_id,          65535 } },
            { 24, "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779]:0",     "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779]:0",     { custom_ipv6,                           0 } },
            { 25, "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779]:1",     "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779]:1",     { custom_ipv6,                           1 } },
            { 26, "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779]:65535", "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779]:65535", { custom_ipv6_with_flowinfo,         65535 } },
#endif
        };

        int index;
        const char* src;
        const char* reconstruction;
        clarinet_endpoint expected;
        std::tie(index, src, reconstruction, expected) = GENERATE_REF(from_range(data));

        FROM(index);

        clarinet_endpoint dst = {{0}};
        memnoise(&dst, sizeof(dst));
        const int errcode = clarinet_endpoint_from_string(&dst, src, strlen(src));
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        REQUIRE(clarinet_endpoint_is_equal(&dst, &expected));

        char s[CLARINET_ENDPOINT_STRLEN] = {0};
        memnoise(&s, sizeof(s));
        const int n = clarinet_endpoint_to_string(s, sizeof(s), &dst);
        REQUIRE(n > 0);
        EXPLAIN("Expected: %s", reconstruction);
        EXPLAIN("Actual: %s", s);       
        REQUIRE((size_t)n == strlen(reconstruction));
        REQUIRE_THAT(std::string(s), Equals(reconstruction));
    }
}
