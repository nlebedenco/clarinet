#include "portability.h"
#include "clarinet/clarinet.h"

#include <string.h>

uint32_t 
clarinet_get_semver(void)
{
    return (CONFIG_VERSION_MAJOR << 24) 
         | (CONFIG_VERSION_MINOR << 16) 
         | (CONFIG_VERSION_PATCH << 8);
}

const char* 
clarinet_get_version(void)
{
    return (CLARINET_XSTR(CONFIG_VERSION_MAJOR)"."CLARINET_XSTR(CONFIG_VERSION_MINOR)"."CLARINET_XSTR(CONFIG_VERSION_PATCH));
}

const char* 
clarinet_get_name(void)
{
    return CONFIG_NAME;
}

const char* 
clarinet_get_description(void)
{
    return CONFIG_DESCRIPTION;
}

uint32_t 
clarinet_get_protocols(void)
{
    uint32_t protocols = CLARINET_PROTO_NONE;
   
    #if defined(CLARINET_ENABLE_UDP)
    protocols |= CLARINET_PROTO_UDP;
    #endif
    
    #if defined(CLARINET_ENABLE_DTLC)
    protocols |= CLARINET_PROTO_DTLC;
    #endif
    
    #if defined(CLARINET_ENABLE_DTLS)
    protocols |= CLARINET_PROTO_DTLS;
    #endif
    
    #if defined(CLARINET_ENABLE_UDTP)
    protocols |= CLARINET_PROTO_UDTP;
    #endif
    
    #if defined(CLARINET_ENABLE_UDTPS)
    protocols |= CLARINET_PROTO_UDTPS;
    #endif
    
    #if defined(CLARINET_ENABLE_ENET)
    protocols |= CLARINET_PROTO_ENET;
    #endif
    
    #if defined(CLARINET_ENABLE_ENETS)
    protocols |= CLARINET_PROTO_ENETS;
    #endif
    
    #if defined(CLARINET_ENABLE_TCP)
    protocols |= CLARINET_PROTO_TCP;
    #endif
    
    #if defined(CLARINET_ENABLE_TLS)
    protocols |= CLARINET_PROTO_TLS;
    #endif
    
    return protocols;
}

uint32_t 
clarinet_get_features(void)
{
    uint32_t features = CLARINET_FEATURE_NONE;
    
    #if !defined(NDEBUG)
    features |= CLARINET_FEATURE_DEBUG;
    #endif  
    
    #if defined(CLARINET_ENABLE_PROFILER)
    features |= CLARINET_FEATURE_PROFILER;
    #endif  
    
    #if defined(CLARINET_ENABLE_LOG)
    features |= CLARINET_FEATURE_LOG;
    #endif  
    
    #if defined(CLARINET_ENABLE_IPV6)
    features |= CLARINET_FEATURE_IPV6;
    #endif  
    
    #if defined(CLARINET_ENABLE_IPV6DUAL)
    features |= CLARINET_FEATURE_IPV6DUAL;
    #endif  
    
    return features;
}

int
clarinet_addr_map_to_ipv4(clarinet_addr* restrict dst, 
                          const clarinet_addr* restrict src)
{
    if (dst && src)
    {
        if (clarinet_addr_is_ipv4(src))
        {
            memcpy(dst, src, sizeof(clarinet_addr));
            return CLARINET_ENONE;
        }

        if (clarinet_addr_is_ipv4mapped(src))
        {
            memset(dst, 0, sizeof(clarinet_addr));
            dst->family = CLARINET_AF_INET;
            dst->as.ipv4.u.dword[0] = src->as.ipv4.u.dword[0];
            return CLARINET_ENONE;
        }
    }
    
    return CLARINET_EINVAL;
}

int
clarinet_addr_map_to_ipv6(clarinet_addr* restrict dst, 
                          const clarinet_addr* restrict src)
{
    if (dst && src)
    {
        if (clarinet_addr_is_ipv6(src))
        {
            memcpy(dst, src, sizeof(clarinet_addr));
            return CLARINET_ENONE;
        }

        if (clarinet_addr_is_ipv4(src))
        {
            memset(dst, 0, sizeof(clarinet_addr));
            dst->family = CLARINET_AF_INET6;
            dst->as.ipv6.u.word[5] = 0xFFFF;
            dst->as.ipv4.u.dword[0] = src->as.ipv4.u.dword[0];
            return CLARINET_ENONE;
        }
    }

    return CLARINET_EINVAL;
}
