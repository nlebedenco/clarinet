#include "test.h"

#include <vector>

static starter init([] // NOLINT(cert-err58-cpp)
{
    #if !CLARINET_ENABLE_IPV6
    WARN("IPv6 suport is disabled. Some IPV6 tests may be skipped. ");
    #endif
});

TEST_CASE("Address Init", "[address]")
{
    clarinet_addr src = { 0 };
    REQUIRE(src.family == 0);
    REQUIRE(src.as.ipv4.u.dword == 0);
    REQUIRE(src.as.ipv6.flowinfo == 0);
    REQUIRE(src.as.ipv6.u.dword[0] == 0);
    REQUIRE(src.as.ipv6.u.dword[1] == 0);
    REQUIRE(src.as.ipv6.u.dword[2] == 0);
    REQUIRE(src.as.ipv6.u.dword[3] == 0);
    REQUIRE(src.as.ipv6.scope_id == 0);
}

TEST_CASE("Address Size", "[address]")
{
    REQUIRE(sizeof(clarinet_addr) == 28);
    REQUIRE(sizeof(((clarinet_addr*)nullptr)->family) == sizeof(uint16_t));
    REQUIRE(sizeof(((clarinet_addr*)nullptr)->as.ipv4.u.byte[0]) == sizeof(uint8_t));
    REQUIRE(sizeof(((clarinet_addr*)nullptr)->as.ipv4.u.word[0]) == sizeof(uint16_t));
    REQUIRE(sizeof(((clarinet_addr*)nullptr)->as.ipv4.u.dword) == sizeof(uint32_t));
    REQUIRE(sizeof(((clarinet_addr*)nullptr)->as.ipv6.u.byte[0]) == sizeof(uint8_t));
    REQUIRE(sizeof(((clarinet_addr*)nullptr)->as.ipv6.u.word[0]) == sizeof(uint16_t));
    REQUIRE(sizeof(((clarinet_addr*)nullptr)->as.ipv6.u.dword[0]) == sizeof(uint32_t));
    REQUIRE(sizeof(((clarinet_addr*)nullptr)->as.ipv6.flowinfo) == sizeof(uint32_t));
    REQUIRE(sizeof(((clarinet_addr*)nullptr)->as.ipv6.scope_id) == sizeof(uint32_t));
    REQUIRE(sizeof(struct clarinet_ipv4) == 24);
    REQUIRE(sizeof(struct clarinet_ipv6) == 24);
    REQUIRE(sizeof(((clarinet_addr*)nullptr)->as.ipv4) == sizeof(struct clarinet_ipv4));
    REQUIRE(sizeof(((clarinet_addr*)nullptr)->as.ipv6) == sizeof(struct clarinet_ipv6));
}

TEST_CASE("Address Max Strlen", "[address]")
{
    REQUIRE(CLARINET_ADDR_STRLEN == 57); // "0000:0000:0000:0000:0000:ffff:255.255.255.255%4294967295"
}

TEST_CASE("IPv4 Address Any", "[address][ipv4][any]")
{
    clarinet_addr actual = clarinet_addr_any_ipv4;
    REQUIRE(actual.family == CLARINET_AF_INET);
    REQUIRE(actual.as.ipv4.u.dword == 0);
    REQUIRE(actual.as.ipv4.u.word[0] == 0);
    REQUIRE(actual.as.ipv4.u.word[1] == 0);
    REQUIRE(actual.as.ipv4.u.byte[0] == 0);
    REQUIRE(actual.as.ipv4.u.byte[1] == 0);
    REQUIRE(actual.as.ipv4.u.byte[2] == 0);
    REQUIRE(actual.as.ipv4.u.byte[3] == 0);


    SECTION("Check constant CLARINET_ADDR_IPV4_ANY")
    {
        clarinet_addr expected = clarinet_addr_none;
        expected.family = CLARINET_AF_INET;
        REQUIRE_THAT(memory(&expected, sizeof(expected)), Equals(memory(&actual, sizeof(actual))));
    }

    SECTION("Check 0.0.0.0 is ipv4 any")
    {
        REQUIRE(clarinet_addr_is_ipv4(&actual));
        REQUIRE(clarinet_addr_is_any_ipv4(&actual));
        REQUIRE(clarinet_addr_is_any_ip(&actual));
    }
}

TEST_CASE("IPv4 Address Loopback", "[address][ipv4][loopback]")
{
    clarinet_addr actual = clarinet_addr_loopback_ipv4;
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

    SECTION("Check 127.0.0.1 is an ipv4 looback")
    {
        REQUIRE(clarinet_addr_is_ipv4(&actual));
        REQUIRE(clarinet_addr_is_loopback_ipv4(&actual));
        REQUIRE(clarinet_addr_is_loopback_ip(&actual));
    }

    SECTION("Check 127.0.0.0 is NOT an ipv4 loopback")
    {
        actual.as.ipv4.u.byte[3] = 0;
        REQUIRE_FALSE(clarinet_addr_is_loopback_ipv4(&actual));
        REQUIRE_FALSE(clarinet_addr_is_loopback_ip(&actual));
    }

    SECTION("Check 127.0.0.255 is NOT an ipv4 loopback")
    {
        actual.as.ipv4.u.byte[3] = 255;
        REQUIRE_FALSE(clarinet_addr_is_loopback_ipv4(&actual));
        REQUIRE_FALSE(clarinet_addr_is_loopback_ip(&actual));
    }

        // Technically we should assert the whole range 127.0.0.1 to 127.255.255.254 but sampling must suffice.

    SECTION("Check 127.0.0.2 is an ipv4 loopback")
    {
        actual.as.ipv4.u.byte[3] = 2;
        REQUIRE(clarinet_addr_is_loopback_ipv4(&actual));
        REQUIRE(clarinet_addr_is_loopback_ip(&actual));
    }

    SECTION("Check 127.0.0.254 is an ipv4 loopback")
    {
        actual.as.ipv4.u.byte[3] = 254;
        REQUIRE(clarinet_addr_is_loopback_ipv4(&actual));
        REQUIRE(clarinet_addr_is_loopback_ip(&actual));
    }

    SECTION("Check 127.255.255.254 is an ipv4 loopback")
    {
        actual.as.ipv4.u.byte[1] = 255;
        actual.as.ipv4.u.byte[2] = 255;
        actual.as.ipv4.u.byte[3] = 254;
        REQUIRE(clarinet_addr_is_loopback_ipv4(&actual));
        REQUIRE(clarinet_addr_is_loopback_ip(&actual));
    }
}

