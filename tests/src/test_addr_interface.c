#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <string.h>

#include "clarinet/clarinet.h"

#if defined(HAVE_CONFIG_H)
#include "config.h" 
#endif
#include "cmocka.h"

static void test_clarinet_addr_size(void** state)
{
    CLARINET_IGNORE(state);

    assert_int_equal(28, sizeof(clarinet_addr));

    assert_int_equal(sizeof(uint16_t), sizeof(((clarinet_addr*)0)->family));
    assert_int_equal(sizeof(uint8_t), sizeof(((clarinet_addr*)0)->as.ipv4.u.byte[0]));
    assert_int_equal(sizeof(uint16_t), sizeof(((clarinet_addr*)0)->as.ipv4.u.word[0]));
    assert_int_equal(sizeof(uint32_t), sizeof(((clarinet_addr*)0)->as.ipv4.u.dword[0]));

    assert_int_equal(sizeof(uint8_t), sizeof(((clarinet_addr*)0)->as.ipv6.u.byte[0]));
    assert_int_equal(sizeof(uint16_t), sizeof(((clarinet_addr*)0)->as.ipv6.u.word[0]));
    assert_int_equal(sizeof(uint32_t), sizeof(((clarinet_addr*)0)->as.ipv6.u.dword[0]));
    
    assert_int_equal(sizeof(uint32_t), sizeof(((clarinet_addr*)0)->as.ipv6.flowinfo));
    assert_int_equal(sizeof(uint32_t), sizeof(((clarinet_addr*)0)->as.ipv6.scope_id));

    assert_int_equal(20, sizeof(((clarinet_addr*)0)->as.ipv4));
    assert_int_equal(24, sizeof(((clarinet_addr*)0)->as.ipv6));
}

static void test_clarinet_addr_ipv4_any(void **state)
{
    CLARINET_IGNORE(state);

    clarinet_addr actual = CLARINET_ADDR_IPV4_ANY;
    assert_int_equal(CLARINET_AF_INET, actual.family);
    assert_int_equal(0, actual.as.ipv4.u.dword[0]);
    assert_int_equal(0, actual.as.ipv4.u.word[0] 
                      | actual.as.ipv4.u.word[1]);
    assert_int_equal(0, actual.as.ipv4.u.byte[0] 
                      | actual.as.ipv4.u.byte[1]
                      | actual.as.ipv4.u.byte[2]
                      | actual.as.ipv4.u.byte[3]);

    clarinet_addr expected;
    memset(&expected, 0, sizeof(clarinet_addr));
    expected.family = CLARINET_AF_INET;
    
    assert_memory_equal(&expected, &actual, sizeof(clarinet_addr));
        
    assert_true(clarinet_addr_is_ipv4(&actual));
    assert_true(clarinet_addr_is_ipv4_any(&actual));
    assert_true(clarinet_addr_is_any(&actual));
}

static void test_clarinet_addr_ipv6_any(void **state)
{
    CLARINET_IGNORE(state);

    clarinet_addr actual = CLARINET_ADDR_IPV6_ANY;
    assert_int_equal(CLARINET_AF_INET6, actual.family);
    assert_int_equal(0, actual.as.ipv6.flowinfo);
    assert_int_equal(0, actual.as.ipv6.scope_id);
    assert_int_equal(0, actual.as.ipv6.u.dword[0]
                      | actual.as.ipv6.u.dword[1]
                      | actual.as.ipv6.u.dword[2]
                      | actual.as.ipv6.u.dword[3]);
    assert_int_equal(0, actual.as.ipv6.u.word[0]
                      | actual.as.ipv6.u.word[1]
                      | actual.as.ipv6.u.word[2]
                      | actual.as.ipv6.u.word[3]
                      | actual.as.ipv6.u.word[4]
                      | actual.as.ipv6.u.word[5]
                      | actual.as.ipv6.u.word[6]
                      | actual.as.ipv6.u.word[7]);
    assert_int_equal(0, actual.as.ipv6.u.byte[0]
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
                      | actual.as.ipv6.u.byte[15]);

    clarinet_addr expected;
    memset(&expected, 0, sizeof(clarinet_addr));
    expected.family = CLARINET_AF_INET6;
    
    assert_memory_equal(&expected, &actual, sizeof(clarinet_addr));
        
    assert_true(clarinet_addr_is_ipv6(&actual));
    assert_true(clarinet_addr_is_ipv6_any(&actual));
    assert_true(clarinet_addr_is_any(&actual));
}

