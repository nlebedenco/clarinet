#include "compat/compat.h"
#include "clarinet/clarinet.h"

#include <string.h>

/* region Error Codes */

#define CLARINET_DECLARE_ENUM_NAME(e, v, s) case (e): return #e;
#define CLARINET_DECLARE_ENUM_DESC(e, v, s) case (e): return (s);

const char*
clarinet_error_name(int errcode)
{
    if (errcode > 0)
        return CLARINET_ERROR_NAME_INVALID;

    switch (errcode)
    {
        CLARINET_ERRORS(CLARINET_DECLARE_ENUM_NAME)
        default:
            return CLARINET_ERROR_NAME_UNDEFINED;
    }
}

const char*
clarinet_error_description(int err)
{
    if (err > 0)
        return CLARINET_ERROR_DESC_INVALID;

    switch (err)
    {
        CLARINET_ERRORS(CLARINET_DECLARE_ENUM_DESC)
        default:
            return CLARINET_ERROR_DESC_UNDEFINED;
    }
}

/* endregion */

/* region Family Codes */

const char*
clarinet_family_name(int family)
{
    switch (family)
    {
        CLARINET_FAMILIES(CLARINET_DECLARE_ENUM_NAME)
        default:
            return "(invalid)";
    }
}

const char*
clarinet_family_description(int family)
{
    switch (family)
    {
        CLARINET_FAMILIES(CLARINET_DECLARE_ENUM_DESC)
        default:
            return "Invalid family code";
    }
}

/* endregion */

/* region Protocol Codes */

const char*
clarinet_proto_name(int proto)
{
    switch (proto)
    {
        CLARINET_PROTOS(CLARINET_DECLARE_ENUM_NAME)
        default:
            return "(invalid)";
    }
}

const char*
clarinet_proto_description(int proto)
{
    switch (proto)
    {
        CLARINET_PROTOS(CLARINET_DECLARE_ENUM_DESC)
        default:
            return "Invalid protocol code";
    }
}

/* endregion */

/* region Library Info */

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
    return CLARINET_XSTR(CONFIG_VERSION_MAJOR)
           "." CLARINET_XSTR(CONFIG_VERSION_MINOR)
           "." CLARINET_XSTR(CONFIG_VERSION_PATCH);
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

int
clarinet_get_features(void)
{
    int features = CLARINET_FEATURE_NONE;

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

/* endregion */

/* region Address */

/* @formatter:off */

const clarinet_addr clarinet_addr_none                      = { 0 };
const clarinet_addr clarinet_addr_any_ipv4                  = { CLARINET_AF_INET,  0, { { 0, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   0,   0,   0,   0,   0,   0 } }, 0 } } };
const clarinet_addr clarinet_addr_any_ipv6                  = { CLARINET_AF_INET6, 0, { { 0, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   0,   0,   0,   0,   0,   0 } }, 0 } } };
const clarinet_addr clarinet_addr_loopback_ipv4             = { CLARINET_AF_INET,  0, { { 0, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   0,   0, 127,   0,   0,   1 } }, 0 } } };
const clarinet_addr clarinet_addr_loopback_ipv6             = { CLARINET_AF_INET6, 0, { { 0, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   0,   0,   0,   0,   0,   1 } }, 0 } } };
const clarinet_addr clarinet_addr_loopback_ipv4mapped       = { CLARINET_AF_INET6, 0, { { 0, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 127,   0,   0,   1 } }, 0 } } };
const clarinet_addr clarinet_addr_broadcast_ipv4            = { CLARINET_AF_INET,  0, { { 0, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   0,   0, 255, 255, 255, 255 } }, 0 } } };

/* @formatter:on */

int
clarinet_addr_convert_to_ipv4(clarinet_addr* restrict dst,
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
        if (!clarinet_addr_is_ipv6(src))
            return CLARINET_EAFNOSUPPORT;

        if (clarinet_addr_is_ipv4mapped(src))
        {
            memset(dst, 0, sizeof(clarinet_addr));
            dst->family = CLARINET_AF_INET;
            dst->as.ipv4.u.dword[0] = src->as.ipv4.u.dword[0];
            return CLARINET_ENONE;
        }
        #else
        return CLARINET_EAFNOSUPPORT;
        #endif
    }

    return CLARINET_EINVAL;
}

int
clarinet_addr_convert_to_ipv6(clarinet_addr* restrict dst,
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
        #else
        return CLARINET_EAFNOSUPPORT;
        #endif
    }

    return CLARINET_EINVAL;
}

/* endregion */