TEST_CASE("IPv4 Address Broadcast", "[address][ipv4][broadcast]")
{
    clarinet_addr actual = clarinet_addr_broadcast_ipv4;
    REQUIRE(actual.family == CLARINET_AF_INET);
    REQUIRE(actual.as.ipv4.u.byte[0] == 255);
    REQUIRE(actual.as.ipv4.u.byte[1] == 255);
    REQUIRE(actual.as.ipv4.u.byte[2] == 255);
    REQUIRE(actual.as.ipv4.u.byte[3] == 255);

    SECTION("Check constant clarinet_addr_broadcast_ipv4")
    {
        clarinet_addr expected = clarinet_addr_none;
        expected.family = CLARINET_AF_INET;
        expected.as.ipv4.u.byte[0] = 255;
        expected.as.ipv4.u.byte[1] = 255;
        expected.as.ipv4.u.byte[2] = 255;
        expected.as.ipv4.u.byte[3] = 255;

        REQUIRE_THAT(memory(&expected, sizeof(expected)), Equals(memory(&actual, sizeof(actual))));
    }

    SECTION("Check 255.255.255.255 is an ipv4 broadcast")
    {
        REQUIRE(clarinet_addr_is_ipv4(&actual));
        REQUIRE(clarinet_addr_is_broadcast_ipv4(&actual));
        REQUIRE(clarinet_addr_is_broadcast_ip(&actual));
    }

    SECTION("Check 255.255.255.0 is NOT an ipv4 broadcast")
    {
        actual.as.ipv4.u.byte[3] = 0;
        REQUIRE_FALSE(clarinet_addr_is_broadcast_ipv4(&actual));
    }

    SECTION("Check 255.255.255.127 is NOT an ipv4 broadcast")
    {
        actual.as.ipv4.u.byte[3] = 127;
        REQUIRE_FALSE(clarinet_addr_is_broadcast_ipv4(&actual));
    }

    SECTION("Check 255.255.255.2 is NOT an ipv4 broadcast")
    {
        actual.as.ipv4.u.byte[3] = 2;
        REQUIRE_FALSE(clarinet_addr_is_broadcast_ipv4(&actual));
    }

    SECTION("Check 255.255.255.127 is NOT an ipv4 broadcast")
    {
        actual.as.ipv4.u.byte[3] = 127;
        REQUIRE_FALSE(clarinet_addr_is_broadcast_ipv4(&actual));
    }

    SECTION("Check ::FFFF:255.255.255.255 is NOT an ipv4 broadcast")
    {
        actual.family = CLARINET_AF_INET6;
        actual.as.ipv6.u.byte[10] = 255;
        actual.as.ipv6.u.byte[11] = 255;
        actual.as.ipv6.u.byte[12] = 255;
        actual.as.ipv6.u.byte[13] = 255;
        actual.as.ipv6.u.byte[14] = 255;
        actual.as.ipv6.u.byte[15] = 255;
        REQUIRE_FALSE(clarinet_addr_is_broadcast_ipv4(&actual));
    }
}

TEST_CASE("IPv4 Mapped Loopback", "[address][ipv6][ipv4][loopback]")
{
    clarinet_addr actual = clarinet_addr_loopback_ipv4mapped;
    REQUIRE(actual.family == CLARINET_AF_INET6);
    REQUIRE(actual.as.ipv6.flowinfo == 0);
    REQUIRE(actual.as.ipv6.scope_id == 0);
    REQUIRE(actual.as.ipv6.u.dword[0] == 0);
    REQUIRE(actual.as.ipv6.u.dword[1] == 0);
    REQUIRE(actual.as.ipv6.u.word[0] == 0);
    REQUIRE(actual.as.ipv6.u.word[1] == 0);
    REQUIRE(actual.as.ipv6.u.word[2] == 0);
    REQUIRE(actual.as.ipv6.u.word[3] == 0);
    REQUIRE(actual.as.ipv6.u.word[4] == 0);
    REQUIRE(actual.as.ipv6.u.byte[0] == 0);
    REQUIRE(actual.as.ipv6.u.byte[1] == 0);
    REQUIRE(actual.as.ipv6.u.byte[2] == 0);
    REQUIRE(actual.as.ipv6.u.byte[3] == 0);
    REQUIRE(actual.as.ipv6.u.byte[4] == 0);
    REQUIRE(actual.as.ipv6.u.byte[5] == 0);
    REQUIRE(actual.as.ipv6.u.byte[6] == 0);
    REQUIRE(actual.as.ipv6.u.byte[7] == 0);
    REQUIRE(actual.as.ipv6.u.byte[8] == 0);
    REQUIRE(actual.as.ipv6.u.byte[9] == 0);

    REQUIRE(actual.as.ipv6.u.byte[10] == 255);
    REQUIRE(actual.as.ipv6.u.byte[11] == 255);
    REQUIRE(actual.as.ipv6.u.byte[12] == 127);
    REQUIRE(actual.as.ipv6.u.byte[13] == 0);
    REQUIRE(actual.as.ipv6.u.byte[14] == 0);
    REQUIRE(actual.as.ipv6.u.byte[15] == 1);

    SECTION("Check constant clarinet_addr_loopback_ipv4mapped")
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

    SECTION("Check ::FFFF:127.0.0.1 is an ipv4 mapped loopback")
    {
        REQUIRE(clarinet_addr_is_ipv4mapped(&actual));
        REQUIRE(clarinet_addr_is_loopback_ipv4mapped(&actual));
        REQUIRE(clarinet_addr_is_loopback_ip(&actual));
    }

    SECTION("Check ::FFFF:127.0.0.0 is NOT an ipv4 mapped loopback")
    {
        actual.as.ipv6.u.byte[15] = 0;
        REQUIRE_FALSE(clarinet_addr_is_loopback_ipv4mapped(&actual));
    }

    SECTION("Check ::FFFF:127.0.0.255 is NOT an ipv4 mapped loopback")
    {
        actual.as.ipv6.u.byte[15] = 255;
        REQUIRE_FALSE(clarinet_addr_is_loopback_ipv4mapped(&actual));
    }

    SECTION("Check ::FFFF:127.0.0.2 is an ipv4 mapped loopback")
    {
        actual.as.ipv6.u.byte[15] = 2;
        REQUIRE(clarinet_addr_is_loopback_ipv4mapped(&actual));
    }

    SECTION("Check ::FFFF:127.255.255.254 is an ipv4 mapped loopback")
    {
        actual.as.ipv6.u.byte[13] = 255;
        actual.as.ipv6.u.byte[14] = 255;
        actual.as.ipv6.u.byte[14] = 254;
        REQUIRE(clarinet_addr_is_loopback_ipv4mapped(&actual));
    }
}

TEST_CASE("IPv6 Address Any", "[address][ipv6][any]")
{
    clarinet_addr actual = clarinet_addr_any_ipv6;
    REQUIRE(actual.family == CLARINET_AF_INET6);
    REQUIRE(actual.as.ipv6.flowinfo == 0);
    REQUIRE(actual.as.ipv6.scope_id == 0);
    REQUIRE(actual.as.ipv6.u.dword[0] == 0);
    REQUIRE(actual.as.ipv6.u.dword[1] == 0);
    REQUIRE(actual.as.ipv6.u.dword[2] == 0);
    REQUIRE(actual.as.ipv6.u.dword[3] == 0);
    REQUIRE(actual.as.ipv6.u.word[0] == 0);
    REQUIRE(actual.as.ipv6.u.word[1] == 0);
    REQUIRE(actual.as.ipv6.u.word[2] == 0);
    REQUIRE(actual.as.ipv6.u.word[3] == 0);
    REQUIRE(actual.as.ipv6.u.word[4] == 0);
    REQUIRE(actual.as.ipv6.u.word[5] == 0);
    REQUIRE(actual.as.ipv6.u.word[6] == 0);
    REQUIRE(actual.as.ipv6.u.word[7] == 0);
    REQUIRE(actual.as.ipv6.u.byte[0] == 0);
    REQUIRE(actual.as.ipv6.u.byte[1] == 0);
    REQUIRE(actual.as.ipv6.u.byte[2] == 0);
    REQUIRE(actual.as.ipv6.u.byte[3] == 0);
    REQUIRE(actual.as.ipv6.u.byte[4] == 0);
    REQUIRE(actual.as.ipv6.u.byte[5] == 0);
    REQUIRE(actual.as.ipv6.u.byte[6] == 0);
    REQUIRE(actual.as.ipv6.u.byte[7] == 0);
    REQUIRE(actual.as.ipv6.u.byte[8] == 0);
    REQUIRE(actual.as.ipv6.u.byte[9] == 0);
    REQUIRE(actual.as.ipv6.u.byte[10] == 0);
    REQUIRE(actual.as.ipv6.u.byte[11] == 0);
    REQUIRE(actual.as.ipv6.u.byte[12] == 0);
    REQUIRE(actual.as.ipv6.u.byte[13] == 0);
    REQUIRE(actual.as.ipv6.u.byte[14] == 0);
    REQUIRE(actual.as.ipv6.u.byte[15] == 0);

    clarinet_addr expected = clarinet_addr_none;
    expected.family = CLARINET_AF_INET6;

    REQUIRE_THAT(memory(&expected, sizeof(expected)), Equals(memory(&actual, sizeof(actual))));

    REQUIRE(clarinet_addr_is_ipv6(&actual));
    REQUIRE(clarinet_addr_is_any_ipv6(&actual));
    REQUIRE(clarinet_addr_is_any_ip(&actual));
}

