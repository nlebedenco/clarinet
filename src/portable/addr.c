#include "portable/addr.h"

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <assert.h>


CLARINET_STATIC_INLINE
void
clarinet_addr_ipv4_to_inet(struct in_addr* restrict dst,
                           const union clarinet_ipv4_octets* restrict src)
{
    memcpy(dst, src, min(sizeof(struct in_addr), sizeof(union clarinet_ipv4_octets)));
}

#if CLARINET_ENABLE_IPV6 && HAVE_SOCKADDR_IN6_SIN6_ADDR

CLARINET_STATIC_INLINE
void
clarinet_addr_ipv6_to_inet6(struct in6_addr* restrict dst,
                            const union clarinet_ipv6_octets* restrict src)
{
    memcpy(dst, src, min(sizeof(struct in6_addr), sizeof(union clarinet_ipv6_octets)));
}

#endif

CLARINET_STATIC_INLINE
void
clarinet_addr_ipv4_from_inet(union clarinet_ipv4_octets* restrict dst,
                             const struct in_addr* restrict src)
{
    memcpy(dst, src, min(sizeof(struct in_addr), sizeof(union clarinet_ipv4_octets)));
}

#if CLARINET_ENABLE_IPV6 && HAVE_SOCKADDR_IN6_SIN6_ADDR

CLARINET_STATIC_INLINE
void
clarinet_addr_ipv6_from_inet6(union clarinet_ipv6_octets* restrict dst,
                              const struct in6_addr* restrict src)
{
    memcpy(dst, src, min(sizeof(struct in6_addr), sizeof(union clarinet_ipv6_octets)));
}

#endif

static
int
clarinet_decode_port(const char* restrict src,
                     size_t len)
{
    assert(src != NULL);
    assert(len > 0);

    size_t i = 0;
    int port = 0;
    int hasdigits = 0;
    while (i < len)
    {
        const char c = src[i++];
        if (!isdigit(c))
            return CLARINET_EINVAL;

        if (hasdigits)
        {
            if (port == 0)
                return CLARINET_EINVAL;
        }
        else
        {
            hasdigits = 1;
        }

        int value = port * 10 + (c - '0');
        if (value > UINT16_MAX)
            return CLARINET_EINVAL;

        port = value;
    }

    return port;
}

static
int
clarinet_decode_scope_id(uint32_t* scope_id,
                         const char* restrict src,
                         size_t len)
{
    assert(scope_id != NULL);
    assert(src != NULL);

    size_t i = 0;
    uint32_t sid = 0;
    int hasdigits = 0;
    while (i < len)
    {
        const char c = src[i++];
        if (!isdigit(c))
            return CLARINET_EINVAL;

        if (hasdigits)
        {
            if (sid == 0)
                return CLARINET_EINVAL;
        }
        else
        {
            hasdigits = 1;
        }

        /* must be careful not to overflow a uint32_t during the calculation while avoiding the use of a uint64_t */
        if (sid > (UINT32_MAX / 10))
            return CLARINET_EINVAL;

        uint32_t value = sid * 10;
        uint32_t inc = (uint32_t)(c - '0');
        if (inc > (UINT32_MAX - value))
            return CLARINET_EINVAL;

        sid = value + inc;
    }

    *scope_id = sid;
    return CLARINET_ENONE;
}


/* Using inet_ntop as the portable solution for converting address to string.
 * Not relying RtlIpv4AddressToStringEx/RtlIpv6AddressToStringEx on Windows to avoid a dependency on ntdll.lib and not
 * using using WSAStringToAddress/WSAAddressToString because all WSA functions require WSAStartup to be called first and
 * dynamically load the winsock dll but we want to do that only if/when a socket is actually created as opposed to a
 * simple address-string conversion.
 *
 * Using a custom solution for string to address. The problem with inet_pton is consistency across platforms. For
 * example, "127.0.0.001" is invalid on Windows and Linux but valid on macOS. "::FFFF:127.0.0.001" is valid on Windows
 * and invalid on Linux and macOS. Besides there is the complication that the scope_id is on macOS is parsed but can be
 * an iface name and is encoded as part of the address while on Linux and Windows the scope id must be parsed separately.
 *
 */

#define INADDRSZ 4

