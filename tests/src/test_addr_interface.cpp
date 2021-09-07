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
    clarinet_addr actual = CLARINET_ADDR_IPV4_ANY;
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
        clarinet_addr expected = CLARINET_ADDR_NONE;
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
    clarinet_addr actual = CLARINET_ADDR_IPV4_LOOPBACK;
    REQUIRE(actual.family == CLARINET_AF_INET);
    REQUIRE(actual.as.ipv4.u.byte[0] == 127);
    REQUIRE(actual.as.ipv4.u.byte[1] == 0);
    REQUIRE(actual.as.ipv4.u.byte[2] == 0);
    REQUIRE(actual.as.ipv4.u.byte[3] == 1);

    SECTION("Check constant CLARINET_ADDR_IPV4_LOOPBACK")
    {
        clarinet_addr expected = CLARINET_ADDR_NONE;
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
    clarinet_addr actual = CLARINET_ADDR_IPV4_BROADCAST;
    REQUIRE(actual.family == CLARINET_AF_INET);
    REQUIRE(actual.as.ipv4.u.byte[0] == 255);
    REQUIRE(actual.as.ipv4.u.byte[1] == 255);
    REQUIRE(actual.as.ipv4.u.byte[2] == 255);
    REQUIRE(actual.as.ipv4.u.byte[3] == 255);

    SECTION("Check constant CLARINET_ADDR_IPV4_BROADCAST")
    {
        clarinet_addr expected = CLARINET_ADDR_NONE;
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
    clarinet_addr actual = CLARINET_ADDR_IPV4MAPPED_LOOPBACK;
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

    SECTION("Check constant CLARINET_ADDR_IPV4MAPPED_LOOPBACK")
    {
        clarinet_addr expected = CLARINET_ADDR_NONE;
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
    clarinet_addr actual = CLARINET_ADDR_IPV6_ANY;
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

    clarinet_addr expected = CLARINET_ADDR_NONE;
    expected.family = CLARINET_AF_INET6;

    REQUIRE_THAT(memory(&expected, sizeof(expected)), Equals(memory(&actual, sizeof(actual))));

    REQUIRE(clarinet_addr_is_ipv6(&actual));
    REQUIRE(clarinet_addr_is_ipv6_any(&actual));
    REQUIRE(clarinet_addr_is_any(&actual));
}

TEST_CASE("IPv6 Address Loopback", "[address][ipv6][loopback]")
{
    clarinet_addr actual = CLARINET_ADDR_IPV6_LOOPBACK;
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

    SECTION("Check CLARINET_ADDR_IPV6_LOOPBACK constant")
    {
        clarinet_addr expected = CLARINET_ADDR_NONE;
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
                { 0, CLARINET_ADDR_NONE, CLARINET_ADDR_NONE }
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
                { 0, CLARINET_ADDR_NONE, CLARINET_ADDR_IPV4_ANY },
                { 1, CLARINET_ADDR_NONE, CLARINET_ADDR_IPV4_LOOPBACK },
                { 2, CLARINET_ADDR_NONE, CLARINET_ADDR_IPV4_BROADCAST }
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
                { 0, CLARINET_ADDR_IPV4_ANY, CLARINET_ADDR_IPV4_ANY },
                { 1, CLARINET_ADDR_IPV4_LOOPBACK, CLARINET_ADDR_IPV4_LOOPBACK },
                { 2, CLARINET_ADDR_IPV4_BROADCAST, CLARINET_ADDR_IPV4_BROADCAST },
            }));

        FROM(index);

        REQUIRE(clarinet_addr_is_equal(&a, &b));
    }

    SECTION("Constant ipv4 addresses (Not Equal)")
    {
        int index;
        clarinet_addr a, b;
        std::tie(index, a, b) = GENERATE(table<int, clarinet_addr, clarinet_addr>({
                { 0, CLARINET_ADDR_IPV4_ANY, CLARINET_ADDR_IPV4_LOOPBACK },
                { 1, CLARINET_ADDR_IPV4_ANY, CLARINET_ADDR_IPV4_BROADCAST },
                { 2, CLARINET_ADDR_IPV4_LOOPBACK, CLARINET_ADDR_IPV4_BROADCAST }
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
                { 0, CLARINET_ADDR_IPV6_ANY, CLARINET_ADDR_IPV6_ANY },
                { 1, CLARINET_ADDR_IPV6_LOOPBACK, CLARINET_ADDR_IPV6_LOOPBACK },
                { 2, CLARINET_ADDR_IPV4MAPPED_LOOPBACK, CLARINET_ADDR_IPV4MAPPED_LOOPBACK}
            }));

        FROM(index);

        REQUIRE(clarinet_addr_is_equal(&a, &b));
    }

    SECTION("Constant ipv6 addresses (Not Equal)")
    {
        int index;
        clarinet_addr a, b;
        std::tie(index, a, b) = GENERATE(table<int, clarinet_addr, clarinet_addr>({
                { 0, CLARINET_ADDR_IPV6_ANY, CLARINET_ADDR_IPV4_ANY },
                { 1, CLARINET_ADDR_IPV6_LOOPBACK, CLARINET_ADDR_IPV4_LOOPBACK },
                { 2, CLARINET_ADDR_IPV6_ANY, CLARINET_ADDR_IPV6_LOOPBACK },
                { 3, CLARINET_ADDR_IPV6_ANY, CLARINET_ADDR_IPV4MAPPED_LOOPBACK },
                { 4, CLARINET_ADDR_IPV6_LOOPBACK, CLARINET_ADDR_IPV4MAPPED_LOOPBACK }
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
                { 0, CLARINET_ADDR_IPV4_ANY, CLARINET_ADDR_IPV4_ANY },
                { 1, CLARINET_ADDR_IPV4_LOOPBACK, CLARINET_ADDR_IPV4_LOOPBACK },
                { 2, CLARINET_ADDR_IPV4_BROADCAST, CLARINET_ADDR_IPV4_BROADCAST }
            }));

        FROM(index);

        REQUIRE(clarinet_addr_is_equivalent(&a, &b));
    }

    SECTION("Constant ipv4 addresses (Not Equivalent)")
    {
        int index;
        clarinet_addr a, b;
        std::tie(index, a, b) = GENERATE(table<int, clarinet_addr, clarinet_addr>({
                { 0, CLARINET_ADDR_IPV4_ANY, CLARINET_ADDR_IPV4_LOOPBACK },
                { 1, CLARINET_ADDR_IPV4_ANY, CLARINET_ADDR_IPV4_BROADCAST },
                { 2, CLARINET_ADDR_IPV4_LOOPBACK, CLARINET_ADDR_IPV4_BROADCAST }
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
                { 0, CLARINET_ADDR_IPV6_ANY, CLARINET_ADDR_IPV6_ANY },
                { 1, CLARINET_ADDR_IPV6_LOOPBACK, CLARINET_ADDR_IPV6_LOOPBACK  },
                { 2, CLARINET_ADDR_IPV4MAPPED_LOOPBACK, CLARINET_ADDR_IPV4MAPPED_LOOPBACK  },
                { 3, CLARINET_ADDR_IPV4_LOOPBACK, CLARINET_ADDR_IPV4MAPPED_LOOPBACK  }
            }));

        FROM(index);

        REQUIRE(clarinet_addr_is_equivalent(&a, &b));
    }

    SECTION("Constant ipv6 addresses (Not Equivalent)")
    {
        int index;
        clarinet_addr a, b;
        std::tie(index, a, b) = GENERATE(table<int, clarinet_addr, clarinet_addr>({
                { 0, CLARINET_ADDR_IPV6_ANY, CLARINET_ADDR_IPV6_LOOPBACK },
                { 1, CLARINET_ADDR_IPV6_ANY, CLARINET_ADDR_IPV4MAPPED_LOOPBACK },
                { 2, CLARINET_ADDR_IPV6_LOOPBACK, CLARINET_ADDR_IPV4MAPPED_LOOPBACK }
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
            { 0, CLARINET_ADDR_IPV4MAPPED_LOOPBACK, CLARINET_ADDR_IPV4_LOOPBACK },
#endif
            { 1, CLARINET_ADDR_IPV4_LOOPBACK, CLARINET_ADDR_IPV4_LOOPBACK }
        };

        int index;
        clarinet_addr src, expected;
        std::tie(index, src, expected) = GENERATE_REF(from_range(data));

        FROM(index);

        clarinet_addr dst;
        const int errcode = clarinet_addr_map_to_ipv4(&dst, &src);
        REQUIRE(errcode == CLARINET_ENONE);
        REQUIRE(clarinet_addr_is_equal(&dst, &expected));
    }

    SECTION("Invalid map To ipv4")
    {
        const std::vector<std::tuple<int, clarinet_addr>> data =
        {
#if !defined(CLARINET_ENABLE_IPV6)
            { 0, CLARINET_ADDR_IPV6_LOOPBACK },
            { 1, CLARINET_ADDR_IPV4MAPPED_LOOPBACK },
#endif
            { 2, CLARINET_ADDR_NONE }
        };

        int index;
        clarinet_addr src;
        std::tie(index, src) = GENERATE_REF(from_range(data));
        
        FROM(index);

        clarinet_addr dst;
        const int errcode = clarinet_addr_map_to_ipv4(&dst, &src);
        REQUIRE(errcode == CLARINET_EINVAL);
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
                { 0, CLARINET_ADDR_IPV4_LOOPBACK, CLARINET_ADDR_IPV4MAPPED_LOOPBACK },
                { 1, CLARINET_ADDR_IPV6_LOOPBACK, CLARINET_ADDR_IPV6_LOOPBACK }
            }));
        
        FROM(index);

        clarinet_addr dst;
        const int errcode = clarinet_addr_map_to_ipv6(&dst, &src);
        REQUIRE(errcode == CLARINET_ENONE);
        REQUIRE(clarinet_addr_is_equal(&dst, &expected));