TEST_CASE("IPv6 Address Loopback", "[address][ipv6][loopback]")
{
    clarinet_addr actual = clarinet_addr_loopback_ipv6;
    REQUIRE(actual.family == CLARINET_AF_INET6);
    REQUIRE(actual.as.ipv6.flowinfo == 0);
    REQUIRE(actual.as.ipv6.scope_id == 0);
    REQUIRE(actual.as.ipv6.u.dword[0] == 0);
    REQUIRE(actual.as.ipv6.u.dword[1] == 0);
    REQUIRE(actual.as.ipv6.u.dword[2] == 0);
    REQUIRE(actual.as.ipv6.u.word[0] == 0);
    REQUIRE(actual.as.ipv6.u.word[1] == 0);
    REQUIRE(actual.as.ipv6.u.word[2] == 0);
    REQUIRE(actual.as.ipv6.u.word[3] == 0);
    REQUIRE(actual.as.ipv6.u.word[4] == 0);
    REQUIRE(actual.as.ipv6.u.word[5] == 0);
    REQUIRE(actual.as.ipv6.u.word[6] == 0);
    REQUIRE(actual.as.ipv6.u.byte[0] == 0);
    REQUIRE(actual.as.ipv6.u.byte[1] == 0);
    REQUIRE(actual.as.ipv6.u.byte[2] == 0);
    REQUIRE(actual.as.ipv6.u.byte[3] == 0);
    REQUIRE(actual.as.ipv6.u.byte[4] == 0);
    REQUIRE(actual.as.ipv6.u.byte[5] == 0);
    REQUIRE(actual.as.ipv6.u.byte[6] == 0);
    REQUIRE(actual.as.ipv6.u.byte[7] == 0);
    REQUIRE(actual.as.ipv6.u.byte[8] == 0);
    REQUIRE(actual.as.ipv6.u.byte[9] == 0);
    REQUIRE(actual.as.ipv6.u.byte[10] == 0);
    REQUIRE(actual.as.ipv6.u.byte[11] == 0);
    REQUIRE(actual.as.ipv6.u.byte[12] == 0);
    REQUIRE(actual.as.ipv6.u.byte[13] == 0);
    REQUIRE(actual.as.ipv6.u.byte[14] == 0);
    REQUIRE(actual.as.ipv6.u.byte[15] == 1);

    SECTION("Check clarinet_addr_loopback_ipv6 constant")
    {
        clarinet_addr expected = clarinet_addr_none;
        expected.family = CLARINET_AF_INET6;
        expected.as.ipv6.u.byte[15] = 1;

        REQUIRE_THAT(memory(&expected, sizeof(expected)), Equals(memory(&actual, sizeof(clarinet_addr))));
    }

    SECTION("Check ::1 is an ipv6 looback")
    {
        REQUIRE(clarinet_addr_is_ipv6(&actual));
        REQUIRE(clarinet_addr_is_loopback_ipv6(&actual));
        REQUIRE(clarinet_addr_is_loopback_ip(&actual));
    }

    SECTION("Check :: is NOT an ipv6 looback")
    {
        actual.as.ipv6.u.byte[15] = 0;
        REQUIRE_FALSE(clarinet_addr_is_loopback_ipv6(&actual));
    }

    SECTION("Check ::00ff is NOT an ipv6 looback")
    {
        actual.as.ipv6.u.byte[15] = 255;
        REQUIRE_FALSE(clarinet_addr_is_loopback_ipv6(&actual));
    }

    SECTION("Check ::2 is NOT an ipv6 looback")
    {
        actual.as.ipv6.u.byte[15] = 2;
        REQUIRE_FALSE(clarinet_addr_is_loopback_ipv6(&actual));
    }

    SECTION("Check ::00fe is NOT an ipv6 looback")
    {
        actual.as.ipv6.u.byte[15] = 254;
        REQUIRE_FALSE(clarinet_addr_is_loopback_ipv6(&actual));
    }

    SECTION("Check ::ffff:127.0.0.1 is NOT an ipv6 looback")
    {
        actual.as.ipv6.u.byte[10] = 255;
        actual.as.ipv6.u.byte[11] = 255;
        actual.as.ipv6.u.byte[12] = 127;
        actual.as.ipv6.u.byte[13] = 0;
        actual.as.ipv6.u.byte[14] = 0;
        actual.as.ipv6.u.byte[15] = 1;
        REQUIRE_FALSE(clarinet_addr_is_loopback_ipv6(&actual));
    }
}

TEST_CASE("Unspecified Address Is Equal", "[address]")
{
    SECTION("Check None IS EQUAL to None")
    {
        // using two distinct allocations for possibly the same contant on purpose
        int sample;
        clarinet_addr a, b;

        // @formatter:off
        std::tie(sample, a, b) = GENERATE(table<int, clarinet_addr, clarinet_addr>({
            { 0, clarinet_addr_none, clarinet_addr_none }
        }));
        // @formatter:on

        FROM(sample);

        auto addr_is_equal = clarinet_addr_is_equal(&a, &b);
        REQUIRE(addr_is_equal);
    }

    SECTION("Check all but None IS NOT EQUAL to None")
    {
        // using two distinct allocations for the same contant on purpose
        int sample;
        clarinet_addr a, b;

        // @formatter:off
        std::tie(sample, a, b) = GENERATE(table<int, clarinet_addr, clarinet_addr>({
            { 0, clarinet_addr_none, clarinet_addr_any_ipv4 },
            { 1, clarinet_addr_none, clarinet_addr_loopback_ipv4 },
            { 2, clarinet_addr_none, clarinet_addr_broadcast_ipv4 },
        #if CLARINET_ENABLE_IPV6
            { 3, clarinet_addr_none, clarinet_addr_any_ipv6 },
            { 4, clarinet_addr_none, clarinet_addr_loopback_ipv6 },
        #endif
        }));
        // @formatter:on

        FROM(sample);

        auto addr_is_equal = clarinet_addr_is_equal(&a, &b);
        REQUIRE_FALSE(addr_is_equal);
    }
}