static void test_clarinet_addr_ipv4_loopback(void** state)
{
    CLARINET_IGNORE(state);

    clarinet_addr actual = CLARINET_ADDR_IPV4_LOOPBACK;
    assert_int_equal(CLARINET_AF_INET, actual.family);
    assert_int_equal(127, actual.as.ipv4.u.byte[0]);
    assert_int_equal(0, actual.as.ipv4.u.byte[1]);
    assert_int_equal(0, actual.as.ipv4.u.byte[2]);
    assert_int_equal(1, actual.as.ipv4.u.byte[3]);

    clarinet_addr expected;
    memset(&expected, 0, sizeof(clarinet_addr));
    expected.family = CLARINET_AF_INET;
    expected.as.ipv4.u.byte[0] = 127;
    expected.as.ipv4.u.byte[1] = 0;
    expected.as.ipv4.u.byte[2] = 0;
    expected.as.ipv4.u.byte[3] = 1;

    assert_memory_equal(&expected, &actual, sizeof(clarinet_addr));
    
    assert_true(clarinet_addr_is_ipv4(&actual));
    assert_true(clarinet_addr_is_ipv4_loopback(&actual));
    assert_true(clarinet_addr_is_loopback(&actual));

    actual.as.ipv4.u.byte[3] = 0;
    assert_false(clarinet_addr_is_ipv4_loopback(&actual));

    actual.as.ipv4.u.byte[3] = 255;
    assert_false(clarinet_addr_is_ipv4_loopback(&actual));

    actual.as.ipv4.u.byte[3] = 2;
    assert_true(clarinet_addr_is_ipv4_loopback(&actual));

    actual.as.ipv4.u.byte[3] = 254;
    assert_true(clarinet_addr_is_ipv4_loopback(&actual));

    actual.as.ipv4.u.byte[1] = 255;
    actual.as.ipv4.u.byte[2] = 255;
    assert_true(clarinet_addr_is_ipv4_loopback(&actual));
}

static void test_clarinet_addr_ipv6_loopback(void** state)
{
    CLARINET_IGNORE(state);

    clarinet_addr actual = CLARINET_ADDR_IPV6_LOOPBACK;
    assert_int_equal(CLARINET_AF_INET6, actual.family);
    assert_int_equal(0, actual.as.ipv6.flowinfo);
    assert_int_equal(0, actual.as.ipv6.scope_id);
    assert_int_equal(0, actual.as.ipv6.u.dword[0]
                      | actual.as.ipv6.u.dword[1]
                      | actual.as.ipv6.u.dword[2]);
    assert_int_equal(0, actual.as.ipv6.u.word[0]
                      | actual.as.ipv6.u.word[1]
                      | actual.as.ipv6.u.word[2]
                      | actual.as.ipv6.u.word[3]
                      | actual.as.ipv6.u.word[4]
                      | actual.as.ipv6.u.word[5]
                      | actual.as.ipv6.u.word[6]);
    assert_int_equal(0, actual.as.ipv6.u.byte[0]
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
                      | actual.as.ipv6.u.byte[14]);
 
    assert_int_equal(1, actual.as.ipv6.u.byte[15]);

    clarinet_addr expected;
    memset(&expected, 0, sizeof(clarinet_addr));
    expected.family = CLARINET_AF_INET6;
    expected.as.ipv6.u.byte[15] = 1;

    assert_memory_equal(&expected, &actual, sizeof(clarinet_addr));

    assert_true(clarinet_addr_is_ipv6(&actual));
    assert_true(clarinet_addr_is_ipv6_loopback(&actual));
    assert_true(clarinet_addr_is_loopback(&actual));


    actual.as.ipv6.u.byte[15] = 0;
    assert_false(clarinet_addr_is_ipv6_loopback(&actual));

    actual.as.ipv6.u.byte[15] = 255;
    assert_false(clarinet_addr_is_ipv6_loopback(&actual));

    actual.as.ipv6.u.byte[15] = 2;
    assert_false(clarinet_addr_is_ipv6_loopback(&actual));

    actual.as.ipv6.u.byte[15] = 254;
    assert_false(clarinet_addr_is_ipv6_loopback(&actual));
}

