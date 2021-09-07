#include "portable/addr.h"

#include <stdio.h>

/* Using inet_ntop/inet_pton as a portable solution for conversion between address and string.
 * Not relying RtlIpv4AddressToStringEx and such on Windows to avoid a dependency on ntdll.lib and not using 
 * using WSAStringToAddress/WSAAddressToString on Windows either because all WSA functions require WSAStartup to be 
 * called first and dynamically load the winsock dll but we want to do that only if/when a socket is actually created
 * as opposed to a simple address-string conversion. 
 * 
 */
 
int
clarinet_addr_to_string(char* restrict dst,
                        size_t dstlen,
                        const clarinet_addr* restrict src)
{   
    if (src && dst && dstlen > 0 && dstlen < INT_MAX) 
    {        
        if (clarinet_addr_is_ipv4(src))
        {
            struct in_addr addr;
            clarinet_addr_ipv4_to_inet(&addr, &src->as.ipv4.u);
            if (inet_ntop(AF_INET, &addr, dst, dstlen))
                return (int)min(strnlen(dst, dstlen), INT_MAX-1);
        }
        #if defined(CLARINET_ENABLE_IPV6) && defined(HAVE_SOCKADDR_IN6_SIN6_ADDR)
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
                        return (int)min(strnlen(dst, dstlen), INT_MAX-1);
                    
                    size_t n = 0;
                    while(n < dstlen && dst[n] != '\0')
                        n++;
                    
                    const size_t size = dstlen - n;
                    const int m = snprintf(&dst[n], size, "%%%u", scope_id);
                    if (m > 0 && (size_t)m < size)
                        return (int)min(n + (size_t)m, INT_MAX-1);
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
    if (dst && src && srclen > 0)
    {
        /* There is no way of knowing if src is an ipv4 or ipv6 we so must try one conversion then the other. */
        int errcode = clarinet_addr_ipv4_from_string(dst, src, srclen);
        #if defined(CLARINET_ENABLE_IPV6) && defined(HAVE_SOCKADDR_IN6_SIN6_ADDR)
        if (errcode != CLARINET_ENONE)
            errcode = clarinet_addr_ipv6_from_string(dst, src, srclen);
        #endif
        return errcode;
    }

    return CLARINET_EINVAL;
}

int
clarinet_endpoint_to_string(char* restrict dst,
                            size_t dstlen,
                            const clarinet_endpoint* restrict src)
{
    if (src && dst && dstlen > 0 && dstlen < INT_MAX) 
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
        
        if (dstlen > reserved)   /* must have enough space to reserve for port (:65535 or []:65535) */
        {               
            int n = clarinet_addr_to_string(dst + offset, dstlen - reserved, &src->addr);
            if (n > 0)
            {                               
                #if defined(CLARINET_ENABLE_IPV6) && defined(HAVE_SOCKADDR_IN6_SIN6_ADDR)
                if (clarinet_addr_is_ipv6(&src->addr))
                {
                    dst[0] = '[';
                    dst[offset + n] = ']';
                    n += offset + 1;
                }
                #endif    
                
                if ((size_t)n < dstlen)
                {
                    const size_t size = dstlen - n;
                    const int m = snprintf(&dst[n], size, ":%u", port);
                    if (m > 0 && (size_t)m < size)
                        return (int)min(n + (size_t)m, INT_MAX-1);
                }
            }
        }
    }

    return CLARINET_EINVAL;
}

CLARINET_STATIC_INLINE 
int
clarinet_decode_port(uint16_t* restrict port, 
                     const char* restrict src, 
                     size_t n)
{
    uint16_t k = 1;
    while (n > 0 && n < 6)
    {
        const char c = src[n-1];
        if (!isdigit(c))
            return CLARINET_EINVAL;
        
        const uint32_t inc = k * (uint16_t)(c - '0');
        if (inc > (uint32_t)(UINT16_MAX - *port)) /* not a valid port number */
            return CLARINET_EINVAL;

        *port += (uint16_t)inc;
        k *= 10;
        n--;
    }
    
    return CLARINET_ENONE;    
}

