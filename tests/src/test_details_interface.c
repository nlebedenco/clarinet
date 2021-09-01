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

static void test_clarinet_get_semver(void **state)
{
    CLARINET_IGNORE(state);
    
    const uint32_t expected = (CONFIG_VERSION_MAJOR << 24) 
                            | (CONFIG_VERSION_MINOR << 16) 
                            | (CONFIG_VERSION_PATCH << 8);

    assert_in_range(CONFIG_VERSION_MAJOR, 0, 255);
    assert_in_range(CONFIG_VERSION_MINOR, 0, 255);
    assert_in_range(CONFIG_VERSION_PATCH, 0, 255);
    assert_int_not_equal(0, CONFIG_VERSION_MAJOR | CONFIG_VERSION_MINOR | CONFIG_VERSION_PATCH);

    const uint32_t actual = clarinet_get_semver();
    assert_int_equal(expected, actual);
}

static void test_clarinet_get_version(void **state)
{
    CLARINET_IGNORE(state);
    
    const char* expected = (CLARINET_XSTR(CONFIG_VERSION_MAJOR)"."CLARINET_XSTR(CONFIG_VERSION_MINOR)"."CLARINET_XSTR(CONFIG_VERSION_PATCH)); 
    const char* actual = clarinet_get_version();
    assert_string_equal(expected, actual);
}

static void test_clarinet_get_name(void **state)
{
    CLARINET_IGNORE(state);
    
    const char* expected = "clarinet";
    const char* actual = clarinet_get_name();
    assert_string_equal(expected, actual);
}

static void test_clarinet_get_description(void **state)
{
    CLARINET_IGNORE(state);
    
    const char* actual = clarinet_get_description();
    assert_non_null(actual);
    assert_true(strlen(actual) > 0);
}

static void test_clarinet_get_protocols(void **state)
{
    CLARINET_IGNORE(state);
    
    assert_int_equal(0, CLARINET_PROTO_NONE);
    uint32_t protocols = clarinet_get_protocols();
    
    #if defined(CLARINET_ENABLE_UDP)
    assert_int_equal(CLARINET_PROTO_UDP, protocols & CLARINET_PROTO_UDP);
    protocols &= ~CLARINET_PROTO_UDP;
    #else
    assert_int_equal(CLARINET_PROTO_NONE, protocols & CLARINET_PROTO_UDP);
    #endif
    
    #if defined(CLARINET_ENABLE_DTLC)
    assert_int_equal(CLARINET_PROTO_DTLC, protocols & CLARINET_PROTO_DTLC);
    protocols &= ~CLARINET_PROTO_DTLC;
    #else
    assert_int_equal(CLARINET_PROTO_NONE, protocols & CLARINET_PROTO_DTLC);
    #endif
    
    #if defined(CLARINET_ENABLE_DTLS)
    assert_int_equal(CLARINET_PROTO_DTLS, protocols & CLARINET_PROTO_DTLS);
    protocols &= ~CLARINET_PROTO_DTLS;
    #else
    assert_int_equal(CLARINET_PROTO_NONE, protocols & CLARINET_PROTO_DTLS);
    #endif
    
    #if defined(CLARINET_ENABLE_UDTP)
    assert_int_equal(CLARINET_PROTO_UDTP, protocols & CLARINET_PROTO_UDTP);
    protocols &= ~CLARINET_PROTO_UDTP;
    #else
    assert_int_equal(CLARINET_PROTO_NONE, protocols & CLARINET_PROTO_UDTP);
    #endif
    
    #if defined(CLARINET_ENABLE_UDTPS)
    assert_int_equal(CLARINET_PROTO_UDTPS, protocols & CLARINET_PROTO_UDTPS);
    protocols &= ~CLARINET_PROTO_UDTPS;
    #else
    assert_int_equal(CLARINET_PROTO_NONE, protocols & CLARINET_PROTO_UDTPS);
    #endif
    
    #if defined(CLARINET_ENABLE_ENET)
    assert_int_equal(CLARINET_PROTO_ENET, protocols & CLARINET_PROTO_ENET);
    protocols &= ~CLARINET_PROTO_ENET;
    #else
    assert_int_equal(CLARINET_PROTO_NONE, protocols & CLARINET_PROTO_ENET);
    #endif
    
    #if defined(CLARINET_ENABLE_ENETS)
    assert_int_equal(CLARINET_PROTO_ENETS, protocols & CLARINET_PROTO_ENETS);
    protocols &= ~CLARINET_PROTO_ENETS;
    #else
    assert_int_equal(CLARINET_PROTO_NONE, protocols & CLARINET_PROTO_ENETS);
    #endif
    
    #if defined(CLARINET_ENABLE_TCP)
    assert_int_equal(CLARINET_PROTO_TCP, protocols & CLARINET_PROTO_TCP);
    protocols &= ~CLARINET_PROTO_TCP;
    #else
    assert_int_equal(CLARINET_PROTO_NONE, protocols & CLARINET_PROTO_TCP);
    #endif
    
    #if defined(CLARINET_ENABLE_TLS)
    assert_int_equal(CLARINET_PROTO_TLS, protocols & CLARINET_PROTO_TLS);
    protocols &= ~CLARINET_PROTO_TLS;
    #else
    assert_int_equal(CLARINET_PROTO_NONE, protocols & CLARINET_PROTO_TLS);
    #endif   
    
    assert_int_equal(CLARINET_PROTO_NONE, protocols);
}