static void test_clarinet_addr_ipv4mapped_loopback(void** state)
{
    CLARINET_IGNORE(state);

    clarinet_addr actual = CLARINET_ADDR_IPV4MAPPED_LOOPBACK;
    assert_int_equal(CLARINET_AF_INET6, actual.family);
    assert_int_equal(0, actual.as.ipv6.flowinfo);
    assert_int_equal(0, actual.as.ipv6.scope_id);
    assert_int_equal(0, actual.as.ipv6.u.dword[0]
                      | actual.as.ipv6.u.dword[1]);
    assert_int_equal(0, actual.as.ipv6.u.word[0]
                      | actual.as.ipv6.u.word[1]
                      | actual.as.ipv6.u.word[2]
                      | actual.as.ipv6.u.word[3]
                      | actual.as.ipv6.u.word[4]);
    assert_int_equal(0, actual.as.ipv6.u.byte[0]
                      | actual.as.ipv6.u.byte[1]
                      | actual.as.ipv6.u.byte[2]
                      | actual.as.ipv6.u.byte[3]
                      | actual.as.ipv6.u.byte[4]
                      | actual.as.ipv6.u.byte[5]
                      | actual.as.ipv6.u.byte[6]
                      | actual.as.ipv6.u.byte[7]
                      | actual.as.ipv6.u.byte[8]
                      | actual.as.ipv6.u.byte[9]);

    assert_int_equal(255, actual.as.ipv6.u.byte[10]);
    assert_int_equal(255, actual.as.ipv6.u.byte[11]);
    assert_int_equal(127, actual.as.ipv6.u.byte[12]);
    assert_int_equal(0, actual.as.ipv6.u.byte[13]);
    assert_int_equal(0, actual.as.ipv6.u.byte[14]);
    assert_int_equal(1, actual.as.ipv6.u.byte[15]);

    clarinet_addr expected;
    memset(&expected, 0, sizeof(clarinet_addr));
    expected.family = CLARINET_AF_INET6;
    expected.as.ipv6.u.byte[10] = 255;
    expected.as.ipv6.u.byte[11] = 255;
    expected.as.ipv6.u.byte[12] = 127;
    expected.as.ipv6.u.byte[13] = 0;
    expected.as.ipv6.u.byte[14] = 0;
    expected.as.ipv6.u.byte[15] = 1;

    assert_memory_equal(&expected, &actual, sizeof(clarinet_addr));

    assert_true(clarinet_addr_is_ipv4mapped(&actual));
    assert_true(clarinet_addr_is_ipv4mapped_loopback(&actual));
    assert_true(clarinet_addr_is_loopback(&actual));

    actual.as.ipv6.u.byte[15] = 0;
    assert_false(clarinet_addr_is_ipv4mapped_loopback(&actual));

    actual.as.ipv6.u.byte[15] = 255;
    assert_false(clarinet_addr_is_ipv4mapped_loopback(&actual));

    actual.as.ipv6.u.byte[15] = 2;
    assert_true(clarinet_addr_is_ipv4mapped_loopback(&actual));

    actual.as.ipv6.u.byte[13] = 255;
    actual.as.ipv6.u.byte[14] = 255;
    assert_true(clarinet_addr_is_ipv4mapped_loopback(&actual));
}

