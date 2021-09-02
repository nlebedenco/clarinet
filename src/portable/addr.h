#pragma once
#ifndef PORTABLE_ADDR_H
#define PORTABLE_ADDR_H

#include "portability.h"
#include "clarinet/clarinet.h"

#include <string.h>

#if defined(_WIN32)
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>    
#else /* BSD, Linux, macOS, iOS, Android, PS4, PS5 */
    #include <sys/socket.h>
    #include <netinet/in.h>   
#endif


int 
clarinet_endpoint_to_sockaddr(struct sockaddr* restrict dst,
                              size_t dstlen,
                              const clarinet_endpoint* restrict src);

int 
clarinet_endpoint_from_sockaddr(clarinet_endpoint* restrict dst,
                                const struct sockaddr* restrict src,
                                size_t srclen);


CLARINET_STATIC_INLINE 
void 
clarinet_addr_ipv4_to_inet(struct in_addr* restrict dst, 
                           const union clarinet_addr_ipv4_octets* restrict src)
{
    memcpy(dst, src, min(sizeof(struct in_addr), sizeof(union clarinet_addr_ipv4_octets)));
}

#if defined(HAVE_SOCKADDR_IN6_SIN6_ADDR)
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

#if defined(HAVE_SOCKADDR_IN6_SIN6_ADDR)
CLARINET_STATIC_INLINE 
void 
clarinet_addr_ipv6_from_inet6(union clarinet_addr_ipv6_octets* restrict dst,
                              const struct in6_addr* restrict src) 
{
    memcpy(dst, src, min(sizeof(struct in6_addr), sizeof(union clarinet_addr_ipv6_octets)));
}
#endif

#endif /* PORTABLE_ADDR_H */
