#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <string.h>

#include "clarinet/clarinet.h"
#include "config.h"
#include "cmocka.h"

static void test_clarinet_getlibnver(void **state)
{
    const uint32_t expected = (CONFIG_VERSION_MAJOR << 24) 
                            | (CONFIG_VERSION_MINOR << 16) 
                            | (CONFIG_VERSION_PATCH << 8);

    assert_in_range(CONFIG_VERSION_MAJOR, 0, 255);
    assert_in_range(CONFIG_VERSION_MINOR, 0, 255);
    assert_in_range(CONFIG_VERSION_PATCH, 0, 255);
    assert_int_not_equal(0, CONFIG_VERSION_MAJOR | CONFIG_VERSION_MINOR | CONFIG_VERSION_PATCH);

    const uint32_t actual = clarinet_getlibnver();
    assert_int_equal(expected, actual);
}

static void test_clarinet_getlibsver(void **state)
{
    const char* expected = (CLARINET_XSTR(CONFIG_VERSION_MAJOR)"."CLARINET_XSTR(CONFIG_VERSION_MINOR)"."CLARINET_XSTR(CONFIG_VERSION_PATCH)); 
    const char* actual = clarinet_getlibsver();
    assert_string_equal(expected, actual);
}

static void test_clarinet_getlibname(void **state)
{
    const char* expected = "clarinet";
    const char* actual = clarinet_getlibname();
    assert_string_equal(expected, actual);
}

static void test_clarinet_getlibdesc(void **state)
{
    const char* actual = clarinet_getlibdesc();
    assert_non_null(actual);
    assert_true(strlen(actual) > 0);
}

int main()
{
    const struct CMUnitTest tests[] =
    {
        cmocka_unit_test(test_clarinet_getlibnver),
        cmocka_unit_test(test_clarinet_getlibsver),
        cmocka_unit_test(test_clarinet_getlibname),
        cmocka_unit_test(test_clarinet_getlibdesc)
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}