static void test_clarinet_addr_is_equal(void** state)
{
    CLARINET_IGNORE(state);

    {
        clarinet_addr a = CLARINET_ADDR_IPV4_ANY;
        clarinet_addr b = CLARINET_ADDR_IPV4_ANY;
        assert_true(clarinet_addr_is_equal(&a, &b));
    }

    {
        clarinet_addr a = CLARINET_ADDR_IPV6_ANY;
        clarinet_addr b = CLARINET_ADDR_IPV6_ANY;
        assert_true(clarinet_addr_is_equal(&a, &b));
    }

    {
        clarinet_addr a = CLARINET_ADDR_IPV4_LOOPBACK;
        clarinet_addr b = CLARINET_ADDR_IPV4_LOOPBACK;
        assert_true(clarinet_addr_is_equal(&a, &b));
    }

    {
        clarinet_addr a = CLARINET_ADDR_IPV6_LOOPBACK;
        clarinet_addr b = CLARINET_ADDR_IPV6_LOOPBACK;
        assert_true(clarinet_addr_is_equal(&a, &b));
    }

    {
        clarinet_addr a = CLARINET_ADDR_IPV4MAPPED_LOOPBACK;
        clarinet_addr b = CLARINET_ADDR_IPV4MAPPED_LOOPBACK;
        assert_true(clarinet_addr_is_equal(&a, &b));
    }
    
    {
        clarinet_addr a = CLARINET_ADDR_IPV4_LOOPBACK;
        clarinet_addr b = CLARINET_ADDR_IPV4MAPPED_LOOPBACK;
        assert_false(clarinet_addr_is_equal(&a, &b));
    }

    {
        clarinet_addr a;
        a.family = CLARINET_AF_INET;
        a.as.ipv4.u.byte[0] = 0x00;
        a.as.ipv4.u.byte[1] = 0x11;
        a.as.ipv4.u.byte[2] = 0x22;
        a.as.ipv4.u.byte[3] = 0x33;

        clarinet_addr b;
        b.family = CLARINET_AF_INET;
        b.as.ipv4.u.byte[0] = 0x00;
        b.as.ipv4.u.byte[1] = 0x11;
        b.as.ipv4.u.byte[2] = 0x22;
        b.as.ipv4.u.byte[3] = 0x33;

        assert_true(clarinet_addr_is_equal(&a, &b));
    }

    {
        clarinet_addr a;
        a.family = CLARINET_AF_NONE;
        a.as.ipv4.u.byte[0] = 0x00;
        a.as.ipv4.u.byte[1] = 0x11;
        a.as.ipv4.u.byte[2] = 0x22;
        a.as.ipv4.u.byte[3] = 0x33;

        clarinet_addr b;
        b.family = CLARINET_AF_INET;
        b.as.ipv4.u.byte[0] = 0x00;
        b.as.ipv4.u.byte[1] = 0x11;
        b.as.ipv4.u.byte[2] = 0x22;
        b.as.ipv4.u.byte[3] = 0x33;

        assert_false(clarinet_addr_is_equal(&a, &b));
    }

    {
        clarinet_addr a;
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

        assert_true(clarinet_addr_is_equal(&a, &b));
    }

    {
        clarinet_addr a;
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

        assert_false(clarinet_addr_is_equal(&a, &b));
    }

    {
        clarinet_addr a;
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

        assert_false(clarinet_addr_is_equal(&a, &b));
    }

    {
        clarinet_addr a;
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

        assert_false(clarinet_addr_is_equal(&a, &b));
    }

    {
        clarinet_addr a;
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

        assert_false(clarinet_addr_is_equal(&a, &b));
    }

    {
        clarinet_addr a;
        a.family = CLARINET_AF_INET;
        a.as.ipv4.u.byte[0] = 0x00;
        a.as.ipv4.u.byte[1] = 0x11;
        a.as.ipv4.u.byte[2] = 0x22;
        a.as.ipv4.u.byte[3] = 0x33;

        clarinet_addr b;
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

        assert_false(clarinet_addr_is_equal(&a, &b));
    }
}