TEST_CASE("IPv4 Address Is Equal", "[address][ipv4]")
{
    SECTION("Check IS EQUAL to constant ipv4 addresses")
    {
        // using two distinct allocations for the same contant on purpose
        int sample;
        clarinet_addr a, b;

        // @formatter:off
        std::tie(sample, a, b) = GENERATE(table<int, clarinet_addr, clarinet_addr>({
            { 0, clarinet_addr_any_ipv4,       clarinet_addr_any_ipv4 },
            { 1, clarinet_addr_loopback_ipv4,  clarinet_addr_loopback_ipv4 },
            { 2, clarinet_addr_broadcast_ipv4, clarinet_addr_broadcast_ipv4 },
        }));
        // @formatter:on

        FROM(sample);

        auto addr_is_equal = clarinet_addr_is_equal(&a, &b);
        REQUIRE(addr_is_equal);
    }

    SECTION("Check IS NOT EQUAL to constant ipv4 addresses")
    {
        int sample;
        clarinet_addr a, b;

        // @formatter:off
        std::tie(sample, a, b) = GENERATE(table<int, clarinet_addr, clarinet_addr>({
            { 0, clarinet_addr_any_ipv4,      clarinet_addr_loopback_ipv4 },
            { 1, clarinet_addr_any_ipv4,      clarinet_addr_broadcast_ipv4 },
            { 2, clarinet_addr_loopback_ipv4, clarinet_addr_broadcast_ipv4 }
        }));
        // @formatter:on

        FROM(sample);

        auto addr_is_equal = clarinet_addr_is_equal(&a, &b);
        REQUIRE_FALSE(addr_is_equal);
    }

    SECTION("Check IS EQUAL to custom ipv4 addresses")
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

        auto addr_is_equal = clarinet_addr_is_equal(&a, &b);
        REQUIRE(addr_is_equal);
    }

    SECTION("Check IS NOT EQUAL to custom ipv4 addresses")
    {
        SECTION("Check SAME ipv4 address but one has family UNSPEC")
        {
            clarinet_addr a;
            memnoise(&a, sizeof(a));
            a.family = CLARINET_AF_UNSPEC;
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

            auto addr_is_equal = clarinet_addr_is_equal(&a, &b);
            REQUIRE_FALSE(addr_is_equal);
        }

        SECTION("Check SAME ipv4 address but one is an IPv6 mapped to IPv6 ")
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

            auto addr_is_equal = clarinet_addr_is_equal(&a, &b);
            REQUIRE_FALSE(addr_is_equal);
        }
    }
}

TEST_CASE("IPv6 Address Is Equal", "[address][ipv6]")
{
    SECTION("Check IS EQUAL to constant ipv6 addresses")
    {
        // using two distinct allocations for the same contant on purpose
        int sample;
        clarinet_addr a, b;

        // @formatter:off
        std::tie(sample, a, b) = GENERATE(table<int, clarinet_addr, clarinet_addr>({
            { 0, clarinet_addr_any_ipv6,            clarinet_addr_any_ipv6 },
            { 1, clarinet_addr_loopback_ipv6,       clarinet_addr_loopback_ipv6 },
            { 2, clarinet_addr_loopback_ipv4mapped, clarinet_addr_loopback_ipv4mapped}
        }));
        // @formatter:on

        FROM(sample);

        auto addr_is_equal = clarinet_addr_is_equal(&a, &b);
        REQUIRE(addr_is_equal);
    }

    SECTION("Check IS NOT EQUAL to constant ipv6 addresses")
    {
        int sample;
        clarinet_addr a, b;

        // @formatter:off
        std::tie(sample, a, b) = GENERATE(table<int, clarinet_addr, clarinet_addr>({
            { 0, clarinet_addr_any_ipv6,      clarinet_addr_any_ipv4 },
            { 1, clarinet_addr_loopback_ipv6, clarinet_addr_loopback_ipv4 },
            { 2, clarinet_addr_any_ipv6,      clarinet_addr_loopback_ipv6 },
            { 3, clarinet_addr_any_ipv6,      clarinet_addr_loopback_ipv4mapped },
            { 4, clarinet_addr_loopback_ipv6, clarinet_addr_loopback_ipv4mapped }
        }));
        // @formatter:on

        FROM(sample);

        auto addr_is_equal = clarinet_addr_is_equal(&a, &b);
        REQUIRE_FALSE(addr_is_equal);
    }

    SECTION("Check IS EQUAL to custom ipv6 addresses")
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

        auto addr_is_equal = clarinet_addr_is_equal(&a, &b);
        REQUIRE(addr_is_equal);

        clarinet_addr c = b;
        c.as.ipv6.flowinfo = 0;

        addr_is_equal = clarinet_addr_is_equal(&b, &c);
        REQUIRE(addr_is_equal);
    }

    SECTION("Check IS NOT EQUAL to custom ipv6 addresses")
    {
        SECTION("Check SAME ipv6 address but one has family UNSPEC")
        {
            clarinet_addr a;
            memnoise(&a, sizeof(a));
            a.family = CLARINET_AF_UNSPEC;
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

            auto addr_is_equal = clarinet_addr_is_equal(&a, &b);
            REQUIRE_FALSE(addr_is_equal);
        }

        SECTION("Check SAME ipv6 address but one has family INET")
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

            auto addr_is_equal = clarinet_addr_is_equal(&a, &b);
            REQUIRE_FALSE(addr_is_equal);
        }

        SECTION("Check SAME ipv6 address but different flow info")
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

            auto addr_is_equal = clarinet_addr_is_equal(&a, &b);
            REQUIRE(addr_is_equal);
        }

        SECTION("Check SAME ipv6 address but different scope id")
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

            auto addr_is_equal = clarinet_addr_is_equal(&a, &b);
            REQUIRE_FALSE(addr_is_equal);
        }
    }
}

TEST_CASE("IPv4 Address Is Equivalent", "[address][ipv4]")
{
    SECTION("Check IS EQUIVALENT to constant ipv4 addresses")
    {
        // using two distinct allocations for the same contant on purpose
        int sample;
        clarinet_addr a, b;

        // @formatter:off
        std::tie(sample, a, b) = GENERATE(table<int, clarinet_addr, clarinet_addr>({
            { 0, clarinet_addr_any_ipv4,       clarinet_addr_any_ipv4 },
            { 1, clarinet_addr_loopback_ipv4,  clarinet_addr_loopback_ipv4 },
            { 2, clarinet_addr_broadcast_ipv4, clarinet_addr_broadcast_ipv4 }
        }));
        // @formatter:on

        FROM(sample);

        auto addr_is_equivalent = clarinet_addr_is_equivalent(&a, &b);
        REQUIRE(addr_is_equivalent);
    }

    SECTION("Check IS NOT EQUIVALENT to constant ipv4 addresses")
    {
        int sample;
        clarinet_addr a, b;

        // @formatter:off
        std::tie(sample, a, b) = GENERATE(table<int, clarinet_addr, clarinet_addr>({
            { 0, clarinet_addr_any_ipv4,      clarinet_addr_loopback_ipv4 },
            { 1, clarinet_addr_any_ipv4,      clarinet_addr_broadcast_ipv4 },
            { 2, clarinet_addr_loopback_ipv4, clarinet_addr_broadcast_ipv4 }
        }));
        // @formatter:on

        FROM(sample);

        auto addr_is_equivalent = clarinet_addr_is_equivalent(&a, &b);
        REQUIRE_FALSE(addr_is_equivalent);
    }

    SECTION("Check IS EQUIVALENT to custom ipv4 addresses")
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

        auto addr_is_equivalent = clarinet_addr_is_equivalent(&a, &b);
        REQUIRE(addr_is_equivalent);
    }
}

