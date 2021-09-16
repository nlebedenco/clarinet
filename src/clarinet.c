#include "portability.h"
#include "clarinet/clarinet.h"

#include <stdlib.h>
#include <string.h>

const clarinet_addr clarinet_addr_none                 = { 0 };
const clarinet_addr clarinet_addr_ipv4_any             = { CLARINET_AF_INET,  0, { { 0, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   0,   0,   0,   0,   0,   0 } }, 0 } } };
const clarinet_addr clarinet_addr_ipv4_loopback        = { CLARINET_AF_INET,  0, { { 0, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   0,   0, 127,   0,   0,   1 } }, 0 } } };
const clarinet_addr clarinet_addr_ipv4_broadcast       = { CLARINET_AF_INET,  0, { { 0, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   0,   0, 255, 255, 255, 255 } }, 0 } } };    

const clarinet_addr clarinet_addr_ipv6_any             = { CLARINET_AF_INET6, 0, { { 0, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   0,   0,   0,   0,   0,   0 } }, 0 } } };
const clarinet_addr clarinet_addr_ipv6_loopback        = { CLARINET_AF_INET6, 0, { { 0, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   0,   0,   0,   0,   0,   1 } }, 0 } } };
const clarinet_addr clarinet_addr_ipv4mapped_loopback  = { CLARINET_AF_INET6, 0, { { 0, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 127,   0,   0,   1 } }, 0 } } };

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
    /* CLARINET_PROTO_SOCK represents the network layer abstraction provided by the system and is always enabled. */
    uint32_t protocols = CLARINET_PROTO_SOCK;
   
    #if CLARINET_ENABLE_UDP
    protocols |= CLARINET_PROTO_UDP;
    #endif
    
    #if CLARINET_ENABLE_DTLC
    protocols |= CLARINET_PROTO_DTLC;
    #endif
    
    #if CLARINET_ENABLE_DTLS
    protocols |= CLARINET_PROTO_DTLS;
    #endif
    
    #if CLARINET_ENABLE_UDTP
    protocols |= CLARINET_PROTO_UDTP;
    #endif
    
    #if CLARINET_ENABLE_UDTPS
    protocols |= CLARINET_PROTO_UDTPS;
    #endif
    
    #if CLARINET_ENABLE_ENET
    protocols |= CLARINET_PROTO_ENET;
    #endif
    
    #if CLARINET_ENABLE_ENETS
    protocols |= CLARINET_PROTO_ENETS;
    #endif
    
    #if CLARINET_ENABLE_TCP
    protocols |= CLARINET_PROTO_TCP;
    #endif
    
    #if CLARINET_ENABLE_TLS
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
    
    #if CLARINET_ENABLE_PROFILER
    features |= CLARINET_FEATURE_PROFILER;
    #endif  
    
    #if CLARINET_ENABLE_LOG
    features |= CLARINET_FEATURE_LOG;
    #endif  
    
    #if CLARINET_ENABLE_IPV6
    features |= CLARINET_FEATURE_IPV6;
    #endif  
    
    #if CLARINET_ENABLE_IPV6DUAL
    features |= CLARINET_FEATURE_IPV6DUAL;
    #endif  
    
    return features;
}

#define CLARINET_DECLARE_ENUM_NAME(e, s) case e: return #e;
#define CLARINET_DECLARE_ENUM_STR(e, s) case e: return s;

const char*
clarinet_error_name(int err)
{
    if (err > 0)
        return "(out of range)";

    switch (err)
    {
        CLARINET_DECLARE_ENUM_NAME(CLARINET_ENONE, 0)
        CLARINET_DECLARE_ENUM_NAME(CLARINET_EDEFAULT, 0)
        CLARINET_ERRORS(CLARINET_DECLARE_ENUM_NAME)
        default: return "(undefined)";
    }
}

const char*
clarinet_error_str(int err)
{
    if (err > 0)
        return "Not an error";

    switch (err)
    {
        CLARINET_DECLARE_ENUM_STR(CLARINET_ENONE, "Success")
        CLARINET_DECLARE_ENUM_STR(CLARINET_EDEFAULT, "Operation failed")
        CLARINET_ERRORS(CLARINET_DECLARE_ENUM_STR)
    default: return "Undefined error";
    }
}

static 
void* 
clarinet_default_malloc(size_t size) 
{ 
    return malloc(size); 
}

static 
void 
clarinet_default_free(void* ptr) 
{ 
    free(ptr); 
}

static 
CLARINET_NORETURN
void 
clarinet_default_nomem(void) 
{ 
    abort(); 
}

static clarinet_allocator calrinet_allocator_static = { 
    clarinet_default_malloc, 
    clarinet_default_free, 
    clarinet_default_nomem
};

int
clarinet_set_allocator(const clarinet_allocator* allocator)
{
    if (!allocator->malloc || !allocator->free)
        return CLARINET_EINVAL;
    
    calrinet_allocator_static.malloc = allocator->malloc;
    calrinet_allocator_static.free = allocator->free;
    calrinet_allocator_static.nomem = allocator->nomem ? allocator->nomem : abort;
   
    return CLARINET_ENONE;
}

void*
clarinet_malloc(size_t size)
{
    void* ptr = calrinet_allocator_static.malloc(size);

   if (!ptr)
     calrinet_allocator_static.nomem();

   return ptr;
}


void
clarinet_free(void* ptr)
{
    calrinet_allocator_static.free(ptr);
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

        #if CLARINET_ENABLE_IPV6
        if (clarinet_addr_is_ipv4mapped(src))
        {
            memset(dst, 0, sizeof(clarinet_addr));
            dst->family = CLARINET_AF_INET;
            dst->as.ipv4.u.dword[0] = src->as.ipv4.u.dword[0];
            return CLARINET_ENONE;
        }
        #endif
    }
    
    return CLARINET_EINVAL;
}

int
clarinet_addr_map_to_ipv6(clarinet_addr* restrict dst, 
                          const clarinet_addr* restrict src)
{
    if (dst && src)
    {
        #if CLARINET_ENABLE_IPV6
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
        #endif
    }

    return CLARINET_EINVAL;
}
