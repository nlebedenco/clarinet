#pragma once
#ifndef PORTABLE_ADDR_H
#define PORTABLE_ADDR_H

#include "portability.h"
#include "clarinet/clarinet.h"

#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <assert.h>

int 
clarinet_endpoint_to_sockaddr(struct sockaddr_storage* restrict dst,
                              const clarinet_endpoint* restrict src);

int 
clarinet_endpoint_from_sockaddr(clarinet_endpoint* restrict dst,
                                const struct sockaddr_storage* restrict src);


CLARINET_STATIC_INLINE 
void 
clarinet_addr_ipv4_to_inet(struct in_addr* restrict dst, 
                           const union clarinet_addr_ipv4_octets* restrict src)
{
    memcpy(dst, src, min(sizeof(struct in_addr), sizeof(union clarinet_addr_ipv4_octets)));
}

#if CLARINET_ENABLE_IPV6 && HAVE_SOCKADDR_IN6_SIN6_ADDR
CLARINET_STATIC_INLINE 
void 
clarinet_addr_ipv6_to_inet6(struct in6_addr* restrict dst, 
                            const union clarinet_addr_ipv6_octets* restrict src)     
{
    memcpy(dst, src, min(sizeof(struct in6_addr), sizeof(union clarinet_addr_ipv6_octets)));
}
#endif

CLARINET_STATIC_INLINE 
void 
clarinet_addr_ipv4_from_inet(union clarinet_addr_ipv4_octets* restrict dst,
                             const struct in_addr* restrict src) 
{
    memcpy(dst, src, min(sizeof(struct in_addr), sizeof(union clarinet_addr_ipv4_octets)));
}

#if CLARINET_ENABLE_IPV6 && HAVE_SOCKADDR_IN6_SIN6_ADDR
CLARINET_STATIC_INLINE 
void 
clarinet_addr_ipv6_from_inet6(union clarinet_addr_ipv6_octets* restrict dst,
                              const struct in6_addr* restrict src) 
{
    memcpy(dst, src, min(sizeof(struct in6_addr), sizeof(union clarinet_addr_ipv6_octets)));
}
#endif

CLARINET_STATIC_INLINE
int
clarinet_addr_ipv4_from_string(clarinet_addr* restrict dst,
                          const char* restrict src,
                          size_t srclen)
{
    /* 
     * not validating parameters to avoid redundat checks. 
     * caller must ensure dst and src are not null and srclen > 0.
     */
    assert(dst != NULL);
    assert(src != NULL);
    
    if (srclen >= 7) /* minimum ipv4 is 0.0.0.0 */
    {
        /* Use a temp buffer to ensure the string is null-terminated. INET_ADDRSTRLEN already accounts for the 
         * nul-termination character but we still add one more position to be able to detect when src contains anything
         * beyond the max address string so that "255.255.255.255!", "255.255.255.255!!", etc... can be be rejected.
         */
        char s[INET_ADDRSTRLEN+1] = {0};
        const size_t m = min(srclen, sizeof(s) - 1); /* srclen does not count the null-termination char */
        strlcpy(s, src, m + 1);                      /* size here must include the nul-termination char */        
        struct in_addr addr = {0};
        const int errcode = inet_pton(AF_INET, s, &addr);
        if (errcode == 1)
        {
            memset(dst, 0, sizeof(clarinet_addr));
            dst->family = CLARINET_AF_INET;
            clarinet_addr_ipv4_from_inet(&dst->as.ipv4.u, &addr);
            return CLARINET_ENONE;
        }
    }
    return CLARINET_EINVAL;
}

#if CLARINET_ENABLE_IPV6 && HAVE_SOCKADDR_IN6_SIN6_ADDR
CLARINET_STATIC_INLINE
int
clarinet_addr_ipv6_from_string(clarinet_addr* restrict dst,
                               const char* restrict src,
                               size_t srclen)
{
    /* 
     * not validating parameters to avoid redundat checks. 
     * caller must ensure dst and src are not null and srclen > 0.
     */
    assert(dst != NULL);
    assert(src != NULL);
    assert(srclen > 0);
    
    /*
     * Unfortunately inet_pton does not support scope id so we have to try to parse it first 
     * and split the address string.
     */

    if (src[srclen - 1] == '%') /* empty scope id is never valid */
        return CLARINET_EINVAL;
    
    uint8_t digits = 0;
    uint32_t k = 1;
    uint32_t scope_id = 0;            
    size_t n = srclen;
    uint32_t inc = 0;
    while (n > 0) 
    {
        const char c = src[n-1];
        if (isdigit(c) && digits < 10)
        {
            inc = k * (uint32_t)(c - '0');
            if (inc > (UINT32_MAX - scope_id)) /* neither a valid scope id nor an ipv6 address field */
                return CLARINET_EINVAL;

            scope_id += inc;
            k *= 10;
            digits++;
        }
        else if (c == '%') /* end of the scope id */
        { 
            if (inc == 0 && digits > 1) /* leading digit cannot be a zero unless this is a single-0 number */
                return CLARINET_EINVAL;
                
            n--; /* consume the '%' */
            break;
        }
        else /* not a valid scope id, maybe it was an ipv6 address field */
        {
            scope_id = 0;
            n = srclen;
            break;
        }
        n--;
    }
    
    /* At this point scope_id contains a valid value and n is the size of the REMAINING ipv6 address */
    if (n >= 2) /* a minimum ipv6 string is "::" */
    {
        /* Use a temp buffer to ensure the string is null-terminated. INET6_ADDRSTRLEN already accounts for the 
         * nul-termination character but we still add one more position to be able to detect when src contains anything
         * beyond the max address string so that "b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf!", 
         * "b0b1:b2b3:b4b5:b6b7:b8b9:babb:bcbd:bebf!!", etc... can be be rejected.
         */
        char s[INET6_ADDRSTRLEN+1] = {0};
        const size_t m = min(n, sizeof(s) - 1); /* srclen does not count the null-termination char */
        strlcpy(s, src, m + 1);                 /* size here must include the nul-termination char */
        struct in6_addr addr = {0};
        int errcode = inet_pton(AF_INET6, s, &addr);
        if (errcode == 1)
        {
            memset(dst, 0, sizeof(clarinet_addr));
            dst->family = CLARINET_AF_INET6;
            clarinet_addr_ipv6_from_inet6(&dst->as.ipv6.u, &addr);
            dst->as.ipv6.scope_id = scope_id;
            return CLARINET_ENONE;
        }
    }
    
    return CLARINET_EINVAL;
}    
#endif

#endif /* PORTABLE_ADDR_H */
