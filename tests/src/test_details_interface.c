#include <stdarg.h>
#include <setjmp.h>
#include <stddef.h>

#include "clarinet/clarinet.h"
#include "cmocka.h"

static void test_clarinet_getlibsver(void **state)
{
    assert_int_equal(2, 2);
}

int main()
{
    const struct CMUnitTest tests[] =
    {
        cmocka_unit_test(test_clarinet_getlibsver)
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}