TEST_CASE("IPv6 Address Is Equivalent", "[address][ipv6]")
{
    SECTION("Check IS EQUIVALENT to constant ipv6 addresses")
    {
        // using two distinct allocations for the same contant on purpose
        int sample;
        clarinet_addr a, b;

        // @formatter:off
        std::tie(sample, a, b) = GENERATE(table<int, clarinet_addr, clarinet_addr>({
            { 0, clarinet_addr_any_ipv6,            clarinet_addr_any_ipv6 },
            { 1, clarinet_addr_loopback_ipv6,       clarinet_addr_loopback_ipv6  },
            { 2, clarinet_addr_loopback_ipv4mapped, clarinet_addr_loopback_ipv4mapped  },
            { 3, clarinet_addr_loopback_ipv4,       clarinet_addr_loopback_ipv4mapped  }
        }));
        // @formatter:on

        FROM(sample);

        auto addr_is_equivalent = clarinet_addr_is_equivalent(&a, &b);
        REQUIRE(addr_is_equivalent);
    }

    SECTION("Check IS NOT EQUIVALENT to constant ipv6 addresses")
    {
        int sample;
        clarinet_addr a, b;

        // @formatter:off
        std::tie(sample, a, b) = GENERATE(table<int, clarinet_addr, clarinet_addr>({
            { 0, clarinet_addr_any_ipv6,      clarinet_addr_loopback_ipv6 },
            { 1, clarinet_addr_any_ipv6,      clarinet_addr_loopback_ipv4mapped },
            { 2, clarinet_addr_loopback_ipv6, clarinet_addr_loopback_ipv4mapped }
        }));
        // @formatter:on

        FROM(sample);

        auto addr_is_equivalent = clarinet_addr_is_equivalent(&a, &b);
        REQUIRE_FALSE(addr_is_equivalent);
    }

    SECTION("Check IS EQUIVALENT to custom ipv6 addresses")
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

            auto addr_is_equivalent = clarinet_addr_is_equivalent(&a, &b);
            REQUIRE(addr_is_equivalent);

            clarinet_addr c = b;
            c.as.ipv6.flowinfo = 0;
            addr_is_equivalent = clarinet_addr_is_equivalent(&b, &c);
            REQUIRE(addr_is_equivalent);
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

            auto addr_is_equivalent = clarinet_addr_is_equivalent(&a, &b);
            REQUIRE(addr_is_equivalent);
        }
    }

    SECTION("Check IS NOT EQUIVALENT to custom ipv6 addresses")
    {
        SECTION("Check SAME ipv6 address but one has family UNSPEC")
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

            auto addr_is_equivalent = clarinet_addr_is_equivalent(&a, &b);
            REQUIRE_FALSE(addr_is_equivalent);
        }

        SECTION("Check SAME ipv6 address but different flow info")
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

            auto addr_is_equivalent = clarinet_addr_is_equivalent(&a, &b);
            REQUIRE(addr_is_equivalent);
        }

        SECTION("Check SAME ipv6 address but different scope id")
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

            auto addr_is_equivalent = clarinet_addr_is_equivalent(&a, &b);
            REQUIRE_FALSE(addr_is_equivalent);
        }
    }
}

TEST_CASE("Convert to IPv4", "[address][ipv4]")
{
    SECTION("VALID conversion to IPv4")
    {
        // @formatter:off
        const std::vector<std::tuple<int, clarinet_addr, clarinet_addr>> data =
        {
        #if CLARINET_ENABLE_IPV6
            { 0, clarinet_addr_loopback_ipv4mapped, clarinet_addr_loopback_ipv4 },
        #endif // CLARINET_ENABLE_IPV6
            { 1, clarinet_addr_loopback_ipv4,       clarinet_addr_loopback_ipv4 }
        };
        // @formatter:on

        SAMPLES(data);

        int sample;
        clarinet_addr src, expected;
        std::tie(sample, src, expected) = GENERATE_REF(from_samples(data));

        FROM(sample);

        clarinet_addr dst;
        const int errcode = clarinet_addr_convert_to_ipv4(&dst, &src);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        auto addr_is_equal = clarinet_addr_is_equal(&dst, &expected);
        REQUIRE(addr_is_equal);
    }

    SECTION("INVALID conversion To IPv4")
    {
        // @formatter:off
        const std::vector<std::tuple<int, clarinet_addr>> data = {
        #if !CLARINET_ENABLE_IPV6
            { 0, clarinet_addr_loopback_ipv6       },
            { 1, clarinet_addr_loopback_ipv4mapped },
        #endif // !CLARINET_ENABLE_IPV6
            { 2, clarinet_addr_none                }
        };
        // @formatter:on

        SAMPLES(data);

        int sample;
        clarinet_addr src;
        std::tie(sample, src) = GENERATE_REF(from_samples(data));

        FROM(sample);

        clarinet_addr dst;
        const int errcode = clarinet_addr_convert_to_ipv4(&dst, &src);
        REQUIRE(Error(errcode) == Error(CLARINET_EAFNOSUPPORT));
    }
}

TEST_CASE("Convert to IPv6", "[address][ipv6]")
{
    SECTION("VALID conversion to IPv6")
    {
        #if CLARINET_ENABLE_IPV6

        int sample;
        clarinet_addr src, expected;

        // @formatter:off
        std::tie(sample, src, expected) = GENERATE(table<int, clarinet_addr, clarinet_addr>({
            { 0, clarinet_addr_loopback_ipv4, clarinet_addr_loopback_ipv4mapped },
            { 1, clarinet_addr_loopback_ipv6, clarinet_addr_loopback_ipv6 }
        }));
        // @formatter:on

        FROM(sample);

        clarinet_addr dst;
        const int errcode = clarinet_addr_convert_to_ipv6(&dst, &src);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        auto addr_is_equal = clarinet_addr_is_equal(&dst, &expected);
        REQUIRE(addr_is_equal);

        #endif // CLARINET_ENABLE_IPV6
    }

    SECTION("INVALID conversion To IPv6")
    {
        // @formatter:off
        const std::vector<std::tuple<int, clarinet_addr>> data = {
        #if !CLARINET_ENABLE_IPV6
            { 1, clarinet_addr_loopback_ipv4mapped },
            { 2, clarinet_addr_loopback_ipv6 },
        #endif // !CLARINET_ENABLE_IPV6
            { 3, clarinet_addr_none }
        };
        // @formatter:on

        SAMPLES(data);

        int sample;
        clarinet_addr src;
        std::tie(sample, src) = GENERATE_REF(from_samples(data));

        FROM(sample);

        clarinet_addr dst;
        const int errcode = clarinet_addr_convert_to_ipv6(&dst, &src);
        REQUIRE_FALSE(Error(errcode) == Error(CLARINET_EAFNOSUPPORT));
    }
}