static void test_clarinet_get_features(void **state)
{
    CLARINET_IGNORE(state);
    
    assert_int_equal(0, CLARINET_FEATURE_NONE);
    uint32_t features = clarinet_get_features();
    
    #if !defined(NDEBUG)
    assert_int_equal(CLARINET_FEATURE_DEBUG, features & CLARINET_FEATURE_DEBUG);
    features &= ~CLARINET_FEATURE_DEBUG;
    #else
    assert_int_equal(CLARINET_FEATURE_NONE, features & CLARINET_FEATURE_DEBUG);
    #endif
 
    
    #if defined(CLARINET_ENABLE_PROFILE)
    assert_int_equal(CLARINET_FEATURE_PROFILE, features & CLARINET_FEATURE_PROFILE);
    features &= ~CLARINET_FEATURE_PROFILE;
    #else
    assert_int_equal(CLARINET_FEATURE_NONE, features & CLARINET_FEATURE_PROFILE);
    #endif  
    
    #if defined(CLARINET_ENABLE_LOG)
    assert_int_equal(CLARINET_FEATURE_LOG, features & CLARINET_FEATURE_LOG);
    features &= ~CLARINET_FEATURE_LOG;
    #else
    assert_int_equal(CLARINET_FEATURE_NONE, features & CLARINET_FEATURE_LOG);
    #endif  
    
    #if defined(CLARINET_ENABLE_IPV6)
    assert_int_equal(CLARINET_FEATURE_IPV6, features & CLARINET_FEATURE_IPV6);
    features &= ~CLARINET_FEATURE_IPV6;
    #else
    assert_int_equal(CLARINET_FEATURE_NONE, features & CLARINET_FEATURE_IPV6);
    #endif  
    
    #if defined(CLARINET_ENABLE_IPV6DUAL)
    assert_int_equal(CLARINET_FEATURE_IPV6DUAL, features & CLARINET_FEATURE_IPV6DUAL);
    features &= ~CLARINET_FEATURE_IPV6DUAL;
    #else
    assert_int_equal(CLARINET_FEATURE_NONE, features & CLARINET_FEATURE_IPV6DUAL);
    #endif      
    
    assert_int_equal(CLARINET_FEATURE_NONE, features);
}

int main(int argc, char* argv[])
{
    CLARINET_IGNORE(argc);
    CLARINET_IGNORE(argv);
    
    const struct CMUnitTest tests[] =
    {
        cmocka_unit_test(test_clarinet_get_semver),
        cmocka_unit_test(test_clarinet_get_version),
        cmocka_unit_test(test_clarinet_get_name),
        cmocka_unit_test(test_clarinet_get_description),
        cmocka_unit_test(test_clarinet_get_protocols),
        cmocka_unit_test(test_clarinet_get_features)
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}