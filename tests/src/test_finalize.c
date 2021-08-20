#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <string.h>

#include "clarinet/clarinet.h"
#include "config.h"
#include "cmocka.h"

#include "test_control_interface.inl"

static void test_clarinet_finalize_ntimes(void **state)
{
    assert_can_finalize(3);
}

static void test_clarinet_initialize_and_finalize(void **state)
{
    assert_can_initialize(1);
    assert_can_finalize(1);
}

static void test_clarinet_initialize_once_and_finalize_ntimes(void **state)
{
    assert_can_initialize(1);
    assert_can_finalize(3);
}

int main()
{
    const struct CMUnitTest tests[] =
    {
        cmocka_unit_test(test_clarinet_finalize_ntimes),
        cmocka_unit_test(test_clarinet_initialize_and_finalize),
        cmocka_unit_test(test_clarinet_initialize_once_and_finalize_ntimes)
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}