static void test_clarinet_addr_is_equivalent(void** state)
{
    CLARINET_IGNORE(state);

    {
        clarinet_addr a = CLARINET_ADDR_IPV4_ANY;
        clarinet_addr b = CLARINET_ADDR_IPV4_ANY;
        assert_true(clarinet_addr_is_equivalent(&a, &b));
    }

    {
        clarinet_addr a = CLARINET_ADDR_IPV6_ANY;
        clarinet_addr b = CLARINET_ADDR_IPV6_ANY;
        assert_true(clarinet_addr_is_equivalent(&a, &b));
    }

    {
        clarinet_addr a = CLARINET_ADDR_IPV4_LOOPBACK;
        clarinet_addr b = CLARINET_ADDR_IPV4_LOOPBACK;
        assert_true(clarinet_addr_is_equivalent(&a, &b));
    }

    {
        clarinet_addr a = CLARINET_ADDR_IPV6_LOOPBACK;
        clarinet_addr b = CLARINET_ADDR_IPV6_LOOPBACK;
        assert_true(clarinet_addr_is_equivalent(&a, &b));
    }

    {
        clarinet_addr a = CLARINET_ADDR_IPV4MAPPED_LOOPBACK;
        clarinet_addr b = CLARINET_ADDR_IPV4MAPPED_LOOPBACK;
        assert_true(clarinet_addr_is_equivalent(&a, &b));
    }

    {
        clarinet_addr a = CLARINET_ADDR_IPV4_LOOPBACK;
        clarinet_addr b = CLARINET_ADDR_IPV4MAPPED_LOOPBACK;
        assert_true(clarinet_addr_is_equivalent(&a, &b));
    }

    {
        clarinet_addr a;
        a.family = CLARINET_AF_INET;
        a.as.ipv4.u.byte[0] = 0x00;
        a.as.ipv4.u.byte[1] = 0x11;
        a.as.ipv4.u.byte[2] = 0x22;
        a.as.ipv4.u.byte[3] = 0x33;

        clarinet_addr b;
        b.family = CLARINET_AF_INET;
        b.as.ipv4.u.byte[0] = 0x00;
        b.as.ipv4.u.byte[1] = 0x11;
        b.as.ipv4.u.byte[2] = 0x22;
        b.as.ipv4.u.byte[3] = 0x33;

        assert_true(clarinet_addr_is_equivalent(&a, &b));
    }

    {
        clarinet_addr a;
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

        assert_true(clarinet_addr_is_equivalent(&a, &b));
    }

    {
        clarinet_addr a;
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

        assert_false(clarinet_addr_is_equivalent(&a, &b));
    }

    {
        clarinet_addr a;
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

        assert_false(clarinet_addr_is_equivalent(&a, &b));
    }

    {
        clarinet_addr a;
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

        assert_false(clarinet_addr_is_equivalent(&a, &b));
    }

    {
        clarinet_addr a;
        a.family = CLARINET_AF_INET;
        a.as.ipv4.u.byte[0] = 0x00;
        a.as.ipv4.u.byte[1] = 0x11;
        a.as.ipv4.u.byte[2] = 0x22;
        a.as.ipv4.u.byte[3] = 0x33;

        clarinet_addr b;
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

        assert_true(clarinet_addr_is_equivalent(&a, &b));
    }
}

static void test_clarinet_addr_strlen(void** state)
{
    CLARINET_IGNORE(state);

    assert_true(CLARINET_ADDR_STRLEN >= 57);
}

static void test_clarinet_addr_map_to_ipv4(void** state)
{
    CLARINET_IGNORE(state);

    {
        clarinet_addr dst;
        clarinet_addr src = CLARINET_ADDR_IPV4MAPPED_LOOPBACK;
        clarinet_addr expected = CLARINET_ADDR_IPV4_LOOPBACK;
        assert_int_equal(CLARINET_ENONE, clarinet_addr_map_to_ipv4(&dst, &src));
        assert_true(clarinet_addr_is_equal(&expected, &dst));
    }

    {
        clarinet_addr dst;
        clarinet_addr src = CLARINET_ADDR_IPV4_LOOPBACK;
        clarinet_addr expected = CLARINET_ADDR_IPV4_LOOPBACK;
        assert_int_equal(CLARINET_ENONE, clarinet_addr_map_to_ipv4(&dst, &src));
        assert_true(clarinet_addr_is_equal(&expected, &dst));
    }

    {
        clarinet_addr dst;
        clarinet_addr src = CLARINET_ADDR_IPV6_LOOPBACK;
        assert_int_not_equal(CLARINET_ENONE, clarinet_addr_map_to_ipv4(&dst, &src));
    }

    {
        clarinet_addr dst;
        clarinet_addr src;
        memset(&src, 0, sizeof(clarinet_addr));
        assert_int_not_equal(CLARINET_ENONE, clarinet_addr_map_to_ipv4(&dst, &src));
    }
}