TEST_CASE("Address To String", "[address]")
{
    SECTION("With INVALID arguments")
    {
        SECTION("NULL dst")
        {
            clarinet_addr src = clarinet_addr_any_ipv4;
            const int errcode = clarinet_addr_to_string(nullptr, 0, &src);
            REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
        }

        SECTION("NULL src")
        {
            char dst[CLARINET_ADDR_STRLEN];
            const int errcode = clarinet_addr_to_string(dst, sizeof(dst), nullptr);
            REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
        }

        SECTION("Not enough space on dst for ipv4")
        {
            clarinet_addr src = clarinet_addr_any_ipv4;
            char dst[4];
            const int errcode = clarinet_addr_to_string(dst, sizeof(dst), &src);
            REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
        }

        SECTION("Not enough space on dst for ipv6")
        {
            clarinet_addr src = { 0 };
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
            char dst[1] = { 0 };
            memnoise(dst, sizeof(dst));
            const int errcode = clarinet_addr_to_string(dst, sizeof(dst), &src);
            REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
        }

        SECTION("Family not supported")
        {
            clarinet_addr custom_ipv6 = { 0 };
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
            custom_none.family = CLARINET_AF_UNSPEC;

            // @formatter:off
            const std::vector<std::tuple<int, clarinet_addr>> data = {
            #if !CLARINET_ENABLE_IPV6
                { 0, custom_ipv6 },
            #endif // !CLARINET_ENABLE_IPV6
                { 1, custom_none }
            };
            // @formatter:on

            SAMPLES(data);

            int sample;
            clarinet_addr src;
            std::tie(sample, src) = GENERATE_REF(from_samples(data));

            FROM(sample);

            char dst[CLARINET_ADDR_STRLEN] = { 0 };
            memnoise(dst, sizeof(dst));
            const int errcode = clarinet_addr_to_string(dst, sizeof(dst), &src);
            REQUIRE(Error(errcode) == Error(CLARINET_EAFNOSUPPORT));
        }
    }

    SECTION("Wiht VALID arguments")
    {
        SECTION("IPv4 to string")
        {
            int sample;
            clarinet_addr src;
            const char* expected;

            // @formatter:off
            std::tie(sample, src, expected) = GENERATE(table<int, clarinet_addr, const char*>({
                { 0, clarinet_addr_any_ipv4,       "0.0.0.0" },
                { 1, clarinet_addr_loopback_ipv4,  "127.0.0.1" },
                { 2, clarinet_addr_broadcast_ipv4, "255.255.255.255" }
            }));
            // @formatter:on

            FROM(sample);

            char dst[CLARINET_ADDR_STRLEN] = { 0 };
            memnoise(dst, sizeof(dst));
            const int n = clarinet_addr_to_string(dst, sizeof(dst), &src);
            REQUIRE(n > 0);
            EXPLAIN("Expected: '%s'", expected);
            EXPLAIN("Actual: '%s'", dst);
            REQUIRE((size_t)n == strlen(expected));
            REQUIRE_THAT(std::string(dst), Equals(expected));
        }

        SECTION("IPv6 to string")
        {
            #if CLARINET_ENABLE_IPV6

            clarinet_addr ipv4mapped_with_scope_id = clarinet_addr_loopback_ipv4mapped;
            ipv4mapped_with_scope_id.as.ipv6.scope_id = 1234567890;

            clarinet_addr custom_ipv6 = { 0 };
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

            int sample;
            clarinet_addr src;
            const char* expected;

            // @formatter:off
            std::tie(sample, src, expected) = GENERATE_REF(table<int, clarinet_addr, const char*>({
                {0, clarinet_addr_any_ipv6,            "::" },
                {1, clarinet_addr_loopback_ipv6,       "::1" },
                {2, clarinet_addr_loopback_ipv4mapped, "::ffff:127.0.0.1" },
                {3, ipv4mapped_with_scope_id,          "::ffff:127.0.0.1%1234567890" },
                {4, custom_ipv6,                       "b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779" }
            }));
            // @formatter:on

            FROM(sample);

            char dst[CLARINET_ADDR_STRLEN] = { 0 };
            memnoise(dst, sizeof(dst));
            const int n = clarinet_addr_to_string(dst, sizeof(dst), &src);
            REQUIRE(n > 0);
            EXPLAIN("Expected: '%s'", expected);
            EXPLAIN("Actual: '%s'", dst);
            REQUIRE((size_t)n == strlen(expected));
            REQUIRE_THAT(std::string(dst), Equals(expected));

            #endif // CLARINET_ENABLE_IPV6
        }
    }
}