static
int
clarinet_pton4(struct in_addr* restrict addr,
               const char* restrict src,
               size_t srclen)
{
    assert(addr != NULL);
    assert(src != NULL);

    uint8_t* addrptr = (uint8_t*)addr;
    size_t i = 0;
    size_t octet = 0;
    int hasdigits = 0;
    while (i < srclen)
    {
        const char c = src[i++];
        if (isdigit(c))
        {
            uint8_t acc = addrptr[octet];
            if (hasdigits)
            {
                if (acc == 0)
                    return CLARINET_EINVAL;
            }
            else
            {
                hasdigits = 1;
            }

            int value = acc * 10 + (c - '0');
            if (value > UINT8_MAX)
                return CLARINET_EINVAL;

            addrptr[octet] = (uint8_t)value;
        }
        else if (c == '.')
        {
            if (octet == 3)
                return CLARINET_EINVAL;

            octet++;
            hasdigits = 0;
        }
        else
        {
            return CLARINET_EINVAL;
        }
    }

    if (octet < 3)
        return CLARINET_EINVAL;

    return CLARINET_ENONE;
}

#if CLARINET_ENABLE_IPV6 && HAVE_SOCKADDR_IN6_SIN6_ADDR

    #define IN6ADDRSZ 16

static
int
clarinet_pton6(struct in6_addr* restrict addr,
               const char* restrict src,
               size_t srclen)
{
    assert(addr != NULL);
    assert(src != NULL);
    assert(srclen >= 2);

    static const char xdigits_l[] = "0123456789abcdef";
    static const char xdigits_u[] = "0123456789ABCDEF";


    uint8_t tmp[IN6ADDRSZ] = { 0 };
    uint8_t* tp = (uint8_t*)tmp;
    uint8_t* endp = tp + sizeof(tmp);
    uint8_t* colonp = NULL;

    size_t i = 0;
    if (src[i] == ':')
        if (src[++i] != ':')
            return CLARINET_EINVAL;

    size_t curtok = i;
    int seen_xdigits = 0;
    uint32_t val = 0;

    while (i < srclen)
    {
        const char c = src[i++];
        const char* xdigits;
        const char* cp;
        if ((cp = strchr((xdigits = xdigits_l), c)) == NULL)
            cp = strchr((xdigits = xdigits_u), c);

        if (cp)
        {
            val <<= 4;
            val = val | (uint32_t)(cp - xdigits);
            if (++seen_xdigits > 4)
                return CLARINET_EINVAL;
            continue;
        }

        if (c == ':')
        {
            curtok = i;
            if (!seen_xdigits)
            {
                if (colonp)
                    return CLARINET_EINVAL;
                colonp = tp;
                continue;
            }
            else if (i == srclen)
            {
                return CLARINET_EINVAL;
            }

            if (tp + sizeof(uint16_t) > endp)
                return CLARINET_EINVAL;

            *tp++ = (uint8_t)(val >> 8) & 0xff;
            *tp++ = (uint8_t)val & 0xff;
            seen_xdigits = 0;
            val = 0;
            continue;
        }

        if (c == '.' && ((tp + INADDRSZ) <= endp) && clarinet_pton4((struct in_addr*)tp, &src[curtok], srclen - curtok) == CLARINET_ENONE)
        {
            tp += sizeof(struct in_addr);
            seen_xdigits = 0;
            break;
        }

        return CLARINET_EINVAL;
    }

    if (seen_xdigits)
    {
        if (tp + sizeof(uint16_t) > endp)
            return CLARINET_EINVAL;

        *tp++ = (uint8_t)(val >> 8) & 0xff;
        *tp++ = (uint8_t)val & 0xff;
    }

    if (colonp)
    {
        if (tp == endp)
            return CLARINET_EINVAL;

        const size_t n = (size_t)(tp - colonp);
        for (i = 1; i <= n; i++)
        {
            *(endp - i) = colonp[n - i];
            colonp[n - i] = 0;
        }
        tp = endp;
    }

    if (tp != endp)
        return CLARINET_EINVAL;

    memcpy(addr, tmp, sizeof(tmp));
    return CLARINET_ENONE;
}

#endif


