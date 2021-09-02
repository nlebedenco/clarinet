#include "portable/addr.h"

#include <stdio.h>
#include <stdlib.h>

/* Using inet_ntop/inet_pton for conversion betwwen address and string.
 * 
 * Not using RtlIpv4AddressToStringEx and such on Windows because those functions require ntdll.lib.
 * Not using WSAStringToAddress/WSAAddressToString on Windows either because they require WSAStartup to be called first 
 * and we only do that when a socket is created. 
 */

int
clarinet_addr_to_string(char* restrict dst,
                        size_t dstlen,
                        const clarinet_addr* restrict src)
{   
    if (src && dst && dstlen && dstlen < INT_MAX) 
    {        
        if (clarinet_addr_is_ipv4(src))
        {
            struct in_addr addr;
            clarinet_addr_ipv4_to_inet(&addr, &src->as.ipv4.u);
            if (inet_ntop(AF_INET, &addr, dst, dstlen))
                return (int)min(strnlen(dst, dstlen), INT_MAX);
        }
        #if defined(HAVE_SOCKADDR_IN6_SIN6_ADDR)
        else if (clarinet_addr_is_ipv6(src))
        {
            if (dstlen >= 11) /* must have enough space to reserve for scope_id (%4294967296) */
            {
                struct in6_addr addr;
                clarinet_addr_ipv6_to_inet6(&addr, &src->as.ipv6.u);
                if (inet_ntop(AF_INET6, &addr, dst, dstlen - 11))
                {
                    const uint32_t scope_id = src->as.ipv6.scope_id;
                    if (scope_id == 0)
                        return (int)min(strnlen(dst, dstlen), INT_MAX);
                    
                    size_t n = 0;
                    while(n < dstlen && dst[n] != '\0')
                        n++;
                    
                    const size_t size = dstlen - n;
                    const int m = snprintf(&dst[n], size, "%%%u", scope_id);
                    if (m > 0 && (size_t)m < size)
                        return (int)min(n + (size_t)m + 1, INT_MAX);
                }
            }
        }
        #endif
    }

    return CLARINET_EINVAL;
}                            


int 
clarinet_addr_from_string(clarinet_addr* restrict dst,
                          const char* restrict src,
                          size_t srclen)
{
    clarinet_endpoint endpoint;
    int errcode = clarinet_endpoint_from_string(&endpoint, src, srclen);
    if (errcode == CLARINET_ENONE)
        memcpy(dst, &endpoint.addr, sizeof(clarinet_addr));
    
    return errcode;   
}

int
clarinet_endpoint_to_string(char* restrict dst,
                            size_t dstlen,
                            const clarinet_endpoint* restrict src)
{
     if (src && dst && dstlen && dstlen < INT_MAX) 
    {           
        const uint16_t port = src->port;
        
        int offset;         /* for the '[' when it's an ipv6 */
        size_t reserved;    /* for the port number */ 
        if (clarinet_addr_is_ipv6(&src->addr))
        {    
            offset = 1;
            reserved = 8;
        }
        else
        {
            offset = 0;
            reserved = 6;
        }
        
        if (dstlen >= reserved)   /* must have enough space to reserve for port (:65535 or []:65535) */
        {               
            int n = clarinet_addr_to_string(dst + offset, dstlen - reserved, &src->addr);
            if (n > 0)
            {                               
                #if defined(HAVE_SOCKADDR_IN6_SIN6_ADDR)         
                if (clarinet_addr_is_ipv6(&src->addr))
                {
                    dst[0] = '[';
                    dst[n] = ']';
                    n++;
                }
                #endif    
                
                if (dstlen > (size_t)n)
                {
                    const size_t size = dstlen - n;
                    const int m = snprintf(&dst[n-1], size, ":%u", port);
                    if (m > 0 && (size_t)m < size)
                        return (int)min(n + (size_t)m + 1, INT_MAX);
                }
            }
        }
    }

    return CLARINET_EINVAL;
}