static void test_clarinet_addr_map_to_ipv6(void** state)
{
    CLARINET_IGNORE(state);

    {
        clarinet_addr dst;
        clarinet_addr src = CLARINET_ADDR_IPV4_LOOPBACK;
        clarinet_addr expected = CLARINET_ADDR_IPV4MAPPED_LOOPBACK;
        assert_int_equal(CLARINET_ENONE, clarinet_addr_map_to_ipv6(&dst, &src));
        assert_true(clarinet_addr_is_equal(&expected, &dst));
    }

    {
        clarinet_addr dst;
        clarinet_addr src = CLARINET_ADDR_IPV6_LOOPBACK;
        clarinet_addr expected = CLARINET_ADDR_IPV6_LOOPBACK;
        assert_int_equal(CLARINET_ENONE, clarinet_addr_map_to_ipv6(&dst, &src));
        assert_true(clarinet_addr_is_equal(&expected, &dst));
    }

    {
        clarinet_addr dst;
        clarinet_addr src;
        memset(&src, 0, sizeof(clarinet_addr));
        assert_int_not_equal(CLARINET_ENONE, clarinet_addr_map_to_ipv6(&dst, &src));
    }
}

static void test_clarinet_addr_to_string(void** state)
{
    CLARINET_IGNORE(state);

    {
        char dst[CLARINET_ADDR_STRLEN];
        clarinet_addr src = CLARINET_ADDR_IPV4_ANY;
        const char* expected = "0.0.0.0";
        assert_true(clarinet_addr_to_string(dst, sizeof(dst), &src) > 0);
        assert_memory_equal(dst, expected, strlen(expected) + 1);
    }

    {
        char dst[CLARINET_ADDR_STRLEN];
        clarinet_addr src = CLARINET_ADDR_IPV4_LOOPBACK;
        const char* expected = "127.0.0.1";
        assert_true(clarinet_addr_to_string(dst, sizeof(dst), &src) > 0);
        assert_memory_equal(dst, expected, strlen(expected) + 1);
    }

    {
        char dst[CLARINET_ADDR_STRLEN];
        clarinet_addr src = CLARINET_ADDR_IPV4_BROADCAST;
        const char* expected = "255.255.255.255";
        assert_true(clarinet_addr_to_string(dst, sizeof(dst), &src) > 0);
        assert_memory_equal(dst, expected, strlen(expected) + 1);
    }

    {
        char dst[CLARINET_ADDR_STRLEN];
        clarinet_addr src = CLARINET_ADDR_IPV6_LOOPBACK;
        const char* expected = "::1";
        assert_true(clarinet_addr_to_string(dst, sizeof(dst), &src) > 0);
        assert_memory_equal(dst, expected, strlen(expected) + 1);
    }
}


int main(int argc, char* argv[])
{
    CLARINET_IGNORE(argc);
    CLARINET_IGNORE(argv);
    
    const struct CMUnitTest tests[] =
    {
        cmocka_unit_test(test_clarinet_addr_size),
        cmocka_unit_test(test_clarinet_addr_ipv4_any),
        cmocka_unit_test(test_clarinet_addr_ipv6_any),
        cmocka_unit_test(test_clarinet_addr_ipv4_loopback),
        cmocka_unit_test(test_clarinet_addr_ipv6_loopback),
        cmocka_unit_test(test_clarinet_addr_ipv4mapped_loopback),
        cmocka_unit_test(test_clarinet_addr_is_equal),
        cmocka_unit_test(test_clarinet_addr_is_equivalent),
        cmocka_unit_test(test_clarinet_addr_strlen),
        cmocka_unit_test(test_clarinet_addr_map_to_ipv4),
        cmocka_unit_test(test_clarinet_addr_map_to_ipv6),
        cmocka_unit_test(test_clarinet_addr_to_string),

    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}