static
int
clarinet_addr_ipv4_from_string(clarinet_addr* restrict dst,
                               const char* restrict src,
                               size_t srclen)
{
    /* Not validating parameters to avoid redundat checks. Caller must ensure dst and src are not null. */
    assert(dst != NULL);
    assert(src != NULL);

    if (srclen < 7) /* minimum ipv4 is 0.0.0.0 */
        return CLARINET_EINVAL;

    struct in_addr addr = { 0 };
    int errcode = clarinet_pton4(&addr, src, srclen);
    if (errcode != CLARINET_ENONE)
        return errcode;

    memset(dst, 0, sizeof(clarinet_addr));
    dst->family = CLARINET_AF_INET;
    clarinet_addr_ipv4_from_inet(&dst->as.ipv4.u, &addr);

    return CLARINET_ENONE;
}


#if CLARINET_ENABLE_IPV6 && HAVE_SOCKADDR_IN6_SIN6_ADDR

static
int
clarinet_addr_ipv6_from_string(clarinet_addr* restrict dst,
                               const char* restrict src,
                               size_t srclen)
{
    /* Not validating parameters to avoid redundat checks. Caller must ensure dst and src are not null. */
    assert(dst != NULL);
    assert(src != NULL);

    if (srclen < 2) /* minimum ipv6 is :: */
        return CLARINET_EINVAL;

    /* Empty scope id is not valid */
    if (src[srclen - 1] == '%')
        return CLARINET_EINVAL;

    /* Parse scope id first */
    size_t i = 0;
    while (i < srclen)
    {
        if (src[i++] == '%')
            break;
    }

    if (i < 2) /* minimum ipv6 with scope id is ::%0 */
        return CLARINET_EINVAL;

    uint32_t scope_id = 0;
    if (i < srclen)
    {
        int errcode = clarinet_decode_scope_id(&scope_id, &src[i], srclen - i);
        if (errcode != CLARINET_ENONE)
            return errcode;
        --i;
    }

    /* Parse inet6 address */
    struct in6_addr addr;
    /* macOS doesn't like when we do in6_addr addr = {0} */
    memset(&addr, 0, sizeof(addr));
    int errcode = clarinet_pton6(&addr, src, i);
    if (errcode != CLARINET_ENONE)
        return errcode;

    memset(dst, 0, sizeof(clarinet_addr));
    dst->family = CLARINET_AF_INET6;
    clarinet_addr_ipv6_from_inet6(&dst->as.ipv6.u, &addr);
    dst->as.ipv6.scope_id = scope_id;
    return CLARINET_ENONE;
}

#endif