int 
clarinet_endpoint_from_string(clarinet_endpoint* restrict dst,
                              const char* restrict src,
                              size_t srclen)
{
    CLARINET_IGNORE(dst);
    CLARINET_IGNORE(src);
    CLARINET_IGNORE(srclen);
    // TODO
    
    return CLARINET_EINVAL;
}

int
clarinet_endpoint_to_sockaddr(struct sockaddr* restrict dst,
                              size_t dstlen,
                              const clarinet_endpoint* restrict src)
{
    if (src && dst)
    {   
        if (clarinet_addr_is_ipv4(&src->addr) && dstlen >= sizeof(struct sockaddr_in))
        {
            struct sockaddr_in* addr = (struct sockaddr_in*)dst;
            memset(addr, 0, sizeof(struct sockaddr_in));
            
            #if defined(HAVE_STRUCT_SOCKADDR_SA_LEN)
            add->sa_len = sizeof(struct sockaddr_in);
            #endif
            addr->sin_family = AF_INET;
            addr->sin_port = src->port;
            clarinet_addr_ipv4_to_inet(&addr->sin_addr, &src->addr.as.ipv4.u);
            
            return CLARINET_ENONE;
        }
        #if defined(HAVE_SOCKADDR_IN6_SIN6_ADDR)
        else if (clarinet_addr_is_ipv6(&src->addr) && dstlen >= sizeof(struct sockaddr_in6))
        {
            struct sockaddr_in6* addr = (struct sockaddr_in6*)dst;
            memset(addr, 0, sizeof(struct sockaddr_in6));
            
            #if defined(HAVE_STRUCT_SOCKADDR_SA_LEN)
            add->sa_len = sizeof(struct sockaddr_in6);
            #endif
            addr->sin6_family = AF_INET6;
            addr->sin6_port = src->port;
            addr->sin6_flowinfo = src->addr.as.ipv6.flowinfo;
            clarinet_addr_ipv6_to_inet6(&addr->sin6_addr, &src->addr.as.ipv6.u);
            #if defined(HAVE_SOCKADDR_IN6_SIN6_SCOPE_ID)
            addr->sin6_scope_id = src->addr.as.ipv6.scope_id;
            #endif
            
            return CLARINET_ENONE;
        }
        #endif
    }
    
    return CLARINET_EINVAL;
}

int
clarinet_endpoint_from_sockaddr(clarinet_endpoint* restrict dst,
                                const struct sockaddr* restrict src,
                                size_t srclen)
{
    if (src && dst)
    { 
        if (src->sa_family == AF_INET && srclen >= sizeof(struct sockaddr_in))
        {
            struct sockaddr_in* addr = (struct sockaddr_in*)src;
            memset(addr, 0, sizeof(clarinet_endpoint));
            
            dst->addr.family = CLARINET_AF_INET;
            clarinet_addr_ipv4_from_inet(&dst->addr.as.ipv4.u, &addr->sin_addr);
            dst->port = addr->sin_port;
        }
        #if defined(HAVE_SOCKADDR_IN6_SIN6_ADDR)
        else if (src->sa_family == AF_INET6 && srclen >= sizeof(struct sockaddr_in6))
        {
            struct sockaddr_in6* addr = (struct sockaddr_in6*)src;
            memset(addr, 0, sizeof(clarinet_endpoint));
            
            dst->addr.family = CLARINET_AF_INET6;
            dst->addr.as.ipv6.flowinfo = addr->sin6_flowinfo;
            clarinet_addr_ipv6_from_inet6(&dst->addr.as.ipv6.u, &addr->sin6_addr);
            dst->addr.as.ipv6.scope_id = addr->sin6_scope_id;
            dst->port = addr->sin6_port;           
        }
        #endif        
    }
    
    return CLARINET_EINVAL;
}
