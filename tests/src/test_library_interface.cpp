#include "test.h"

TEST_CASE("Library Semantic Version", "[details][version]")
{
    const uint32_t expected = (CONFIG_VERSION_MAJOR << 24)
                              | (CONFIG_VERSION_MINOR << 16)
                              | (CONFIG_VERSION_PATCH << 8);

    const uint32_t actual = clarinet_get_semver();

    REQUIRE(CONFIG_VERSION_MAJOR >= 0);
    REQUIRE(CONFIG_VERSION_MAJOR <= 255);
    REQUIRE(CONFIG_VERSION_MINOR >= 0);
    REQUIRE(CONFIG_VERSION_MINOR <= 255);
    REQUIRE(CONFIG_VERSION_PATCH >= 0);
    REQUIRE(CONFIG_VERSION_PATCH <= 255);
    REQUIRE((CONFIG_VERSION_MAJOR | CONFIG_VERSION_MINOR | CONFIG_VERSION_PATCH) != 0);
    REQUIRE(actual == expected);
}

TEST_CASE("Library String Version", "[details][version]")
{
    const char* expected = CLARINET_XSTR(CONFIG_VERSION_MAJOR)
                           "." CLARINET_XSTR(CONFIG_VERSION_MINOR)
                           "." CLARINET_XSTR(CONFIG_VERSION_PATCH);
    const char* actual = clarinet_get_version();
    REQUIRE_THAT(actual, Equals(expected));
}

TEST_CASE("Library Name", "[details][name]")
{
    const char* expected = "clarinet";
    const char* actual = clarinet_get_name();
    REQUIRE_THAT(actual, Equals(expected));
}

TEST_CASE("Library Description", "[details][description]")
{
    const char* actual = clarinet_get_description();
    REQUIRE(actual != NULL);
    REQUIRE_THAT(std::string(actual), !IsEmpty());
}

#define REQUIRE_FLAG_SET(F, P)          do { REQUIRE(((F) & (P)) == (P)); (F) &= ~(P); } while(0)
#define REQUIRE_FLAG_CLR(F, P)          REQUIRE(((F) & (P)) == 0)

TEST_CASE("Library Feature Flags", "[details][features]")
{
    REQUIRE(CLARINET_FEATURE_NONE == 0);

    uint32_t features = clarinet_get_features();

    #if !defined(NDEBUG)
    REQUIRE_FLAG_SET(features, CLARINET_FEATURE_DEBUG);
    #else
    REQUIRE_FLAG_CLR(features, CLARINET_FEATURE_DEBUG);
    #endif

    #if defined(CLARINET_ENABLE_PROFILE)
    REQUIRE_FLAG_SET(features, CLARINET_FEATURE_PROFILE);
    #else
    REQUIRE_FLAG_CLR(features, CLARINET_FEATURE_PROFILE);
    #endif

    #if defined(CLARINET_ENABLE_LOG)
    REQUIRE_FLAG_SET(features, CLARINET_FEATURE_LOG);
    #else
    REQUIRE_FLAG_CLR(features, CLARINET_FEATURE_LOG);
    #endif

    #if CLARINET_ENABLE_IPV6
    REQUIRE_FLAG_SET(features, CLARINET_FEATURE_IPV6);
    #else
    REQUIRE_FLAG_CLR(features, CLARINET_FEATURE_IPV6);
    #endif

    #if CLARINET_ENABLE_IPV6DUAL
    REQUIRE_FLAG_SET(features, CLARINET_FEATURE_IPV6DUAL);
    #else
    REQUIRE_FLAG_CLR(features, CLARINET_FEATURE_IPV6DUAL);
    #endif

    REQUIRE(features == CLARINET_FEATURE_NONE);
}

TEST_CASE("Library Initialize")
{
    SECTION("Initialize ONCE")
    {
        int errcode = clarinet_initialize();
        REQUIRE(Error(errcode) == CLARINET_ENONE);

        SECTION("Finalize ONCE")
        {
            errcode = clarinet_finalize();
            REQUIRE(Error(errcode) == CLARINET_ENONE);
        }

        SECTION("Finalize MULTIPLE")
        {
            /* Repeated calls should never fail */
            for (int i = 0; i < 10; ++i)
            {
                errcode = clarinet_finalize();
                REQUIRE(Error(errcode) == CLARINET_ENONE);
            }
        }
    }

    SECTION("Initialize MULTIPLE")
    {
        /* Repeated calls should never fail */
        for (int i = 0; i < 10; ++i)
        {
            int errcode = clarinet_initialize();
            REQUIRE(Error(errcode) == CLARINET_ENONE);
        }

        SECTION("Finalize ONCE")
        {
            int errcode = clarinet_finalize();
            REQUIRE(Error(errcode) == CLARINET_ENONE);
        }

        SECTION("Finalize MULTIPLE")
        {
            /* Repeated calls should never fail */
            for (int i = 0; i < 10; ++i)
            {
                int errcode = clarinet_finalize();
                REQUIRE(Error(errcode) == CLARINET_ENONE);
            }
        }
    }
}

TEST_CASE("Library Finalize")
{
    /* It's OK to call finalize without a previous call to Initialize() */
    int errcode = clarinet_finalize();
    REQUIRE(Error(errcode) == CLARINET_ENONE);

    /* And repeated calls should never fail */
    for (int i = 0; i < 10; ++i)
    {
        errcode = clarinet_finalize();
        REQUIRE(Error(errcode) == CLARINET_ENONE);
    }
}