int
clarinet_addr_to_string(char* restrict dst,
                        size_t dstlen,
                        const clarinet_addr* restrict src)
{
    if (src && dst && dstlen > 0 && dstlen <= INT_MAX)
    {
        if (clarinet_addr_is_ipv4(src))
        {
            struct in_addr addr;
            clarinet_addr_ipv4_to_inet(&addr, &src->as.ipv4.u);
            if (inet_ntop(AF_INET, &addr, dst, (socklen_t)dstlen))
            {
                const size_t limit = dstlen - 1;
                const size_t n = strnlen(dst, limit);
                if (n == limit) // sanity: ensure dst is nul-terminated (normally shouldn't be necessary)
                    dst[n] = '\0';
                return (int)n;
            }
        }
#if CLARINET_ENABLE_IPV6 && HAVE_SOCKADDR_IN6_SIN6_ADDR
        else if (clarinet_addr_is_ipv6(src))
        {
            if (dstlen >= 11) /* must have enough space to reserve for scope_id (%4294967295) */
            {
                struct in6_addr addr;
                clarinet_addr_ipv6_to_inet6(&addr, &src->as.ipv6.u);
    #if defined(_WIN32)
                /* on windows inet_ntop expects a size_t length not a socklen_t */
                const size_t sublen = dstlen - 11;
    #else
                const socklen_t sublen = (socklen_t)(dstlen - 11);
    #endif
                if (inet_ntop(AF_INET6, &addr, dst, sublen))
                {
                    const uint32_t scope_id = src->as.ipv6.scope_id;
                    if (scope_id == 0)
                    {
                        const size_t limit = dstlen - 12;
                        const size_t n = strnlen(dst, limit);
                        if (n == limit) // sanity: ensure dst is nul-terminated (normally shouldn't be necessary)
                            dst[n] = '\0';
                        return (int)n;
                    }
                    else
                    {
                        size_t n = 0;
                        while (n < dstlen && dst[n] != '\0')
                        {
                            n++;
                        }

                        const size_t size = dstlen - n;
                        const int m = snprintf(&dst[n], size, "%%%u", scope_id);
                        if (m > 0 && (size_t)m < size)
                            return (int)n + m;
                    }
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
        /* There is no way of knowing if src is an ipv4 or ipv6 upfront so we must try one conversion then the other. */
        int errcode = clarinet_addr_ipv4_from_string(dst, src, srclen);
    #if CLARINET_ENABLE_IPV6 && HAVE_SOCKADDR_IN6_SIN6_ADDR
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
    if (src && dst && dstlen > 0 && dstlen <= INT_MAX)
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
    #if CLARINET_ENABLE_IPV6 && HAVE_SOCKADDR_IN6_SIN6_ADDR
                if (clarinet_addr_is_ipv6(&src->addr))
                {
                    dst[0] = '[';
                    dst[offset + n] = ']';
                    n += offset + 1;
                }
    #endif

                if ((size_t)n < dstlen)
                {
                    const size_t size = dstlen - (size_t)n;
                    const int m = snprintf(&dst[n], size, ":%u", port);
                    if (m > 0 && (size_t)m < size)
                        return (int)n + m;
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
    if (dst && src && srclen > 0)
    {
        const char first = src[0];
        const char last = src[srclen - 1];
        if (isdigit(first) && isdigit(last)) /* either ipv4 or invalid */
        {
            size_t i = 0;
            while (i < srclen) /* consume the address part until a ':' is found */
            {
                const char c = src[i];
                if (c == ':')
                    break;
                /*
                 * using 15 explicitly here instead of INET_ADDRSTRLEN-1 because some systems define INET_ADDRSTRLEN
                 * as 22 or more instead of 16 to account for the port number (eg.: windows)
                 */
                if (i >= 15 || (c != '.' && !isdigit(c))) /* not a valid ipv4 endpoint */
                    return CLARINET_EINVAL;

                i++;
            }

            const size_t n = srclen - i;
            if (n < 2) /* not enough for a valid port number */
                return CLARINET_EINVAL;

            int errcode = clarinet_decode_port(&src[i + 1], n - 1);
            if (errcode < 0)
                return errcode;

            const uint16_t port = (uint16_t)errcode;
            clarinet_addr addr = { 0 };
            errcode = clarinet_addr_ipv4_from_string(&addr, src, i);
            if (errcode == CLARINET_ENONE)
            {
                memset(dst, 0, sizeof(clarinet_endpoint));
                dst->addr = addr;
                dst->port = port;
            }

            return errcode;
        }
#if CLARINET_ENABLE_IPV6 && HAVE_SOCKADDR_IN6_SIN6_ADDR
        else if (first == '[' && isdigit(last)) /* either ipv6 or invalid */
        {
            size_t i = 1;     /* consume '[' */
            while (i < srclen) /* consume the address part until a ']' is found */
            {
                const char c = src[i];
                if (c == ']')
                    break;
                /*
                 * using 56 explicitly here instead of INET6_ADDRSTRLEN-1 most systems don't account for the scope id
                 * and may even reserve space for the port instead (e.g.: windows)
                 */
                if (i >= 56 || (c != '.' && c != ':' && c != '%' && !isxdigit(c))) /* not a valid ipv6 endpoint */
                    return CLARINET_EINVAL;

                i++;
            }

            const size_t n = srclen - i;
            if (n < 3 || src[i + 1] != ':') /* not enough for a valid port number or separator mismatch */
                return CLARINET_EINVAL;

            int errcode = clarinet_decode_port(&src[i + 2], n - 2);
            if (errcode < 0)
                return errcode;

            const uint16_t port = (uint16_t)errcode;
            clarinet_addr addr = { 0 };
            errcode = clarinet_addr_ipv6_from_string(&addr, &src[1], i - 1);
            if (errcode == CLARINET_ENONE)
            {
                memset(dst, 0, sizeof(clarinet_endpoint));
                dst->addr = addr;
                dst->port = port;
            }

            return errcode;
        }
#endif
    }

    return CLARINET_EINVAL;
}

clarinet_addr
clarinet_make_ipv4(uint8_t a,
                   uint8_t b,
                   uint8_t c,
                   uint8_t d)
{
    clarinet_addr r = { 0 };
    r.family = CLARINET_AF_INET;
    r.as.ipv4.u.byte[0] = a;
    r.as.ipv4.u.byte[1] = b;
    r.as.ipv4.u.byte[2] = c;
    r.as.ipv4.u.byte[3] = d;
    return r;
}

clarinet_addr
clarinet_make_ipv6(uint32_t flow_info,
                   uint16_t a,
                   uint16_t b,
                   uint16_t c,
                   uint16_t d,
                   uint16_t e,
                   uint16_t f,
                   uint16_t g,
                   uint16_t h,
                   uint32_t scope_id)
{
    clarinet_addr r = { 0 };
    r.family = CLARINET_AF_INET6;
    r.as.ipv6.flowinfo = flow_info;
    r.as.ipv6.u.word[0] = htons(a);
    r.as.ipv6.u.word[1] = htons(b);
    r.as.ipv6.u.word[2] = htons(c);
    r.as.ipv6.u.word[3] = htons(d);
    r.as.ipv6.u.word[4] = htons(e);
    r.as.ipv6.u.word[5] = htons(f);
    r.as.ipv6.u.word[6] = htons(g);
    r.as.ipv6.u.word[7] = htons(h);
    r.as.ipv6.scope_id = scope_id;
    return r;
}

clarinet_endpoint
clarinet_make_endpoint(const clarinet_addr addr,
                       uint16_t port)
{
    clarinet_endpoint r = { addr, port };
    return r;
}

int
clarinet_endpoint_to_sockaddr(struct sockaddr_storage* restrict dst,
                              socklen_t* restrict dstlen,
                              const clarinet_endpoint* restrict src)
{
    if (src && dst)
    {
        if (clarinet_addr_is_ipv4(&src->addr))
        {
            struct sockaddr_in* addr = (struct sockaddr_in*)dst;
            memset(dst, 0, sizeof(struct sockaddr_in));
            if (dstlen)
                *dstlen = sizeof(struct sockaddr_in);
    #if HAVE_STRUCT_SOCKADDR_SA_LEN
            addr->sin_len = sizeof(struct sockaddr_in);
    #endif
            addr->sin_family = AF_INET;
            addr->sin_port = src->port;
            clarinet_addr_ipv4_to_inet(&addr->sin_addr, &src->addr.as.ipv4.u);

            return CLARINET_ENONE;
        }
#if CLARINET_ENABLE_IPV6 && HAVE_SOCKADDR_IN6_SIN6_ADDR
        else if (clarinet_addr_is_ipv6(&src->addr))
        {
            struct sockaddr_in6* addr = (struct sockaddr_in6*)dst;
            memset(dst, 0, sizeof(struct sockaddr_in6));
            if (dstlen)
                *dstlen = sizeof(struct sockaddr_in6);

    #if HAVE_STRUCT_SOCKADDR_SA_LEN
            addr->sin6_len = sizeof(struct sockaddr_in6);
    #endif
            addr->sin6_family = AF_INET6;
            addr->sin6_port = src->port;
            addr->sin6_flowinfo = src->addr.as.ipv6.flowinfo;
            clarinet_addr_ipv6_to_inet6(&addr->sin6_addr, &src->addr.as.ipv6.u);
    #if HAVE_SOCKADDR_IN6_SIN6_SCOPE_ID
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
                                const struct sockaddr_storage* restrict src)
{
    if (src && dst)
    {
        if (src->ss_family == AF_INET)
        {
            struct sockaddr_in* addr = (struct sockaddr_in*)src;
            memset(dst, 0, sizeof(clarinet_endpoint));

            dst->addr.family = CLARINET_AF_INET;
            clarinet_addr_ipv4_from_inet(&dst->addr.as.ipv4.u, &addr->sin_addr);
            dst->port = addr->sin_port;

            return CLARINET_ENONE;
        }
#if CLARINET_ENABLE_IPV6 && HAVE_SOCKADDR_IN6_SIN6_ADDR
        else if (src->ss_family == AF_INET6)
        {
            struct sockaddr_in6* addr = (struct sockaddr_in6*)src;
            memset(dst, 0, sizeof(clarinet_endpoint));

            dst->addr.family = CLARINET_AF_INET6;
            dst->addr.as.ipv6.flowinfo = addr->sin6_flowinfo;
            clarinet_addr_ipv6_from_inet6(&dst->addr.as.ipv6.u, &addr->sin6_addr);
    #if HAVE_SOCKADDR_IN6_SIN6_SCOPE_ID
            dst->addr.as.ipv6.scope_id = addr->sin6_scope_id;
    #endif
            dst->port = addr->sin6_port;

            return CLARINET_ENONE;
        }
#endif
    }

    return CLARINET_EINVAL;
}
