#include "platforms/windows/addr.h"

#include <stdlib.h>
#include <ip2string.h>

CLARINET_STATIC_INLINE 
void 
clarinet_addr_ipv4_to_inet(struct in_addr* CLARINET_RESTRICT dst, 
                           const clarinet_addr_ipv4* CLARINET_RESTRICT src)
{
    memcpy(dst, src, MIN(sizeof(in_addr), sizeof(clarinet_addr_ipv4)));
}

#if defined(HAVE_SOCKADDR_IN6_SIN6_ADDR)
CLARINET_STATIC_INLINE 
void 
clarinet_addr_ipv6_to_inet6(struct in6_addr* CLARINET_RESTRICT dst, 
                            const clarinet_addr_ipv6* CLARINET_RESTRICT src)     
{
    memcpy(dst, src, MIN(sizeof(in6_addr), sizeof(clarinet_addr_ipv6)));
}
#endif

CLARINET_STATIC_INLINE 
void 
clarinet_addr_ipv4_from_inet(clarinet_addr_ipv4* CLARINET_RESTRICT dst
                             const struct in_addr* CLARINET_RESTRICT src) 
{
    memcpy(dst, src, MIN(sizeof(in_addr), sizeof(clarinet_addr_ipv4)));
}

#if defined(HAVE_SOCKADDR_IN6_SIN6_ADDR)
CLARINET_STATIC_INLINE 
void 
clarinet_addr_ipv6_from_inet6(clarinet_addr_ipv6* CLARINET_RESTRICT dst
                              const struct in6_addr* CLARINET_RESTRICT src) 
{
    memcpy(dst, src, MIN(sizeof(in6_addr), sizeof(clarinet_addr_ipv6)));
}
#endif

int
clarinet_endpoint_to_sockaddr(struct sockaddr* CLARINET_RESTRICT dst,
                              size_t dstlen,
                              const clarinet_endpoint* CLARINET_RESTRICT src)
{
    if (src && dst)
    {   
        if (src->addr.family == CLARINET_AF_INET && dstlen >= sizeof(struct sockaddr_in))
        {
            struct sockaddr_in* addr = (struct sockaddr_in*)dst;
            memset(addr, 0, sizeof(struct sockaddr_in));
            #if defined(HAVE_STRUCT_SOCKADDR_SA_LEN)
            add->sa_len = sizeof(struct sockaddr_in);
            #endif
            addr->family = AF_INET;
            addr->port = src->port;
            clarinet_addr_ipv6_to_inet6(&addr->sin_addr, &src->addr.ipv4);
            
            return CLARINET_ENONE;
        }
        #if defined(HAVE_SOCKADDR_IN6_SIN6_ADDR)
        else if (src->addr.family == CLARINET_AF_INET6 && dstlen >= sizeof(struct sockaddr_in6))
        {
            struct sockaddr_in6* addr = (struct sockaddr_in6*)dst;
            memset(addr, 0, sizeof(struct sockaddr_in6));
            #if defined(HAVE_STRUCT_SOCKADDR_SA_LEN)
            add->sa_len = sizeof(struct sockaddr_in6);
            #endif
            addr->family = AF_INET6;
            addr->port = src->port;
            addr->sin6_flowinfo = src->addr.ipv6.flowinfo;
            clarinet_addr_ipv6_to_inet6(&addr->sin6_addr, &src->addr.ipv6);
            #if defined(HAVE_SOCKADDR_IN6_SIN6_SCOPE_ID)
            addr->sin6_scope_id = src->addr.ipv6.scope_id;
            #endif
            
            return CLARINET_ENONE;
        }
        #endif
    }
    
    return CLARINET_EINVAL;
}

int
clarinet_endpoint_from_sockaddr(clarinet_endpoint* CLARINET_RESTRICT dst,
                                const struct sockaddr* CLARINET_RESTRICT src,
                                size_t srclen)
{
    if (src && dst)
    { 
        if (src->family == AF_INET && srclen >= sizeof(struct sockaddr_in))
        {
            struct sockaddr_in* addr = (struct sockaddr_in*)src;
            memset(addr, 0, sizeof(clarinet_endpoint));
            
            dst->addr.family = src->family;
            clarinet_addr_ipv4_from_inet(&dst->addr.ipv4, &addr->sin_addr)
            dst->port = src->port;
        }
        #if defined(HAVE_SOCKADDR_IN6_SIN6_ADDR)
        else if (src->family == AF_INET6 && srclen >= sizeof(struct sockaddr_in6))
        {
            struct sockaddr_in6* addr = (struct sockaddr_in6*)src;
            memset(addr, 0, sizeof(clarinet_endpoint));
            dst->addr.flowinfo = addr->sin6_flowinfo;
            clarinet_addr_ipv4_from_inet(&dst->addr.ipv6, &addr->sin6_addr)
            dst->addr.scope_id = addr->sin6_scope_id;
            dst->port = src->port;           
        }
        #endif        
    }
    
    return CLARINET_EINVAL;
}

