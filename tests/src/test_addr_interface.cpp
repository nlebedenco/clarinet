#include "test.h"

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

TEST_CASE("Address String Length", "[address]")
{
    REQUIRE(CLARINET_ADDR_STRLEN >= 57); /* "0000:0000:0000:0000:0000:ffff:255.255.255.255%4294967295" */
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
        REQUIRE_THAT(Memory(&expected, sizeof(expected)), Equals(Memory(&actual, sizeof(actual))));
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

        REQUIRE_THAT(Memory(&expected, sizeof(expected)), Equals(Memory(&actual, sizeof(actual))));
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

        REQUIRE_THAT(Memory(&expected, sizeof(expected)), Equals(Memory(&actual, sizeof(actual))));
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

        REQUIRE_THAT(Memory(&expected, sizeof(expected)), Equals(Memory(&actual, sizeof(actual))));
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

    REQUIRE_THAT(Memory(&expected, sizeof(expected)), Equals(Memory(&actual, sizeof(actual))));

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

        REQUIRE_THAT(Memory(&expected, sizeof(expected)), Equals(Memory(&actual, sizeof(clarinet_addr))));
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
        const auto& p = GENERATE(
            std::make_pair(CLARINET_ADDR_NONE, CLARINET_ADDR_NONE));

        REQUIRE(clarinet_addr_is_equal(&p.first, &p.second));
    }

    SECTION("Address None (Not Equal)")
    {
        /* using two distinct allocations for the same contant on purpose */
        const auto& p = GENERATE(
            std::make_pair(CLARINET_ADDR_NONE, CLARINET_ADDR_IPV4_ANY),
            std::make_pair(CLARINET_ADDR_NONE, CLARINET_ADDR_IPV4_LOOPBACK),
            std::make_pair(CLARINET_ADDR_NONE, CLARINET_ADDR_IPV4_BROADCAST));

        REQUIRE_FALSE(clarinet_addr_is_equal(&p.first, &p.second));
    }

    SECTION("Constant ipv4 addresses (Equal)")
    {
        /* using two distinct allocations for the same contant on purpose */
        const auto& p = GENERATE(
            std::make_pair(CLARINET_ADDR_IPV4_ANY, CLARINET_ADDR_IPV4_ANY),
            std::make_pair(CLARINET_ADDR_IPV4_LOOPBACK, CLARINET_ADDR_IPV4_LOOPBACK),
            std::make_pair(CLARINET_ADDR_IPV4_BROADCAST, CLARINET_ADDR_IPV4_BROADCAST));

        REQUIRE(clarinet_addr_is_equal(&p.first, &p.second));
    }

    SECTION("Constant ipv4 addresses (Not Equal)")
    {
        const auto& p = GENERATE(
                std::make_pair(CLARINET_ADDR_IPV4_ANY, CLARINET_ADDR_IPV4_LOOPBACK),
                std::make_pair(CLARINET_ADDR_IPV4_ANY, CLARINET_ADDR_IPV4_BROADCAST),
                std::make_pair(CLARINET_ADDR_IPV4_LOOPBACK, CLARINET_ADDR_IPV4_BROADCAST));

        REQUIRE_FALSE(clarinet_addr_is_equal(&p.first, &p.second));
    }


    SECTION("Constant ipv6 addresses (Equal)")
    {
        /* using two distinct allocations for the same contant on purpose */
        const auto& p = GENERATE(
            std::make_pair(CLARINET_ADDR_IPV6_ANY, CLARINET_ADDR_IPV6_ANY),
            std::make_pair(CLARINET_ADDR_IPV6_LOOPBACK, CLARINET_ADDR_IPV6_LOOPBACK),
            std::make_pair(CLARINET_ADDR_IPV4MAPPED_LOOPBACK, CLARINET_ADDR_IPV4MAPPED_LOOPBACK));

        REQUIRE(clarinet_addr_is_equal(&p.first, &p.second));
    }

    SECTION("Constant ipv6 addresses (Not Equal)")
    {
        const auto& p = GENERATE(
            std::make_pair(CLARINET_ADDR_IPV6_ANY, CLARINET_ADDR_IPV4_ANY),
            std::make_pair(CLARINET_ADDR_IPV6_LOOPBACK, CLARINET_ADDR_IPV4_LOOPBACK),
            std::make_pair(CLARINET_ADDR_IPV6_ANY, CLARINET_ADDR_IPV6_LOOPBACK),
            std::make_pair(CLARINET_ADDR_IPV6_ANY, CLARINET_ADDR_IPV4MAPPED_LOOPBACK),
            std::make_pair(CLARINET_ADDR_IPV6_LOOPBACK, CLARINET_ADDR_IPV4MAPPED_LOOPBACK));

        REQUIRE_FALSE(clarinet_addr_is_equal(&p.first, &p.second));
    }

    SECTION("Custom ipv4 addresses (Equal)")
    {
        clarinet_addr a;
        memset(&a, 0xAA, sizeof(a)); /* memory noise */
        a.family = CLARINET_AF_INET;
        a.as.ipv4.u.byte[0] = 0x00;
        a.as.ipv4.u.byte[1] = 0x11;
        a.as.ipv4.u.byte[2] = 0x22;
        a.as.ipv4.u.byte[3] = 0x33;

        clarinet_addr b;
        memset(&b, 0xBB, sizeof(b)); /* memory noise */
        b.family = CLARINET_AF_INET;
        b.as.ipv4.u.byte[0] = 0x00;
        b.as.ipv4.u.byte[1] = 0x11;
        b.as.ipv4.u.byte[2] = 0x22;
        b.as.ipv4.u.byte[3] = 0x33;

        REQUIRE(clarinet_addr_is_equal(&a, &b));
    }

    SECTION("Custom ipv4 addresses (Not Equal)")
    {
        SECTION("Same ipv4 address but one has family NONE")
        {
            clarinet_addr a;
            memset(&a, 0xAA, sizeof(a)); /* memory noise */
            a.family = CLARINET_AF_NONE;
            a.as.ipv4.u.byte[0] = 0x00;
            a.as.ipv4.u.byte[1] = 0x11;
            a.as.ipv4.u.byte[2] = 0x22;
            a.as.ipv4.u.byte[3] = 0x33;

            clarinet_addr b;
            memset(&b, 0xAA, sizeof(b)); /* memory noise */
            b.family = CLARINET_AF_INET;
            b.as.ipv4.u.byte[0] = 0x00;
            b.as.ipv4.u.byte[1] = 0x11;
            b.as.ipv4.u.byte[2] = 0x22;
            b.as.ipv4.u.byte[3] = 0x33;

            REQUIRE_FALSE(clarinet_addr_is_equal(&a, &b));
        }
    }

    SECTION("Custom ipv6 addresses (Equal)")
    {
        clarinet_addr a;
        memset(&a, 0xAA, sizeof(a)); /* memory noise */
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
        memset(&b, 0xBB, sizeof(b)); /* memory noise */
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
    }

    SECTION("Custom ipv6 addresses (Not Equal)")
    {
        SECTION("Same ipv6 address but one has family NONE")
        {
            clarinet_addr a;
            memset(&a, 0xAA, sizeof(a)); /* memory noise */
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
            memset(&b, 0xBB, sizeof(b)); /* memory noise */
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
            memset(&a, 0xAA, sizeof(a)); /* memory noise */
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
            memset(&b, 0xBB, sizeof(b)); /* memory noise */
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
            memset(&a, 0xAA, sizeof(a)); /* memory noise */
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
            memset(&b, 0xBB, sizeof(b)); /* memory noise */
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

            REQUIRE_FALSE(clarinet_addr_is_equal(&a, &b));
        }

        SECTION("Same ipv6 address but different scope id")
        {
            clarinet_addr a;
            memset(&a, 0xAA, sizeof(a)); /* memory noise */
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
            memset(&b, 0xBB, sizeof(b)); /* memory noise */
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
            memset(&a, 0xAA, sizeof(a)); /* memory noise */
            a.family = CLARINET_AF_INET;
            a.as.ipv4.u.byte[0] = 0x00;
            a.as.ipv4.u.byte[1] = 0x11;
            a.as.ipv4.u.byte[2] = 0x22;
            a.as.ipv4.u.byte[3] = 0x33;

            clarinet_addr b;
            memset(&b, 0xBB, sizeof(b)); /* memory noise */
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

TEST_CASE("Address Is Equivalent", "[address]")
{
    SECTION("Constant ipv4 addresses (Equivalent)")
    {
        /* using two distinct allocations for the same contant on purpose */
        const auto& p = GENERATE(
            std::make_pair(CLARINET_ADDR_IPV4_ANY, CLARINET_ADDR_IPV4_ANY),
            std::make_pair(CLARINET_ADDR_IPV4_LOOPBACK, CLARINET_ADDR_IPV4_LOOPBACK),
            std::make_pair(CLARINET_ADDR_IPV4_BROADCAST, CLARINET_ADDR_IPV4_BROADCAST));

        REQUIRE(clarinet_addr_is_equivalent(&p.first, &p.second));
    }

    SECTION("Constant ipv4 addresses (Not Equivalent)")
    {
        const auto& p = GENERATE(
            std::make_pair(CLARINET_ADDR_IPV4_ANY, CLARINET_ADDR_IPV4_LOOPBACK),
            std::make_pair(CLARINET_ADDR_IPV4_ANY, CLARINET_ADDR_IPV4_BROADCAST),
            std::make_pair(CLARINET_ADDR_IPV4_LOOPBACK, CLARINET_ADDR_IPV4_BROADCAST));

        REQUIRE_FALSE(clarinet_addr_is_equivalent(&p.first, &p.second));
    }


    SECTION("Constant ipv6 addresses (Equivalent)")
    {
        /* using two distinct allocations for the same contant on purpose */
        const auto& p = GENERATE(
            std::make_pair(CLARINET_ADDR_IPV6_ANY, CLARINET_ADDR_IPV6_ANY),
            std::make_pair(CLARINET_ADDR_IPV6_LOOPBACK, CLARINET_ADDR_IPV6_LOOPBACK),
            std::make_pair(CLARINET_ADDR_IPV4MAPPED_LOOPBACK, CLARINET_ADDR_IPV4MAPPED_LOOPBACK),
            std::make_pair(CLARINET_ADDR_IPV4_LOOPBACK, CLARINET_ADDR_IPV4MAPPED_LOOPBACK));

        REQUIRE(clarinet_addr_is_equivalent(&p.first, &p.second));
    }

    SECTION("Constant ipv6 addresses (Not Equivalent)")
    {
        const auto& p = GENERATE(
            std::make_pair(CLARINET_ADDR_IPV6_ANY, CLARINET_ADDR_IPV6_LOOPBACK),
            std::make_pair(CLARINET_ADDR_IPV6_ANY, CLARINET_ADDR_IPV4MAPPED_LOOPBACK),
            std::make_pair(CLARINET_ADDR_IPV6_LOOPBACK, CLARINET_ADDR_IPV4MAPPED_LOOPBACK));

        REQUIRE_FALSE(clarinet_addr_is_equivalent(&p.first, &p.second));
    }

    SECTION("Custom ipv4 addresses (Equivalent)")
    {
        clarinet_addr a;
        memset(&a, 0xAA, sizeof(a)); /* memory noise */
        a.family = CLARINET_AF_INET;
        a.as.ipv4.u.byte[0] = 0x00;
        a.as.ipv4.u.byte[1] = 0x11;
        a.as.ipv4.u.byte[2] = 0x22;
        a.as.ipv4.u.byte[3] = 0x33;

        clarinet_addr b;
        memset(&b, 0xBB, sizeof(b)); /* memory noise */
        b.family = CLARINET_AF_INET;
        b.as.ipv4.u.byte[0] = 0x00;
        b.as.ipv4.u.byte[1] = 0x11;
        b.as.ipv4.u.byte[2] = 0x22;
        b.as.ipv4.u.byte[3] = 0x33;

        REQUIRE(clarinet_addr_is_equivalent(&a, &b));
    }

    SECTION("Custom ipv6 addresses (Equivalent)")
    {
        SECTION("Same ipv6 addresses")
        {
            clarinet_addr a;
            memset(&a, 0xAA, sizeof(a)); /* memory noise */
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
            memset(&b, 0xBB, sizeof(b)); /* memory noise */
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
        }

        SECTION("Same ipv4 address one is an ipv4 mapped to ipv6")
        {
            clarinet_addr a;
            memset(&a, 0xAA, sizeof(a)); /* memory noise */
            a.family = CLARINET_AF_INET;
            a.as.ipv4.u.byte[0] = 0x00;
            a.as.ipv4.u.byte[1] = 0x11;
            a.as.ipv4.u.byte[2] = 0x22;
            a.as.ipv4.u.byte[3] = 0x33;

            clarinet_addr b;
            memset(&b, 0xBB, sizeof(b)); /* memory noise */
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
            memset(&a, 0xAA, sizeof(a)); /* memory noise */
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
            memset(&b, 0xBB, sizeof(b)); /* memory noise */
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
            memset(&a, 0xAA, sizeof(a)); /* memory noise */
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
            memset(&b, 0xBB, sizeof(b)); /* memory noise */
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

            REQUIRE_FALSE(clarinet_addr_is_equivalent(&a, &b));
        }

        SECTION("Same ipv6 address but different cope id")
        {
            clarinet_addr a;
            memset(&a, 0xAA, sizeof(a)); /* memory noise */
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
            memset(&b, 0xBB, sizeof(b)); /* memory noise */
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

TEST_CASE("Address Mapping", "[address]")
{
    SECTION("Valid map to ipv4")
    {
#if defined(CLARINET_ENABLE_IPV6)
        const auto& p = GENERATE(
            std::make_pair(CLARINET_ADDR_IPV4MAPPED_LOOPBACK, CLARINET_ADDR_IPV4_LOOPBACK),
            std::make_pair(CLARINET_ADDR_IPV4_LOOPBACK, CLARINET_ADDR_IPV4_LOOPBACK));
#else 
        const auto& p = GENERATE(
            std::make_pair(CLARINET_ADDR_IPV4_LOOPBACK, CLARINET_ADDR_IPV4_LOOPBACK));
#endif

        SECTION("Map to ipv4 and compare")
        {
            clarinet_addr dst;
            REQUIRE(clarinet_addr_map_to_ipv4(&dst, &p.first) == CLARINET_ENONE);
            REQUIRE(clarinet_addr_is_equal(&p.second, &dst));
        }
    }

    SECTION("Invalid map To ipv4")
    {
#if defined(CLARINET_ENABLE_IPV6)
        const auto& src = GENERATE(
            CLARINET_ADDR_IPV6_LOOPBACK,
            CLARINET_ADDR_NONE);
#else
        const auto& src = GENERATE(
            CLARINET_ADDR_IPV4MAPPED_LOOPBACK,
            CLARINET_ADDR_IPV6_LOOPBACK,
            CLARINET_ADDR_NONE);
#endif

        SECTION("Map to ipv4 and compare")
        {
            clarinet_addr dst;
            REQUIRE_FALSE(clarinet_addr_map_to_ipv4(&dst, &src) == CLARINET_ENONE);
        }
    }

    SECTION("Valid map to ipv6")
    {
#if defined(CLARINET_ENABLE_IPV6)
        const auto& p = GENERATE(
            std::make_pair(CLARINET_ADDR_IPV4_LOOPBACK, CLARINET_ADDR_IPV4MAPPED_LOOPBACK),
            std::make_pair(CLARINET_ADDR_IPV6_LOOPBACK, CLARINET_ADDR_IPV6_LOOPBACK));

        SECTION("Map to ipv6 and compare")
        {
            clarinet_addr dst;
            REQUIRE(clarinet_addr_map_to_ipv6(&dst, &p.first) == CLARINET_ENONE);
            REQUIRE(clarinet_addr_is_equal(&p.second, &dst));
        }
#endif
    }

    SECTION("Invalid map To ipv6")
    {
#if defined(CLARINET_ENABLE_IPV6)
        const auto& src = GENERATE(
            CLARINET_ADDR_NONE);
#else
        const auto& src = GENERATE(
            CLARINET_ADDR_IPV4MAPPED_LOOPBACK,
            CLARINET_ADDR_IPV6_LOOPBACK,
            CLARINET_ADDR_NONE);
#endif

        SECTION("Map to ipv6 and compare")
        {
            clarinet_addr dst;
            REQUIRE_FALSE(clarinet_addr_map_to_ipv6(&dst, &src) == CLARINET_ENONE);
        }
    }
}

TEST_CASE("Address to string", "[address]")
{
}

TEST_CASE("Address from string", "[address]")
{
}

TEST_CASE("Endpoint Size", "[endpoint]")
{
}

TEST_CASE("Endpoint to string", "[endpoint]")
{
}
    
TEST_CASE("Endpoint from string", "[endpoint]")
{
}

#if 0

#define test_clarinet_addr_to_string_step(src, expected, noise) {   \
    char dst[CLARINET_ADDR_STRLEN] = {0};                           \
    for (size_t i = 0; i < sizeof(dst); ++i) dst[i] = (*noise)++;   \
    int actual = clarinet_addr_to_string(dst, sizeof(dst), src);    \
    assert_int_equal(strlen(expected), actual);                     \
    assert_memory_equal(dst, expected, strlen(expected) + 1);       \
}

static void test_clarinet_addr_to_string(void** state)
{
    CLARINET_IGNORE(state);
    uint8_t noise = 0;

    /* Tests with invalid arguments */
    
    {
        clarinet_addr src = CLARINET_ADDR_IPV4_ANY;
        int actual = clarinet_addr_to_string(NULL, 0, &src);
        assert_int_equal(CLARINET_EINVAL, actual);
    }
   
    {
        char dst[CLARINET_ADDR_STRLEN];
        int actual = clarinet_addr_to_string(dst, sizeof(dst), NULL);
        assert_int_equal(CLARINET_EINVAL, actual);
    }
    
    {
        clarinet_addr src = CLARINET_ADDR_IPV4_ANY;
        char dst[4];
        int actual = clarinet_addr_to_string(dst, sizeof(dst), &src);
        assert_int_equal(CLARINET_EINVAL, actual);
    }
    
    {
        clarinet_addr src = {0};
        for (size_t i = 0; i < sizeof(src); ++i) ((uint8_t*)&src)[i] = noise++; /* memory noise */
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
        for (size_t i = 0; i < sizeof(dst); ++i) dst[i] = noise++;
        int actual = clarinet_addr_to_string(dst, sizeof(dst), &src);
        assert_int_equal(CLARINET_EINVAL, actual);
    }   

    /* Tests with valid arguments */

    {
        clarinet_addr src = CLARINET_ADDR_IPV4_ANY;
        const char* expected = "0.0.0.0";
        test_clarinet_addr_to_string_step(&src, expected, &noise);
    }

    {
        clarinet_addr src = CLARINET_ADDR_IPV4_LOOPBACK;
        const char* expected = "127.0.0.1";
        test_clarinet_addr_to_string_step(&src, expected, &noise);
    }

    {        
        clarinet_addr src = CLARINET_ADDR_IPV4_BROADCAST;
        const char* expected = "255.255.255.255";
        test_clarinet_addr_to_string_step(&src, expected, &noise);
    }

    {
        clarinet_addr src = CLARINET_ADDR_IPV6_LOOPBACK;
        const char* expected = "::1";
        test_clarinet_addr_to_string_step(&src, expected, &noise);
    }
    
    {       
        clarinet_addr src = CLARINET_ADDR_IPV4MAPPED_LOOPBACK;
        const char* expected = "::ffff:127.0.0.1";
        test_clarinet_addr_to_string_step(&src, expected, &noise);
    }
    
    {       
        clarinet_addr src = CLARINET_ADDR_IPV4MAPPED_LOOPBACK;
        src.as.ipv6.scope_id = 1234567890;
        const char* expected = "::ffff:127.0.0.1%1234567890";
        test_clarinet_addr_to_string_step(&src, expected, &noise);
    }
    
    {
        clarinet_addr src = {0};
        for (size_t i = 0; i < sizeof(src); ++i) ((uint8_t*)&src)[i] = noise++; /* memory noise */
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
        const char* expected = "b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779";
        test_clarinet_addr_to_string_step(&src, expected, &noise);
    }   
}

#define test_clarinet_addr_from_string_step(src, srclen, expected, noise) {     \
    clarinet_addr dst = {0};                                                    \
    for (size_t i = 0; i < sizeof(dst); ++i) ((uint8_t*)&dst)[i] = (*noise)++;  \
    int actual = clarinet_addr_from_string(&dst, src, srclen);                  \
    assert_int_equal(CLARINET_ENONE, actual);                                   \
    assert_true(clarinet_addr_is_equal(expected, &dst));                        \
}

static void test_clarinet_addr_from_string(void** state)
{
    CLARINET_IGNORE(state);
    uint8_t noise = 0;

     /* Tests with invalid arguments */

    {
        clarinet_addr dst = {0};
        int actual = clarinet_addr_from_string(&dst, NULL, 0);
        assert_int_equal(CLARINET_EINVAL, actual);
    }

    {
        const char* src = "192.168.0.1";
        int actual = clarinet_addr_from_string(NULL, src, sizeof(src));
        assert_int_equal(CLARINET_EINVAL, actual);
    }

    {
        clarinet_addr dst = {0};
        const char* src = "";
        int actual = clarinet_addr_from_string(&dst, src, strlen(src));
        assert_int_equal(CLARINET_EINVAL, actual);
    }

    {
        
        clarinet_addr dst = {0};
        const char* src = "192.168.0.1";
        int actual = clarinet_addr_from_string(&dst, src, 0);
        assert_int_equal(CLARINET_EINVAL, actual);
    }

    {       
        clarinet_addr dst = {0};
        const char* src = "192.168.0.1";
        int actual = clarinet_addr_from_string(&dst, src, strlen(src) >> 1);
        assert_int_equal(CLARINET_EINVAL, actual);
    }

    {        
        clarinet_addr dst = {0};
        const char* src = "::1%";
        int actual = clarinet_addr_from_string(&dst, src, strlen(src));
        assert_int_equal(CLARINET_EINVAL, actual);
    }

    {        
        clarinet_addr dst = {0};
        const char* src = "::1%4294969999";
        int actual = clarinet_addr_from_string(&dst, src, strlen(src));
        assert_int_equal(CLARINET_EINVAL, actual);
    }

    /* Tests with valid arguments */

    {
        clarinet_addr expected = CLARINET_ADDR_IPV4_ANY;
        const char* src = "0.0.0.0";        
        test_clarinet_addr_from_string_step(src, strlen(src), &expected, &noise);
    }

    {
        clarinet_addr expected = CLARINET_ADDR_IPV4_LOOPBACK;
        const char* src = "127.0.0.1";
        test_clarinet_addr_from_string_step(src, strlen(src), &expected, &noise);
    }

    {
        clarinet_addr expected = CLARINET_ADDR_IPV4_BROADCAST;
        const char* src = "255.255.255.255";
        test_clarinet_addr_from_string_step(src, strlen(src), &expected, &noise);
    }

    {
        clarinet_addr expected = CLARINET_ADDR_IPV6_LOOPBACK;
        const char* src = "::1";
        test_clarinet_addr_from_string_step(src, strlen(src), &expected, &noise);
    }

    {
        clarinet_addr expected = CLARINET_ADDR_IPV4MAPPED_LOOPBACK;;
        const char* src = "::ffff:127.0.0.1";
        test_clarinet_addr_from_string_step(src, strlen(src), &expected, &noise);
    }

    {
        clarinet_addr expected = CLARINET_ADDR_IPV4MAPPED_LOOPBACK;
        expected.as.ipv6.scope_id = 1234567890;
        const char* src = "::ffff:127.0.0.1%1234567890";
        test_clarinet_addr_from_string_step(src, strlen(src), &expected, &noise);
    }

    {
        clarinet_addr expected = {0};
        expected.family = CLARINET_AF_INET6;
        expected.as.ipv6.flowinfo = 0;
        expected.as.ipv6.u.byte[0] = 0xB0;
        expected.as.ipv6.u.byte[1] = 0xB1;
        expected.as.ipv6.u.byte[2] = 0xB2;
        expected.as.ipv6.u.byte[3] = 0xB3;
        expected.as.ipv6.u.byte[4] = 0xB4;
        expected.as.ipv6.u.byte[5] = 0xB5;
        expected.as.ipv6.u.byte[6] = 0xB6;
        expected.as.ipv6.u.byte[7] = 0xB7;
        expected.as.ipv6.u.byte[8] = 0xB8;
        expected.as.ipv6.u.byte[9] = 0xB9;
        expected.as.ipv6.u.byte[10] = 0xBA;
        expected.as.ipv6.u.byte[11] = 0xBB;
        expected.as.ipv6.u.byte[12] = 0xBC;
        expected.as.ipv6.u.byte[13] = 0xBD;
        expected.as.ipv6.u.byte[14] = 0xBE;
        expected.as.ipv6.u.byte[15] = 0xBF;
        expected.as.ipv6.scope_id = 3233923779;
        const char* src = "b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779";
        test_clarinet_addr_from_string_step(src, strlen(src), &expected, &noise);
    }
}

static void test_clarinet_endpoint_size(void** state)
{
    CLARINET_IGNORE(state);

    assert_int_equal(32, sizeof(clarinet_endpoint));

    assert_int_equal(sizeof(clarinet_addr), sizeof(((clarinet_endpoint*)0)->addr));

    assert_int_equal(sizeof(uint16_t), sizeof(((clarinet_endpoint*)0)->port));
}

static void test_clarinet_endpoint_strlen(void** state)
{
    CLARINET_IGNORE(state);

    assert_true(CLARINET_ENDPOINT_STRLEN >= 65); /* "[0000:0000:0000:0000:0000:ffff:255.255.255.255%4294967296]:65535" */
}

#define test_clarinet_endpoint_to_string_step(src, expected, noise) {   \
    char dst[CLARINET_ENDPOINT_STRLEN] = {0};                           \
    for (size_t i = 0; i < sizeof(dst); ++i) dst[i] = (*noise)++;       \
    int actual = clarinet_endpoint_to_string(dst, sizeof(dst), src);    \
    assert_int_equal(strlen(expected), actual);                         \
    assert_memory_equal(dst, expected, strlen(expected) + 1);           \
}

static void test_clarinet_endpoint_to_string(void** state)
{
    CLARINET_IGNORE(state);
    uint8_t noise = 0;
    
    /* Tests with invalid arguments */
    
    {
        clarinet_endpoint src = {0};
        for (size_t i = 0; i < sizeof(src); ++i) ((uint8_t*)&src)[i] = noise++; /* memory noise */
        src.addr = CLARINET_ADDR_IPV4_ANY;
        int actual = clarinet_endpoint_to_string(NULL, 0, &src);
        assert_int_equal(CLARINET_EINVAL, actual);
    }
    
    {
        char dst[CLARINET_ADDR_STRLEN] = {0};
        for (size_t i = 0; i < sizeof(dst); ++i) ((uint8_t*)&dst)[i] = noise++; /* memory noise */
        int actual = clarinet_endpoint_to_string(dst, sizeof(dst), NULL);
        assert_int_equal(CLARINET_EINVAL, actual);
    }
    
    {
        clarinet_endpoint src = {0};
        for (size_t i = 0; i < sizeof(src); ++i) ((uint8_t*)&src)[i] = noise++; /* memory noise */
        src.addr = CLARINET_ADDR_IPV4_ANY;
        char dst[4];
        memset(dst, 0xDD, sizeof(dst)); /* memory noise */
        int actual = clarinet_endpoint_to_string(dst, sizeof(dst), &src);
        assert_int_equal(CLARINET_EINVAL, actual);
    }
    
    {
        clarinet_endpoint src = {0};
        for (size_t i = 0; i < sizeof(src); ++i) ((uint8_t*)&src)[i] = noise++; /* memory noise */
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
        src.port = 12345;
        const char* expected = "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779]:12345";
        assert_true(CLARINET_ADDR_STRLEN < CLARINET_ENDPOINT_STRLEN);
        assert_true(CLARINET_ADDR_STRLEN < strlen(expected));
        char dst[CLARINET_ADDR_STRLEN] = {0}; /* pass a buffer too small on purpose */
        for (size_t i = 0; i < sizeof(dst); ++i) dst[i] = noise++; /* add noise */
        int actual = clarinet_endpoint_to_string(dst, sizeof(dst), &src);
        assert_int_equal(CLARINET_EINVAL, actual);
    }    
    
    /* Tests with valid arguments and port = 0 */
    
    {       
        clarinet_endpoint src = {0};
        for (size_t i = 0; i < sizeof(src); ++i) ((uint8_t*)&src)[i] = noise++; /* memory noise */
        src.addr = CLARINET_ADDR_IPV4_ANY;
        src.port = 0;
        const char* expected = "0.0.0.0:0";        
        test_clarinet_endpoint_to_string_step(&src, expected, &noise);
    }

    {
        clarinet_endpoint src = {0};
        for (size_t i = 0; i < sizeof(src); ++i) ((uint8_t*)&src)[i] = noise++; /* memory noise */
        src.addr = CLARINET_ADDR_IPV4_LOOPBACK;
        src.port = 0;
        const char* expected = "127.0.0.1:0";
        test_clarinet_endpoint_to_string_step(&src, expected, &noise);
    }

    {
        clarinet_endpoint src = {0};
        for (size_t i = 0; i < sizeof(src); ++i) ((uint8_t*)&src)[i] = noise++; /* memory noise */
        src.addr = CLARINET_ADDR_IPV4_BROADCAST;
        src.port = 0;
        const char* expected = "255.255.255.255:0";
        test_clarinet_endpoint_to_string_step(&src, expected, &noise);
    }

    {       
        clarinet_endpoint src = {0};
        for (size_t i = 0; i < sizeof(src); ++i) ((uint8_t*)&src)[i] = noise++; /* memory noise */
        src.addr = CLARINET_ADDR_IPV6_LOOPBACK;
        src.port = 0;
        const char* expected = "[::1]:0";
        test_clarinet_endpoint_to_string_step(&src, expected, &noise);
    }
    
    {       
        clarinet_endpoint src = {0};
        for (size_t i = 0; i < sizeof(src); ++i) ((uint8_t*)&src)[i] = noise++; /* memory noise */
        src.addr = CLARINET_ADDR_IPV4MAPPED_LOOPBACK;
        src.port = 0;
        const char* expected = "[::ffff:127.0.0.1]:0";
        test_clarinet_endpoint_to_string_step(&src, expected, &noise);
    }
    
    {       
        clarinet_endpoint src = {0};
        for (size_t i = 0; i < sizeof(src); ++i) ((uint8_t*)&src)[i] = noise++; /* memory noise */
        src.addr = CLARINET_ADDR_IPV4MAPPED_LOOPBACK;
        src.addr.as.ipv6.scope_id = 1234567890;
        src.port = 0;
        const char* expected = "[::ffff:127.0.0.1%1234567890]:0";
        test_clarinet_endpoint_to_string_step(&src, expected, &noise);
    }
    
    {       
        clarinet_endpoint src = {0};
        for (size_t i = 0; i < sizeof(src); ++i) ((uint8_t*)&src)[i] = noise++; /* memory noise */
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
        src.port = 0;
        const char* expected = "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779]:0";
        test_clarinet_endpoint_to_string_step(&src, expected, &noise);
    }
    
    /* Tests with valid arguments and port > 0 */
    
    {       
        clarinet_endpoint src = {0};
        for (size_t i = 0; i < sizeof(src); ++i) ((uint8_t*)&src)[i] = noise++; /* memory noise */
        src.addr = CLARINET_ADDR_IPV4_ANY;
        src.port = 1234;
        const char* expected = "0.0.0.0:1234";
        test_clarinet_endpoint_to_string_step(&src, expected, &noise);
    }

    {
        clarinet_endpoint src = {0};
        for (size_t i = 0; i < sizeof(src); ++i) ((uint8_t*)&src)[i] = noise++; /* memory noise */
        src.addr = CLARINET_ADDR_IPV4_LOOPBACK;
        src.port = 1;
        const char* expected = "127.0.0.1:1";
        test_clarinet_endpoint_to_string_step(&src, expected, &noise);
    }

    {
        clarinet_endpoint src = {0};
        for (size_t i = 0; i < sizeof(src); ++i) ((uint8_t*)&src)[i] = noise++; /* memory noise */
        src.addr = CLARINET_ADDR_IPV4_BROADCAST;
        src.port = 78;
        const char* expected = "255.255.255.255:78";
        test_clarinet_endpoint_to_string_step(&src, expected, &noise);
    }

    {       
        clarinet_endpoint src = {0};
        for (size_t i = 0; i < sizeof(src); ++i) ((uint8_t*)&src)[i] = noise++; /* memory noise */
        src.addr = CLARINET_ADDR_IPV6_LOOPBACK;
        src.port = 65534;
        const char* expected = "[::1]:65534";
        test_clarinet_endpoint_to_string_step(&src, expected, &noise);
    }
    
    {
        clarinet_endpoint src = {0};
        for (size_t i = 0; i < sizeof(src); ++i) ((uint8_t*)&src)[i] = noise++; /* memory noise */
        src.addr = CLARINET_ADDR_IPV4MAPPED_LOOPBACK;
        src.port = 5678;
        const char* expected = "[::ffff:127.0.0.1]:5678";
        test_clarinet_endpoint_to_string_step(&src, expected, &noise);
    }
    
    {
        clarinet_endpoint src = {0};
        for (size_t i = 0; i < sizeof(src); ++i) ((uint8_t*)&src)[i] = noise++; /* memory noise */
        src.addr = CLARINET_ADDR_IPV4MAPPED_LOOPBACK;
        src.addr.as.ipv6.scope_id = 1234567890;
        src.port = 999;
        const char* expected = "[::ffff:127.0.0.1%1234567890]:999";
        test_clarinet_endpoint_to_string_step(&src, expected, &noise);
    }
    
    {
        clarinet_endpoint src = {0};
        for (size_t i = 0; i < sizeof(src); ++i) ((uint8_t*)&src)[i] = noise++; /* memory noise */
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
        src.port = 12345;
        const char* expected = "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779]:12345";
        test_clarinet_endpoint_to_string_step(&src, expected, &noise);
    }    
}

static void test_clarinet_endpoint_from_string(void** state)
{
    CLARINET_IGNORE(state);
    // uint8_t noise = 0;

     /* Tests with invalid arguments */

    {
        clarinet_endpoint dst = {0};
        int actual = clarinet_endpoint_from_string(&dst, NULL, 0);
        assert_int_equal(CLARINET_EINVAL, actual);
    }

    {
        const char* src = "192.168.0.1:12345";
        int actual = clarinet_endpoint_from_string(NULL, src, sizeof(src));
        assert_int_equal(CLARINET_EINVAL, actual);
    }

    {
        clarinet_endpoint dst = {0};
        const char* src = "";
        int actual = clarinet_endpoint_from_string(&dst, src, strlen(src));
        assert_int_equal(CLARINET_EINVAL, actual);
    }

    {

        clarinet_endpoint dst = {0};
        const char* src = "192.168.0.1:12345";
        int actual = clarinet_endpoint_from_string(&dst, src, 0);
        assert_int_equal(CLARINET_EINVAL, actual);
    }

    {
        clarinet_endpoint dst = {0};
        const char* src = "192.168.0.1:12345";
        int actual = clarinet_endpoint_from_string(&dst, src, strlen(src) >> 1);
        assert_int_equal(CLARINET_EINVAL, actual);
    }

    {
        clarinet_endpoint dst = {0};
        const char* src = "[::1%]:12345";
        int actual = clarinet_endpoint_from_string(&dst, src, strlen(src));
        assert_int_equal(CLARINET_EINVAL, actual);
    }

    {
        clarinet_endpoint dst = {0};
        const char* src = "[::1%4294969999]:12345";
        int actual = clarinet_endpoint_from_string(&dst, src, strlen(src));
        assert_int_equal(CLARINET_EINVAL, actual);
    }
}

#endif
