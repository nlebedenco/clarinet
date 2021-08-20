#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <string.h>

#include "clarinet/clarinet.h"
#include "config.h"
#include "cmocka.h"

#include "test_control_interface.inl"

static void test_clarinet_initialize(void **state)
{
    assert_can_initialize(1);
}

static void test_clarinet_initialize_ntimes(void **state)
{
    assert_can_initialize(3);
}

int main()
{
    const struct CMUnitTest tests[] =
    {
        cmocka_unit_test(test_clarinet_initialize),
        cmocka_unit_test(test_clarinet_initialize_ntimes)
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}