int
clarinet_addr_to_string(char* CLARINET_RESTRICT dst,
                        size_t dstlen,
                        const clarinet_addr* CLARINET_RESTRICT src)
{   
    /* On windows the functions required to convert an address (IPv4 or IPv6) to string are the same used to convert
     * an endpoint. The address is just a particular case of endpoint where the port is zero. 
     */
    if (src && dst) 
    {        
        clarinet_endpoint endpoint;
        memcpy(&endpoint.addr, src, sizeof(clarinet_addr));
        return clarinet_endpoint_to_string(dst, dstlen, &endpoint);
    }

    return CLARINET_EINVAL;
}                            


int 
clarinet_addr_from_string(clarinet_addr* CLARINET_RESTRICT dst,
                          const char* CLARINET_RESTRICT src,
                          size_t srclen)
{
    /* On windows the functions required to parse an address (IPv4 or IPv6) from string are the same used to parse
     * an endpoint. The address is just a particular case of endpoint without port information in which case the port 
     * defaults to zero. 
     */
    clarient_endpoint endpoint;
    int errcode = clarinet_endpoint_from_string(&endpoint, src, srclen);
    if (errcode == CLARINET_ENONE)
        mempcy(dst, &endpoint.addr, sizeof(clarinet_addr));
    
    return errcode;   
}

int
clarinet_endpoint_to_string(char* CLARINET_RESTRICT dst,
                            size_t dstlen,
                            const clarinet_endpoint* CLARINET_RESTRICT src)
{
    if (src && dst) 
    {
        if (src->family == CLARINET_AF_INET)
        {
            in_addr addr;
            clarinet_addr_ipv4_to_inet(&addr, &src->addr.ipv4);
            const int errcode = RtlIpv4AddressToStringExA(&addr, src->port, dst, &dstlen);           
            if (errcode == STATUS_SUCCESS)
                return dstlen;
        }
        #if defined(HAVE_SOCKADDR_IN6_SIN6_ADDR)
        else if (src->family == CLARINET_AF_INET6)
        {
            in6_addr addr;         
            clarinet_addr_ipv6_to_inet6(&addr, &src->addr.ipv6);
            const uint32_t scope_id = src->addr.ipv6.scope_id;
            const int errcode = RtlIpv6AddressToStringExA(&addr, scope_id, src->port, dst, &dstlen);    
            if (errcode == STATUS_SUCCESS)
                return dstlen;
        }
        #endif
    }
    
    return CLARINET_EINVAL;
}

int 
clarinet_endpoint_from_string(clarinet_endpoint* CLARINET_RESTRICT dst,
                              const char* CLARINET_RESTRICT src,
                              size_t srclen)
{
    if (dst && src && srclen)
    {        
        #if defined(HAVE_SOCKADDR_IN6_SIN6_ADDR)
        in6_addr addr;
        #else
        in_addr addr;
        #endif
        uint16_t port = 0;
        
        /* Ensure the string passed to RtlIpv4StringToAddressExA/RtlIpv6StringToAddressExA is nul-terminated.
         * INET_ADDRSTRLEN and INET6_ADDRSTRLEN should be at least 16 and 57 respectively. On win sdk 10.0.14393.0 they 
         * are defined in ws2ipdef.h as:
         *
         * #define INET_ADDRSTRLEN  22
         * #define INET6_ADDRSTRLEN 65
         *
         * The totals are derived from the following data:
         *  15: IPv4 address
         *  45: IPv6 address including embedded IPv4 address
         *  11: Scope Id
         *   2: Brackets around IPv6 address when port is present
         *   6: Port (including colon)
         *   1: Terminating null byte
         */
        #if defined(HAVE_SOCKADDR_IN6_SIN6_ADDR)
        char s[INET6_ADDRSTRLEN];
        #else
        char s[INET_ADDRSTRLEN];
        #endif
        const size_t n = MIN(sizeof(s), srclen);
        memcpy(s, src, n);
        s[n-1] = '\0';
        
        /* There is no way to know which family the string represents unless we try to parse so first try IPv4 which is 
         * simpler and more common. 
         */        
        const int errcode = RtlIpv4StringToAddressExA(s, true, (in_addr*)&addr, &port);
        if (errcode == STATUS_SUCCESS)
        {
            memset(dst, 0, sizeof(clarinet_endpoint));
            dst->addr.family = CLARINET_AF_INET;
            clarinet_addr_inet_to_ipv4(&dst->addr.ipv4, (in_addr*)&addr);
            dst->port = port;        
            return CLARINET_ENONE;
        }
        
        #if defined(HAVE_SOCKADDR_IN6_SIN6_ADDR)
        /* If IPv4 didn't work try to parse an IPv6. */
        uint32_t scope_id = 0;
        const int errcode = RtlIpv6StringToAddressExA(s, &addr, &scope_id, &port);
        if (errcode == STATUS_SUCCESS)
        {
            memset(dst, 0, sizeof(clarinet_endpoint));
            dst->addr.family = CLARINET_AF_INET6;            
            clarinet_addr_inet6_to_ipv6(&dst->addr.ipv6, addr);
            dst->addr.ipv6.scope_id = scope_id;
            dst->port = port;
            return CLARINET_ENONE;
        }
        #endif
    }
    
    return CLARINET_EINVAL;
}