int 
clarinet_endpoint_from_string(clarinet_endpoint* restrict dst,
                              const char* restrict src,
                              size_t srclen)
{
    if (dst && src && srclen > 0)
    {
        const char first = src[0];
        const char last = src[srclen-1];
        if (isdigit(first) && isdigit(last)) /* either ipv4 or invalid */
        {
            size_t i = 0;
            while(i < srclen) /* consume the address part until a ':' is found */
            {
                const char c = src[i];
                if (c == ':')
                    break;
                /* 
                 * using 15 explicitly here instead of INET_ADDRSTRLEN-1 because some systems define INET_ADDRSTRLEN 
                 * as 22 instead of 16 to account for the port number 
                 */
                if (i >= 15 || (c != '.' && !isdigit(c))) /* not a valid ipv4 endpoint */
                    return CLARINET_EINVAL;                             
                    
                i++;
            }
            
            const size_t n = srclen - i;
            if (n < 2 || n > 6) /* not enough or too much for a valid port number */
                return CLARINET_EINVAL;
                
            uint16_t port = 0;
            int errcode = clarinet_decode_port(&port, &src[i+1], n-1); 
            if (errcode == CLARINET_ENONE)
            {
                clarinet_addr addr;
                errcode = clarinet_addr_ipv4_from_string(&addr, src, i);
                if(errcode == CLARINET_ENONE)             
                {
                    memset(dst, 0, sizeof(clarinet_endpoint));
                    dst->addr = addr;
                    dst->port = port;
                }
            }
            
            return errcode;
        }            
        #if defined(CLARINET_ENABLE_IPV6) && defined(HAVE_SOCKADDR_IN6_SIN6_ADDR)
        else if (first == '[' && isdigit(last)) /* either ipv6 or invalid */
        {
            size_t i = 1;     /* consume '[' */
            while(i < srclen) /* consume the address part until a ']' is found */
            {
                const char c = src[i];
                if (c == ']')
                    break;
                /* 
                 * using 56 explicitly here instead of INET6_ADDRSTRLEN-1 most systems don't account for the scope id 
                 * and may even reserve space for the port instead
                 */
                if (i >= 56 || (c != '.' && c != ':' && c != '%' && !isxdigit(c))) /* not a valid ipv6 endpoint */
                    return CLARINET_EINVAL;
                    
                i++;
            }
            
            const size_t n = srclen - i;
            if (n < 3 || n > 7 || src[i+1] != ':') /* not enough or too much for a valid port number */
                return CLARINET_EINVAL;
                
            uint16_t port = 0;
            int errcode = clarinet_decode_port(&port, &src[i+2], n-2); 
            if (errcode == CLARINET_ENONE)
            {
                clarinet_addr addr;
                errcode = clarinet_addr_ipv6_from_string(&addr, &src[1], i-1);
                if(errcode == CLARINET_ENONE)             
                {
                    memset(dst, 0, sizeof(clarinet_endpoint));
                    dst->addr = addr;
                    dst->port = port;
                }
            }
                
            return errcode;            
        }
        #endif
    }

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
        #if defined(CLARINET_ENABLE_IPV6) && defined(HAVE_SOCKADDR_IN6_SIN6_ADDR)
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
        #if defined(CLARINET_ENABLE_IPV6) && defined(HAVE_SOCKADDR_IN6_SIN6_ADDR)
        else if (src->sa_family == AF_INET6 && srclen >= sizeof(struct sockaddr_in6))
        {
            struct sockaddr_in6* addr = (struct sockaddr_in6*)src;
            memset(addr, 0, sizeof(clarinet_endpoint));
            
            dst->addr.family = CLARINET_AF_INET6;
            dst->addr.as.ipv6.flowinfo = addr->sin6_flowinfo;
            clarinet_addr_ipv6_from_inet6(&dst->addr.as.ipv6.u, &addr->sin6_addr);
            #if defined(HAVE_SOCKADDR_IN6_SIN6_SCOPE_ID)
            dst->addr.as.ipv6.scope_id = addr->sin6_scope_id;
            #endif
            dst->port = addr->sin6_port;           
        }
        #endif        
    }
    
    return CLARINET_EINVAL;
}
