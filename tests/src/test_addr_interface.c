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

    assert_int_equal(20, sizeof(struct clarinet_addr_ipv4));
    assert_int_equal(24, sizeof(struct clarinet_addr_ipv6));
    
    assert_int_equal(sizeof(struct clarinet_addr_ipv4), sizeof(((clarinet_addr*)0)->as.ipv4));
    assert_int_equal(sizeof(struct clarinet_addr_ipv6), sizeof(((clarinet_addr*)0)->as.ipv6));
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

    clarinet_addr expected = {0};
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

    clarinet_addr expected = {0};
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

    clarinet_addr expected = {0};
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
    actual.as.ipv4.u.byte[3] = 254;
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

    clarinet_addr expected = {0};
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

    clarinet_addr expected = {0};
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
    actual.as.ipv6.u.byte[14] = 254;
    assert_true(clarinet_addr_is_ipv4mapped_loopback(&actual));
}

static void test_clarinet_addr_ipv4_broadcast(void** state)
{
    CLARINET_IGNORE(state);

    clarinet_addr actual = CLARINET_ADDR_IPV4_BROADCAST;
    assert_int_equal(CLARINET_AF_INET, actual.family);
    assert_int_equal(255, actual.as.ipv4.u.byte[0]);
    assert_int_equal(255, actual.as.ipv4.u.byte[1]);
    assert_int_equal(255, actual.as.ipv4.u.byte[2]);
    assert_int_equal(255, actual.as.ipv4.u.byte[3]);

    clarinet_addr expected = {0};
    expected.family = CLARINET_AF_INET;
    expected.as.ipv4.u.byte[0] = 255;
    expected.as.ipv4.u.byte[1] = 255;
    expected.as.ipv4.u.byte[2] = 255;
    expected.as.ipv4.u.byte[3] = 255;

    assert_memory_equal(&expected, &actual, sizeof(clarinet_addr));
    
    assert_true(clarinet_addr_is_ipv4(&actual));
    assert_true(clarinet_addr_is_ipv4_broadcast(&actual));
    assert_true(clarinet_addr_is_broadcast(&actual));

    actual.as.ipv4.u.byte[3] = 0;
    assert_false(clarinet_addr_is_ipv4_broadcast(&actual));

    actual.as.ipv4.u.byte[3] = 127;
    assert_false(clarinet_addr_is_ipv4_broadcast(&actual));

    actual.as.ipv4.u.byte[3] = 2;
    assert_false(clarinet_addr_is_ipv4_broadcast(&actual));

    actual.as.ipv4.u.byte[3] = 254;
    assert_false(clarinet_addr_is_ipv4_broadcast(&actual));

    actual.as.ipv4.u.byte[1] = 0;
    actual.as.ipv4.u.byte[2] = 0;
    actual.as.ipv4.u.byte[3] = 255;
    assert_false(clarinet_addr_is_ipv4_broadcast(&actual));
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

        assert_true(clarinet_addr_is_equal(&a, &b));
    }

    {
        clarinet_addr a;
        memset(&a, 0xAA, sizeof(a)); /* memory noise */
        a.family = CLARINET_AF_NONE;
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

        assert_false(clarinet_addr_is_equal(&a, &b));
    }

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

        assert_true(clarinet_addr_is_equal(&a, &b));
    }

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

        assert_false(clarinet_addr_is_equal(&a, &b));
    }

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

        assert_false(clarinet_addr_is_equal(&a, &b));
    }

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

        assert_false(clarinet_addr_is_equal(&a, &b));
    }

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

        assert_false(clarinet_addr_is_equal(&a, &b));
    }

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

        assert_true(clarinet_addr_is_equivalent(&a, &b));
    }

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

        assert_true(clarinet_addr_is_equivalent(&a, &b));
    }

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

        assert_false(clarinet_addr_is_equivalent(&a, &b));
    }

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

        assert_false(clarinet_addr_is_equivalent(&a, &b));
    }

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

        assert_false(clarinet_addr_is_equivalent(&a, &b));
    }

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

        assert_true(clarinet_addr_is_equivalent(&a, &b));
    }
}

static void test_clarinet_addr_strlen(void** state)
{
    CLARINET_IGNORE(state);

    assert_true(CLARINET_ADDR_STRLEN >= 57); /* "0000:0000:0000:0000:0000:ffff:255.255.255.255%4294967295" */
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
        clarinet_addr src = {0};
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
        cmocka_unit_test(test_clarinet_addr_ipv4_broadcast),
        cmocka_unit_test(test_clarinet_addr_is_equal),
        cmocka_unit_test(test_clarinet_addr_is_equivalent),
        cmocka_unit_test(test_clarinet_addr_strlen),
        cmocka_unit_test(test_clarinet_addr_map_to_ipv4),
        cmocka_unit_test(test_clarinet_addr_map_to_ipv6),
        cmocka_unit_test(test_clarinet_addr_to_string),
        cmocka_unit_test(test_clarinet_addr_from_string),
        cmocka_unit_test(test_clarinet_endpoint_size),
        cmocka_unit_test(test_clarinet_endpoint_strlen),
        cmocka_unit_test(test_clarinet_endpoint_to_string)

    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}