#endif
    }

    SECTION("Invalid map To ipv6")
    {
        const std::vector<std::tuple<int, clarinet_addr>> data =
        {
#if !defined(CLARINET_ENABLE_IPV6)
            {1, CLARINET_ADDR_IPV4MAPPED_LOOPBACK },
            {2, CLARINET_ADDR_IPV6_LOOPBACK },
#endif
            {3, CLARINET_ADDR_NONE }
        };

        int index;
        clarinet_addr src;
        std::tie(index, src) = GENERATE_REF(from_range(data));

        FROM(index);

        clarinet_addr dst;
        const int errcode = clarinet_addr_map_to_ipv6(&dst, &src);
        REQUIRE_FALSE(errcode == CLARINET_ENONE);
    }
}

TEST_CASE("Address To String", "[address]")
{
    SECTION("With invalid arguments")
    {
        SECTION("NULL dst")
        {
            clarinet_addr src = CLARINET_ADDR_IPV4_ANY;
            const int errcode = clarinet_addr_to_string(NULL, 0, &src);
            REQUIRE(errcode == CLARINET_EINVAL);
        }

        SECTION("NULL src")
        {
            char dst[CLARINET_ADDR_STRLEN];
            const int errcode = clarinet_addr_to_string(dst, sizeof(dst), NULL);
            REQUIRE(errcode == CLARINET_EINVAL);
        }

        SECTION("Not enough space on dst for ipv4")
        {
            clarinet_addr src = CLARINET_ADDR_IPV4_ANY;
            char dst[4];
            const int errcode = clarinet_addr_to_string(dst, sizeof(dst), &src);
            REQUIRE(errcode == CLARINET_EINVAL);
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
            REQUIRE(errcode == CLARINET_EINVAL);
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
            REQUIRE(errcode == CLARINET_EINVAL);
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
                    { 0, CLARINET_ADDR_IPV4_ANY, "0.0.0.0" },
                    { 1, CLARINET_ADDR_IPV4_LOOPBACK, "127.0.0.1" },
                    { 2, CLARINET_ADDR_IPV4_BROADCAST, "255.255.255.255" }
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
            clarinet_addr ipv4mapped_with_scope_id = CLARINET_ADDR_IPV4MAPPED_LOOPBACK;
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
                    {0, CLARINET_ADDR_IPV6_ANY, "::" },
                    {1, CLARINET_ADDR_IPV6_LOOPBACK, "::1" },
                    {2, CLARINET_ADDR_IPV4MAPPED_LOOPBACK, "::ffff:127.0.0.1" },
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
        REQUIRE(errcode == CLARINET_EINVAL);
    }

    SECTION("NULL src")
    {
        clarinet_addr dst = {0};
        const int errcode = clarinet_addr_from_string(&dst, NULL, 0);
        REQUIRE(errcode == CLARINET_EINVAL);
    }

    SECTION("Zero srclen")
    {
        clarinet_addr dst = {0};
        const char* src = "192.168.0.1";
        const int errcode = clarinet_addr_from_string(&dst, src, 0);
        REQUIRE(errcode == CLARINET_EINVAL);
    }

    SECTION("Insufficient srclen")
    {
        clarinet_addr dst = {0};
        const char* src = "192.168.0.1";
        const int errcode = clarinet_addr_from_string(&dst, src, strlen(src) >> 1);
        REQUIRE(errcode == CLARINET_EINVAL);
    }

    SECTION("Invalid addresses")
    {
        const std::vector<std::tuple<int, const char*>> data =
        {
            { 0, "" },
            { 1, "0" },
            { 2, "0." },
            { 3, "0.0" },
            { 4, "0.0.0" },
            { 5, "127.0.0.a" },
            { 6, "255.255.255.255a" },
            { 7, "255.255.255.ff" },
            { 8, "10.0.0.-1" },
            { 9, "ff.ff.ff.ff" },
            { 10, "a0.a1.a2.a3" },
            { 11, "0a.1a.2a.3a" },
#if !defined(CLARINET_ENABLE_IPV6)
            { 12, "::ffff:127.0.0.1" }, 
            { 13, "::ffff:127.0.0.1%1234567890" },
            { 15, "b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf" },
            { 15, "b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779" },
#endif
            { 16, ":" },
            { 17, "::%" },
            { 18, "::1%" },
            { 19, "::ffff::1" },
            { 20, "::ffff::1" },
            { 21, "::1%4294969999" },
            { 22, "::1%-100" },
            { 23, "::1%0x12345" },
            { 24, "b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%00003233923779" },
        };

        int index;
        const char* src;
        std::tie(index, src) = GENERATE_REF(from_range(data));

        FROM(index);

        clarinet_addr dst = {0};
        memnoise(&dst, sizeof(dst));
        const int errcode = clarinet_addr_from_string(&dst, src, strlen(src));
        REQUIRE(errcode == CLARINET_EINVAL);
    }

    SECTION("Valid addresses")
    {
#if defined(CLARINET_ENABLE_IPV6)
        clarinet_addr ipv4mapped_with_scope_id = CLARINET_ADDR_IPV4MAPPED_LOOPBACK;
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
#endif


        const std::vector<std::tuple<int, const char*, clarinet_addr>> data =
        {
            { 0, "0.0.0.0", CLARINET_ADDR_IPV4_ANY },
            { 1, "127.0.0.1", CLARINET_ADDR_IPV4_LOOPBACK },
            { 2, "255.255.255.255", CLARINET_ADDR_IPV4_BROADCAST },
#if defined(CLARINET_ENABLE_IPV6)
            { 3, "::", CLARINET_ADDR_IPV6_ANY },
            { 4, "::1", CLARINET_ADDR_IPV6_LOOPBACK },
            { 5, "::ffff:127.0.0.1", CLARINET_ADDR_IPV4MAPPED_LOOPBACK },
            { 6, "::ffff:127.0.0.1%1234567890", ipv4mapped_with_scope_id },
            { 7, "b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779", custom_ipv6 },
            { 8, "b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779", custom_ipv6_with_flowinfo }
#endif
        };

        int index;
        const char* src;
        clarinet_addr expected;
        std::tie(index, src, expected) = GENERATE_REF(from_range(data));

        FROM(index);

        clarinet_addr dst = {0};
        memnoise(&dst, sizeof(dst));
        const int errcode = clarinet_addr_from_string(&dst, src, strlen(src));
        REQUIRE(errcode == CLARINET_ENONE);
        REQUIRE(clarinet_addr_is_equal(&dst, &expected));
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
    REQUIRE(CLARINET_ENDPOINT_STRLEN == 65); /* "[0000:0000:0000:0000:0000:ffff:255.255.255.255%4294967296]:65535" */
}

TEST_CASE("Endpoint To String", "[endpoint]")
{
    SECTION("With invalid arguments")
    {
        SECTION("NULL dst")
        {
            clarinet_endpoint src = {CLARINET_ADDR_IPV4_ANY, 0};
            const int errcode = clarinet_endpoint_to_string(NULL, 0, &src);
            REQUIRE(errcode == CLARINET_EINVAL);
        }

        SECTION("NULL src")
        {
            char dst[CLARINET_ENDPOINT_STRLEN];
            const int errcode = clarinet_endpoint_to_string(dst, sizeof(dst), NULL);
            REQUIRE(errcode == CLARINET_EINVAL);
        }

        SECTION("Not enough space on dst for ipv4")
        {
            clarinet_endpoint src = {CLARINET_ADDR_IPV4_ANY, 0};
            char dst[4];
            const int errcode = clarinet_endpoint_to_string(dst, sizeof(dst), &src);
            REQUIRE(errcode == CLARINET_EINVAL);
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
            REQUIRE(errcode == CLARINET_EINVAL);
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
            REQUIRE(errcode == CLARINET_EINVAL);
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
                    { 0, { CLARINET_ADDR_IPV4_ANY, 0 }, "0.0.0.0:0" },
                    { 1, { CLARINET_ADDR_IPV4_LOOPBACK, 0 }, "127.0.0.1:0" },
                    { 2, { CLARINET_ADDR_IPV4_BROADCAST, 0 }, "255.255.255.255:0" },
                    { 3, { CLARINET_ADDR_IPV4_ANY, 65535 }, "0.0.0.0:65535" },
                    { 4, { CLARINET_ADDR_IPV4_LOOPBACK, 65535 }, "127.0.0.1:65535" },
                    { 5, { CLARINET_ADDR_IPV4_BROADCAST, 65535 }, "255.255.255.255:65535" }
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
            clarinet_addr ipv4mapped_with_scope_id = CLARINET_ADDR_IPV4MAPPED_LOOPBACK;
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
                    {0, { CLARINET_ADDR_IPV6_ANY,                   0 }, "[::]:0" },
                    {1, { CLARINET_ADDR_IPV6_LOOPBACK,              0 }, "[::1]:0" },
                    {2, { CLARINET_ADDR_IPV4MAPPED_LOOPBACK,        0 }, "[::ffff:127.0.0.1]:0" },
                    {3, { ipv4mapped_with_scope_id,                 0 }, "[::ffff:127.0.0.1%1234567890]:0" },
                    {4, { custom_ipv6,                              0 }, "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779]:0" },
                    {5, { CLARINET_ADDR_IPV6_ANY,               65535 }, "[::]:65535" },
                    {6, { CLARINET_ADDR_IPV6_LOOPBACK,          65535 }, "[::1]:65535" },
                    {7, { CLARINET_ADDR_IPV4MAPPED_LOOPBACK,    65535 }, "[::ffff:127.0.0.1]:65535" },
                    {8, { ipv4mapped_with_scope_id,             65535 }, "[::ffff:127.0.0.1%1234567890]:65535" },
                    {9, { custom_ipv6,                          65535 }, "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779]:65535" }
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
        REQUIRE(errcode == CLARINET_EINVAL);
    }

    SECTION("NULL src")
    {
        clarinet_endpoint dst = {{0}};
        const int errcode = clarinet_endpoint_from_string(&dst, NULL, 0);
        REQUIRE(errcode == CLARINET_EINVAL);
    }

    SECTION("Zero srclen")
    {
        clarinet_endpoint dst = {{0}};
        const char* src = "192.168.0.1:0";
        const int errcode = clarinet_endpoint_from_string(&dst, src, 0);
        REQUIRE(errcode == CLARINET_EINVAL);
    }

    SECTION("Insufficient srclen")
    {
        clarinet_endpoint dst = {{0}};
        const char* src = "192.168.0.1:0";
        const int errcode = clarinet_endpoint_from_string(&dst, src, strlen(src) >> 1);
        REQUIRE(errcode == CLARINET_EINVAL);
    }

    SECTION("Invalid endpoints")
    {
        const std::vector<std::tuple<int, const char*>> data =
        {
            { 0, "" },
            { 1, "0" },
            { 2, "0." },
            { 3, "0.0" },
            { 4, "0.0.0" },
            { 5, "0.0.0.0" },
            { 6, "0.0.0.0:" },
            { 7, "0.0.0.0::13" },
            { 8, "127.0.0.1" },
            { 9, "127.0.0.1:" },
            { 10, "127.0.0.1::9" },
            { 11, "127.0.0.1:0x1234" },
            { 12, "127.0.0.1:99999" },
            { 13, "127.0.0.1:12345FooBar" },
            { 14, "FooBar127.0.0.1:12345" },
            { 15, "127.0.0.a" },
            { 16, "255.255.255.255" },
            { 17, "255.255.255.255a:1234" },
            { 18, "255.255.255.ff:1234" },
            { 19, "10.0.0.-1:1234" },
            { 20, "ff.ff.ff.ff:1234" },
            { 21, "a0.a1.a2.a3:1234" },
            { 22, "0a.1a.2a.3a:1234" },
#if !defined(CLARINET_ENABLE_IPV6)
            { 23, "[::ffff:127.0.0.1]:1234" },
            { 24, "[::ffff:127.0.0.1%1234567890]:1234" },
            { 25, "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf]:1234" },
            { 26, "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779]:1234" },
#endif
            { 27, "[::ffff:127.0.0.1]" },
            { 28, "[::ffff:127.0.0.1%1234567890]" },
            { 29, "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf]" },
            { 30, "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779]" },
            { 31, "[::ffff:127.0.0.1]:" },
            { 32, "[::ffff:127.0.0.1%1234567890]:" },
            { 33, "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf]:" },
            { 34, "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779]:" },
            { 35, "[::ffff:127.0.0.1:" },
            { 36, "[::ffff:127.0.0.1%1234567890:" },
            { 37, "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf" },
            { 38, "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779:" },
            { 39, "::ffff:127.0.0.1:1234" },
            { 40, "::ffff:127.0.0.1%1234567890:1234"  },
            { 41, "b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf:1234" },
            { 42, "b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779:1234" },
            { 43, ":" },
            { 44, "::%" },
            { 45, "::1%" },
            { 46, "::ffff::1" },
            { 47, "::ffff::1" },
            { 48, "::1%4294969999" },
            { 49, "::1%-100" },
            { 50, "::1%0x12345" },
            { 51, "b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%00003233923779" },
            { 52, "[:" },
            { 53, "[::%" },
            { 54, "[::1%" },
            { 55, "[::ffff::1" },
            { 56, "[::ffff::1" },
            { 57, "[::1%4294969999" },
            { 58, "[::1%-100" },
            { 59, "[::1%0x12345" },
            { 60, "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%00003233923779" },
            { 61, "[:]" },
            { 62, "[::%]" },
            { 63, "[::1%]" },
            { 64, "[::ffff::1]" },
            { 65, "[::ffff::1]" },
            { 66, "[::1%4294969999]" },
            { 67, "[::1%-100]" },
            { 68, "[::1%0x12345]" },
            { 69, "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%00003233923779]" },
            { 70, "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779]:99999" },
            { 71, "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779]:12345FooBar" },
            { 72, "FooBar[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779]:12345" },
            { 73, "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779]:-12345" },
            { 74, "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779]:0012345" },
        };

        int index;
        const char* src;
        std::tie(index, src) = GENERATE_REF(from_range(data));

        FROM(index);

        clarinet_endpoint dst = {{0}};
        memnoise(&dst, sizeof(dst));
        const int errcode = clarinet_endpoint_from_string(&dst, src, strlen(src));
        REQUIRE(errcode == CLARINET_EINVAL);
    }

    SECTION("Valid endpoints")
    {
#if defined(CLARINET_ENABLE_IPV6)
        clarinet_addr ipv4mapped_with_scope_id = CLARINET_ADDR_IPV4MAPPED_LOOPBACK;
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
#endif


        const std::vector<std::tuple<int, const char*, clarinet_endpoint>> data =
        {
            { 0, "0.0.0.0:0",                                                    { CLARINET_ADDR_IPV4_ANY,                0 } },
            { 1, "0.0.0.0:1234",                                                 { CLARINET_ADDR_IPV4_ANY,             1234 } },
            { 2, "127.0.0.1:0",                                                  { CLARINET_ADDR_IPV4_LOOPBACK,           0 } },
            { 3, "127.0.0.1:65535",                                              { CLARINET_ADDR_IPV4_LOOPBACK,       65535 } },
            { 4, "255.255.255.255:9",                                            { CLARINET_ADDR_IPV4_BROADCAST,          9 } },
            { 5, "255.255.255.255:09",                                           { CLARINET_ADDR_IPV4_BROADCAST,          9 } },
            { 6, "255.255.255.255:00009",                                        { CLARINET_ADDR_IPV4_BROADCAST,          9 } },
#if defined(CLARINET_ENABLE_IPV6)                                                                                     
            { 7, "[::]:0",                                                       { CLARINET_ADDR_IPV6_ANY,                0 } },
            { 8, "[::]:1234",                                                    { CLARINET_ADDR_IPV6_ANY,             1234 } },
            { 9, "[::1]:0",                                                      { CLARINET_ADDR_IPV6_LOOPBACK,           0 } },
            { 10, "[::1]:65535",                                                 { CLARINET_ADDR_IPV6_LOOPBACK,       65535 } },
            { 11, "[::ffff:127.0.0.1]:0",                                        { CLARINET_ADDR_IPV4MAPPED_LOOPBACK,     0 } },
            { 12, "[::ffff:127.0.0.1]:65535",                                    { CLARINET_ADDR_IPV4MAPPED_LOOPBACK, 65535 } },
            { 13, "[::ffff:127.0.0.1%1234567890]:0",                             { ipv4mapped_with_scope_id,              0 } },
            { 14, "[::ffff:127.0.0.1%1234567890]:65535",                         { ipv4mapped_with_scope_id,          65535 } },
            { 15, "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779]:0",      { custom_ipv6,                           0 } },
            { 16, "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779]:00",     { custom_ipv6,                           0 } },
            { 17, "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779]:000",    { custom_ipv6,                           0 } },
            { 18, "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779]:0000",   { custom_ipv6,                           0 } },
            { 19, "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779]:00000",  { custom_ipv6,                           0 } },
            { 20, "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779]:00001",  { custom_ipv6,                           1 } },
            { 21, "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779]:65535",  { custom_ipv6_with_flowinfo,         65535 } },
#endif
        };

        int index;
        const char* src;
        clarinet_endpoint expected;
        std::tie(index, src, expected) = GENERATE_REF(from_range(data));

        FROM(index);

        clarinet_endpoint dst = {{0}};
        memnoise(&dst, sizeof(dst));
        const int errcode = clarinet_endpoint_from_string(&dst, src, strlen(src));
        REQUIRE(errcode == CLARINET_ENONE);
        REQUIRE(clarinet_endpoint_is_equal(&dst, &expected));
    }
}
