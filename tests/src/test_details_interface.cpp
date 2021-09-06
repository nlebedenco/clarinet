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

#define REQUIRE_FLAG_SET(F, P)          \
    do {                                \
        REQUIRE(((F) & (P)) == (P));    \
        (F) &= ~(P);                    \
    } while(0)

#define REQUIRE_FLAG_CLR(F, P)          REQUIRE(((F) & (P)) == 0)
    

TEST_CASE("Library Protocol Flags", "[details][protocols]") 
{      
    REQUIRE(CLARINET_PROTO_NONE == 0);
    
    uint32_t protocols = clarinet_get_protocols();  
    
    #ifdef CLARINET_ENABLE_UDP
    REQUIRE_FLAG_SET(protocols, CLARINET_PROTO_UDP);
    #else
    REQUIRE_FLAG_CLR(protocols, CLARINET_PROTO_UDP);
    #endif
    
    #ifdef CLARINET_ENABLE_DTLC
    REQUIRE_FLAG_SET(protocols, CLARINET_PROTO_DTLC);
    #else
    REQUIRE_FLAG_CLR(protocols, CLARINET_PROTO_DTLC);
    #endif
    
    #ifdef CLARINET_ENABLE_DTLS
    REQUIRE_FLAG_SET(protocols, CLARINET_PROTO_DTLS);
    #else
    REQUIRE_FLAG_CLR(protocols, CLARINET_PROTO_DTLS);
    #endif
    
    #ifdef CLARINET_ENABLE_UDTP
    REQUIRE_FLAG_SET(protocols, CLARINET_PROTO_UDTP);
    #else
    REQUIRE_FLAG_CLR(protocols, CLARINET_PROTO_UDTP);
    #endif
    
    #ifdef CLARINET_ENABLE_UDTPS
    REQUIRE_FLAG_SET(protocols, CLARINET_PROTO_UDTPS);
    #else
    REQUIRE_FLAG_CLR(protocols, CLARINET_PROTO_UDTPS);
    #endif
    
    #ifdef CLARINET_ENABLE_ENET
    REQUIRE_FLAG_SET(protocols, CLARINET_PROTO_ENET);
    #else
    REQUIRE_FLAG_CLR(protocols, CLARINET_PROTO_ENET);
    #endif
    
    #ifdef CLARINET_ENABLE_ENETS
    REQUIRE_FLAG_SET(protocols, CLARINET_PROTO_ENETS);
    #else
    REQUIRE_FLAG_CLR(protocols, CLARINET_PROTO_ENETS);
    #endif
    
    #ifdef CLARINET_ENABLE_TCP
    REQUIRE_FLAG_SET(protocols, CLARINET_PROTO_TCP);
    #else
    REQUIRE_FLAG_CLR(protocols, CLARINET_PROTO_TCP);
    #endif
    
    #ifdef CLARINET_ENABLE_TLS
    REQUIRE_FLAG_SET(protocols, CLARINET_PROTO_TLS);
    #else
    REQUIRE_FLAG_CLR(protocols, CLARINET_PROTO_TLS);
    #endif
    
    REQUIRE(protocols == CLARINET_PROTO_NONE);
}

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
    
    #if defined(CLARINET_ENABLE_IPV6)
    REQUIRE_FLAG_SET(features, CLARINET_FEATURE_IPV6);
    #else
    REQUIRE_FLAG_CLR(features, CLARINET_FEATURE_IPV6);
    #endif
    
    #if defined(CLARINET_ENABLE_IPV6DUAL)
    REQUIRE_FLAG_SET(features, CLARINET_FEATURE_IPV6DUAL);
    #else
    REQUIRE_FLAG_CLR(features, CLARINET_FEATURE_IPV6DUAL);
    #endif

    REQUIRE(features == CLARINET_FEATURE_NONE);
}