TEST_CASE("Address From String", "[address]")
{
    SECTION("With NULL dst")
    {
        const char* src = "192.168.0.1";
        const int errcode = clarinet_addr_from_string(nullptr, src, strlen(src));
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
    }

    SECTION("With NULL src")
    {
        clarinet_addr dst = { 0 };
        const int errcode = clarinet_addr_from_string(&dst, nullptr, 0);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
    }

    SECTION("With Zero srclen")
    {
        clarinet_addr dst = { 0 };
        const char* src = "192.168.0.1";
        const int errcode = clarinet_addr_from_string(&dst, src, 0);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
    }

    SECTION("With INSUFFICIENT srclen")
    {
        clarinet_addr dst = { 0 };
        const char* src = "192.168.0.1";
        const int errcode = clarinet_addr_from_string(&dst, src, strlen(src) >> 1);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
    }

    SECTION("With INVALID addresses")
    {
        const std::vector<std::tuple<int, const char*>> data = {
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
            #if !CLARINET_ENABLE_IPV6
            { 12, "::ffff:127.0.0.1" },
            { 13, "::ffff:127.0.0.1%1234567890" },
            { 14, "b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf" },
            { 15, "b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebfK" },
            { 16, "b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779" },
            #endif // !CLARINET_ENABLE_IPV6
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
            { 30, "0000:0000:0000:0000:0000:ffff:127.0.0.001" },
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

        SAMPLES(data);

        int sample;
        const char* src;
        std::tie(sample, src) = GENERATE_REF(from_samples(data));

        FROM(sample);

        clarinet_addr dst = { 0 };
        memnoise(&dst, sizeof(dst));
        const int errcode = clarinet_addr_from_string(&dst, src, strlen(src));
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
    }

    SECTION("With VALID addresses")
    {
        #if CLARINET_ENABLE_IPV6

        clarinet_addr ipv4mapped_with_scope_id = clarinet_addr_loopback_ipv4mapped;
        ipv4mapped_with_scope_id.as.ipv6.scope_id = 1234567890;

        clarinet_addr custom_ipv6 = { 0 };
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

        // flowinfo is not compared
        clarinet_addr custom_ipv6_with_flowinfo = custom_ipv6;
        custom_ipv6_with_flowinfo.as.ipv6.flowinfo = 0xFF;

        // leading zeros ignored
        clarinet_addr custom_ipv6_with_short_scope_id = custom_ipv6;
        custom_ipv6_with_short_scope_id.as.ipv6.scope_id = 12345;

        clarinet_addr custom_ipv6_f = clarinet_addr_any_ipv6;
        custom_ipv6_f.as.ipv6.u.byte[15] = 0x0F;

        clarinet_addr custom_ipv6_feef_1886 = clarinet_addr_any_ipv6;
        custom_ipv6_feef_1886.as.ipv6.u.byte[12] = 0xFE;
        custom_ipv6_feef_1886.as.ipv6.u.byte[13] = 0xEF;
        custom_ipv6_feef_1886.as.ipv6.u.byte[14] = 0x18;
        custom_ipv6_feef_1886.as.ipv6.u.byte[15] = 0x86;

        clarinet_addr custom_ipv6_ffff_127_0_0_10 = clarinet_addr_loopback_ipv4mapped;
        custom_ipv6_ffff_127_0_0_10.as.ipv6.u.byte[15] = 10;

        #endif // CLARINET_ENABLE_IPV6

        // @formatter:off
        const std::vector<std::tuple<int, const char*, const char*, clarinet_addr>> data = {
            {  0, "0.0.0.0",                                            "0.0.0.0",                                            clarinet_addr_any_ipv4            },
            {  1, "127.0.0.1",                                          "127.0.0.1",                                          clarinet_addr_loopback_ipv4       },
            {  2, "255.255.255.255",                                    "255.255.255.255",                                    clarinet_addr_broadcast_ipv4      },
        #if CLARINET_ENABLE_IPV6
            {  3, "::",                                                 "::",                                                 clarinet_addr_any_ipv6            },
            {  4, "0000:0000:0000:0000:0000:0000:0000:0000",            "::",                                                 clarinet_addr_any_ipv6            },
            {  5, "::1",                                                "::1",                                                clarinet_addr_loopback_ipv6       },
            {  6, "::ffff:127.0.0.1",                                   "::ffff:127.0.0.1",                                   clarinet_addr_loopback_ipv4mapped },
            {  7, "0000:0000:0000:0000:0000:ffff:127.0.0.1%0",          "::ffff:127.0.0.1",                                   clarinet_addr_loopback_ipv4mapped },
            {  8, "::ffff:127.0.0.1%1234567890",                        "::ffff:127.0.0.1%1234567890",                        ipv4mapped_with_scope_id          },
            {  9, "b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779", "b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779", custom_ipv6                       },
            { 10, "b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779", "b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779", custom_ipv6_with_flowinfo         },
            { 11, "b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%12345",      "b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%12345",      custom_ipv6_with_short_scope_id   },
            { 12, "::F",                                                "::f",                                                custom_ipv6_f                     },
            { 13, "::FEEF:1886",                                        "::254.239.24.134",                                   custom_ipv6_feef_1886             }, // CHECK: shouldn't ::FEEF:1886 be reconstructed as ::feef:1886 instead of ::254.239.24.134 ?
            { 14, "0000:0000:0000:0000:0000:ffff:127.0.0.10",           "::ffff:127.0.0.10",                                  custom_ipv6_ffff_127_0_0_10       },
        #endif // CLARINET_ENABLE_IPV6
        };
        // @formatter:on

        SAMPLES(data);

        int sample;
        const char* src;
        const char* reconstruction;
        clarinet_addr expected;
        std::tie(sample, src, reconstruction, expected) = GENERATE_REF(from_samples(data));

        FROM(sample);

        clarinet_addr dst = { 0 };
        memnoise(&dst, sizeof(dst));
        const int errcode = clarinet_addr_from_string(&dst, src, strlen(src));
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        auto addr_is_equal = clarinet_addr_is_equal(&dst, &expected);
        REQUIRE(addr_is_equal);

        char s[CLARINET_ADDR_STRLEN] = { 0 };
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
    REQUIRE(sizeof(((clarinet_endpoint*)nullptr)->addr) == sizeof(clarinet_addr));
    REQUIRE(sizeof(((clarinet_endpoint*)nullptr)->port) == sizeof(uint16_t));
}

TEST_CASE("Endpoint Max Strlen", "[endpoint]")
{
    REQUIRE(CLARINET_ENDPOINT_STRLEN == 65); // "[0000:0000:0000:0000:0000:ffff:255.255.255.255%4294967295]:65535"
}

TEST_CASE("Endpoint To String", "[endpoint]")
{
    SECTION("With INVALID arguments")
    {
        SECTION("With NULL dst")
        {
            clarinet_endpoint src = { clarinet_addr_any_ipv4, 0 };
            const int errcode = clarinet_endpoint_to_string(nullptr, 0, &src);
            REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
        }

        SECTION("With NULL src")
        {
            char dst[CLARINET_ENDPOINT_STRLEN];
            const int errcode = clarinet_endpoint_to_string(dst, sizeof(dst), nullptr);
            REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
        }

        SECTION("With INSUFFICIENT space on dst for ipv4")
        {
            clarinet_endpoint src = { clarinet_addr_any_ipv4, 0 };
            char dst[4];
            const int errcode = clarinet_endpoint_to_string(dst, sizeof(dst), &src);
            REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
        }

        SECTION("With INSUFFICIENT space on dst for ipv6")
        {
            clarinet_endpoint src = { { 0 } };
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

            char dst[1] = { 0 };
            memnoise(dst, sizeof(dst));
            const int errcode = clarinet_endpoint_to_string(dst, sizeof(dst), &src);
            REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
        }

        SECTION("With UNSUPPORTED family")
        {
            clarinet_endpoint custom_ipv6 = { { 0 } };
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
            custom_none.addr.family = CLARINET_AF_UNSPEC;

            const std::vector<std::tuple<int, clarinet_endpoint>> data = {
            #if !CLARINET_ENABLE_IPV6
                { 0, custom_ipv6 },
            #endif // !CLARINET_ENABLE_IPV6
                { 1, custom_none }
            };

            SAMPLES(data);

            int sample;
            clarinet_endpoint src;
            std::tie(sample, src) = GENERATE_REF(from_samples(data));

            FROM(sample);

            char dst[CLARINET_ENDPOINT_STRLEN] = { 0 };
            memnoise(dst, sizeof(dst));
            const int errcode = clarinet_endpoint_to_string(dst, sizeof(dst), &src);
            REQUIRE(Error(errcode) == Error(CLARINET_EAFNOSUPPORT));
        }
    }

    SECTION("With VALID arguments")
    {
        SECTION("IPv4 to string")
        {
            int sample;
            clarinet_endpoint src;
            const char* expected;

            // @formatter:off
            std::tie(sample, src, expected) = GENERATE(table<int, clarinet_endpoint, const char*>({
                { 0, {clarinet_addr_any_ipv4,           0 }, "0.0.0.0:0"             },
                { 1, {clarinet_addr_loopback_ipv4,      0 }, "127.0.0.1:0"           },
                { 2, {clarinet_addr_broadcast_ipv4,     0 }, "255.255.255.255:0"     },
                { 3, {clarinet_addr_any_ipv4,       65535 }, "0.0.0.0:65535"         },
                { 4, {clarinet_addr_loopback_ipv4,  65535 }, "127.0.0.1:65535"       },
                { 5, {clarinet_addr_broadcast_ipv4, 65535 }, "255.255.255.255:65535" },
            }));
            // @formatter:on

            FROM(sample);

            char dst[CLARINET_ENDPOINT_STRLEN] = { 0 };
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
            #if CLARINET_ENABLE_IPV6

            clarinet_addr ipv4mapped_with_scope_id = clarinet_addr_loopback_ipv4mapped;
            ipv4mapped_with_scope_id.as.ipv6.scope_id = 1234567890;

            clarinet_addr custom_ipv6 = { 0 };
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

            int sample;
            clarinet_endpoint src;
            const char* expected;

            // @formatter:off
            std::tie(sample, src, expected) = GENERATE_REF(table<int, clarinet_endpoint, const char*>({
                { 0, {clarinet_addr_any_ipv6,                0 }, "[::]:0" },
                { 1, {clarinet_addr_loopback_ipv6,           0 }, "[::1]:0" },
                { 2, {clarinet_addr_loopback_ipv4mapped,     0 }, "[::ffff:127.0.0.1]:0" },
                { 3, {ipv4mapped_with_scope_id,              0 }, "[::ffff:127.0.0.1%1234567890]:0" },
                { 4, {custom_ipv6,                           0 }, "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779]:0" },
                { 5, {clarinet_addr_any_ipv6,            65535 }, "[::]:65535" },
                { 6, {clarinet_addr_loopback_ipv6,       65535 }, "[::1]:65535" },
                { 7, {clarinet_addr_loopback_ipv4mapped, 65535 }, "[::ffff:127.0.0.1]:65535" },
                { 8, {ipv4mapped_with_scope_id,          65535 }, "[::ffff:127.0.0.1%1234567890]:65535" },
                { 9, {custom_ipv6,                       65535 }, "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779]:65535" }
            }));
            // @formatter:on

            FROM(sample);

            char dst[CLARINET_ENDPOINT_STRLEN] = { 0 };
            memnoise(dst, sizeof(dst));
            const int n = clarinet_endpoint_to_string(dst, sizeof(dst), &src);
            REQUIRE(n > 0);
            EXPLAIN("Expected: '%s'", expected);
            EXPLAIN("Actual: '%s'", dst);
            REQUIRE((size_t)n == strlen(expected));
            REQUIRE_THAT(std::string(dst), Equals(expected));

            #endif // CLARINET_ENABLE_IPV6
        }
    }
}

TEST_CASE("Endpoint From string", "[endpoint]")
{
    SECTION("With NULL dst")
    {
        const char* src = "192.168.0.1:0";
        const int errcode = clarinet_endpoint_from_string(nullptr, src, strlen(src));
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
    }

    SECTION("With NULL src")
    {
        clarinet_endpoint dst = { { 0 } };
        const int errcode = clarinet_endpoint_from_string(&dst, nullptr, 0);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
    }

    SECTION("With Zero srclen")
    {
        clarinet_endpoint dst = { { 0 } };
        const char* src = "192.168.0.1:0";
        const int errcode = clarinet_endpoint_from_string(&dst, src, 0);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
    }

    SECTION("With INSUFFICIENT space on src")
    {
        clarinet_endpoint dst = { { 0 } };
        const char* src = "192.168.0.1:0";
        const int errcode = clarinet_endpoint_from_string(&dst, src, strlen(src) >> 1);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
    }

    SECTION("With INVALID endpoints")
    {
        const std::vector<std::tuple<int, const char*>> data = {
            { 0, "" },
            { 1, "0" },
            { 2, "0." },
            { 3, "0.0" },
            { 4, "0.0.0" },
            { 5, "0.0.0.0" },
            { 6, "0.0.0.0:" },
            { 7, "0.0.0.0::13" },
            { 8, "127.0.0.1" },
            { 9, "127.0.0.001" },
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
            #if !CLARINET_ENABLE_IPV6
            { 25, "[::ffff:127.0.0.1]:1234" },
            { 26, "[::ffff:127.0.0.1%1234567890]:1234" },
            { 27, "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf]:1234" },
            { 28, "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779]:1234" },
            #endif // !CLARINET_ENABLE_IPV6
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
            { 42, "::ffff:127.0.0.1%1234567890:1234" },
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

        SAMPLES(data);

        int sample;
        const char* src;
        std::tie(sample, src) = GENERATE_REF(from_samples(data));

        FROM(sample);

        clarinet_endpoint dst = { { 0 } };
        memnoise(&dst, sizeof(dst));
        const int errcode = clarinet_endpoint_from_string(&dst, src, strlen(src));
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
    }

    SECTION("With VALID endpoints")
    {
        #if CLARINET_ENABLE_IPV6

        clarinet_addr ipv4mapped_with_scope_id = clarinet_addr_loopback_ipv4mapped;
        ipv4mapped_with_scope_id.as.ipv6.scope_id = 1234567890;

        clarinet_addr ipv4mapped_with_short_scope_id = clarinet_addr_loopback_ipv4mapped;
        ipv4mapped_with_short_scope_id.as.ipv6.scope_id = 12345;

        clarinet_addr custom_ipv6 = { 0 };
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

        // flowinfo is not compared
        clarinet_addr custom_ipv6_with_flowinfo = custom_ipv6;
        custom_ipv6_with_flowinfo.as.ipv6.flowinfo = 0xFF;

        #endif // CLARINET_ENABLE_IPV6

        // @formatter:off
        const std::vector<std::tuple<int, const char*, const char*, clarinet_endpoint>> data = {
            {  0, "0.0.0.0:0",                                                  "0.0.0.0:0",                                                  { clarinet_addr_any_ipv4,                0 } },
            {  1, "0.0.0.0:1234",                                               "0.0.0.0:1234",                                               { clarinet_addr_any_ipv4,             1234 } },
            {  2, "127.0.0.1:0",                                                "127.0.0.1:0",                                                { clarinet_addr_loopback_ipv4,           0 } },
            {  3, "127.0.0.1:65535",                                            "127.0.0.1:65535",                                            { clarinet_addr_loopback_ipv4,       65535 } },
            {  4, "255.255.255.255:9",                                          "255.255.255.255:9",                                          { clarinet_addr_broadcast_ipv4,          9 } },
        #if CLARINET_ENABLE_IPV6
            {  5, "[::]:0",                                                     "[::]:0",                                                     { clarinet_addr_any_ipv6,                0 } },
            {  6, "[::]:1234",                                                  "[::]:1234",                                                  { clarinet_addr_any_ipv6,             1234 } },
            {  7, "[::1]:0",                                                    "[::1]:0",                                                    { clarinet_addr_loopback_ipv6,           0 } },
            {  8, "[::1]:65535",                                                "[::1]:65535",                                                { clarinet_addr_loopback_ipv6,       65535 } },
            { 19, "[::ffff:127.0.0.1]:0",                                       "[::ffff:127.0.0.1]:0",                                       { clarinet_addr_loopback_ipv4mapped,     0 } },
            { 20, "[0000:0000:0000:0000:0000:ffff:127.0.0.1%0]:0",              "[::ffff:127.0.0.1]:0",                                       { clarinet_addr_loopback_ipv4mapped,     0 } },
            { 21, "[::ffff:127.0.0.1]:65535",                                   "[::ffff:127.0.0.1]:65535",                                   { clarinet_addr_loopback_ipv4mapped, 65535 } },
            { 22, "[::ffff:127.0.0.1%12345]:8",                                 "[::ffff:127.0.0.1%12345]:8",                                 { ipv4mapped_with_short_scope_id,        8 } },
            { 23, "[::ffff:127.0.0.1%1234567890]:65535",                        "[::ffff:127.0.0.1%1234567890]:65535",                        { ipv4mapped_with_scope_id,          65535 } },
            { 24, "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779]:0",     "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779]:0",     { custom_ipv6,                           0 } },
            { 25, "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779]:1",     "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779]:1",     { custom_ipv6,                           1 } },
            { 26, "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779]:65535", "[b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf%3233923779]:65535", { custom_ipv6_with_flowinfo,         65535 } },
        #endif // CLARINET_ENABLE_IPV6
        };
        // @formatter:on

        SAMPLES(data);

        int sample;
        const char* src;
        const char* reconstruction;
        clarinet_endpoint expected;
        std::tie(sample, src, reconstruction, expected) = GENERATE_REF(from_samples(data));

        FROM(sample);

        clarinet_endpoint dst = { { 0 } };
        memnoise(&dst, sizeof(dst));
        const int errcode = clarinet_endpoint_from_string(&dst, src, strlen(src));
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        auto endpoint_is_equal = clarinet_endpoint_is_equal(&dst, &expected);
        REQUIRE(endpoint_is_equal);

        char s[CLARINET_ENDPOINT_STRLEN] = { 0 };
        memnoise(&s, sizeof(s));
        const int n = clarinet_endpoint_to_string(s, sizeof(s), &dst);
        REQUIRE(n > 0);
        EXPLAIN("Expected: %s", reconstruction);
        EXPLAIN("Actual: %s", s);
        REQUIRE((size_t)n == strlen(reconstruction));
        REQUIRE_THAT(std::string(s), Equals(reconstruction));
    }
}
