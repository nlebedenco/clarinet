#pragma once
#ifndef CLARINET_H
#define CLARINET_H

#include <stdint.h>
#include <stdbool.h>

#if defined(__cplusplus)
extern "C" {
#endif

/***********************************************************************************************************************
 * Utility macros
 **********************************************************************************************************************/

/** Stringification. */
#define CLARINET_STR(s)     #s
#define CLARINET_XSTR(s)    CLARINET_STR(s)

/** Check whether this is GCC major.minor or a later release. */
#if !defined(__GNUC__)
  /* Not GCC and not "just like GCC" */
  #define CLARINET_IS_AT_LEAST_GNUC_VERSION(major, minor) 0
#else
  /* GCC or "just like GCC" */
  #define CLARINET_IS_AT_LEAST_GNUC_VERSION(major, minor) \
	(__GNUC__ > (major) || \
	 (__GNUC__ == (major) && __GNUC_MINOR__ >= (minor)))
#endif

/** Check whether this is Clang major.minor or a later release. */
#if !defined(__clang__)
  /* Not Clang */
  #define CLARINET_IS_AT_LEAST_CLANG_VERSION(major, minor) 0
#else
  /* Clang */
  #define CLARINET_IS_AT_LEAST_CLANG_VERSION(major, minor) \
	(__clang_major__ > (major) || \
	 (__clang_major__ == (major) && __clang_minor__ >= (minor)))
#endif


/***********************************************************************************************************************
 * API symbols
 *
 * CLARINET_EXPORT is defined by the build system when the target is a shared library. In this case we can arrange to 
 * export only the necessary symbols by defining CLARINET_API accordingly. 
 * 
 * On Windows it is advantageous for headers accompanying DLLs to declare exported symbols with 
 * __declspec(dllimport) because the compiler can alledgedly produce more efficient code if the attribute is present.
 * See https://docs.microsoft.com/en-us/cpp/build/importing-into-an-application-using-declspec-dllimport?view=msvc-160
 * 
 * The common case is for the header to accompany a shared library so the user must define CLARINET_STATIC if using 
 * static linkage. This is more convenient than having to define a CLARINET_DLL because CMake can automatically export 
 * compiler definitions required by dependencies and then users don't even have to be aware that such a define is 
 * needed.
 **********************************************************************************************************************/
 #if defined(CLARINET_STATIC)
    #define CLARINET_VISIBLITY
#else 
    #if defined(_WIN32)
        #if defined(CLARINET_EXPORT)
            #define CLARINET_VISIBLITY     __declspec(dllexport)
        #else
            #define CLARINET_VISIBLITY     __declspec(dllimport)
        #endif
    #else /* UN*X */
        #if defined(CLARINET_EXPORT)
            #if CLARINET_IS_AT_LEAST_GNUC_VERSION(3,4)
                /* GCC 3.4 and later (or some compiler asserting compatibility with
                 * GCC 3.4 and later) so we have __attribute__((visibility()). */
                #define CLARINET_VISIBLITY    __attribute__((visibility("default")))
            #else 
                #define CLARINET_VISIBLITY
            #endif
        #else 
            #define CLARINET_VISIBLITY
        #endif
    #endif
#endif

#define CLARINET_API CLARINET_VISIBLITY extern

#if defined(__cplusplus)
    #if defined(_WIN32) && (_MSC_VER >= 1900) /* MSVC on VS2015+ */
        #define CLARINET_RESTRICT __restrict
    #elseif defined(__GNUC__) /* GCC */
        #define CLARINET_RESTRICT __restrict__
    #elseif defined(__clang__) /* CLANG */
        #define CLARINET_RESTRICT __restrict
    #else
        #define CLARINET_RESTRICT 
    #endif
#else 
    /* This should be a c99 compiler with proper support to the restrict keyword */
    #define CLARINET_RESTRICT restrict
#endif


/***********************************************************************************************************************
 * Common
 *
 * In POSIX, names ending with _t are reserved. Since we're targeting at least one POSIX system (i.e. Linux) typenames 
 * defined here MUST NOT end with _t.
 **********************************************************************************************************************/

enum clarinet_errcode
{
    CLARINET_ENONE = 0,                     /* Success */ 
    CLARINET_EFAIL = -1,                    /* Operation failed (unspecified error) */
    CLARINET_ESHUTDOWN = -2,                /* Not initialized */
    CLARINET_EALREADY = -3,                 /* Operation already in progress */
    CLARINET_EINVAL = -4,                   /* Invalid Argument */
    CLARINET_ENOTSUP = -5,                  /* Operation is not supported */
    CLARINET_EPERM = -6,                    /* Operation is not permitted */
    CLARINET_EPROTO = -7,                   /* Protocol error */
    CLARINET_EPROTONOSUPPORT = -8,          /* Protocol not supported */
    CLARINET_ETIME = -9,                    /* Timeout */
    CLARINET_EOVERFLOW = -10,               /* Buffer overflow */
};

typedef enum clarinet_errcode clarinet_errcode;

enum clarinet_proto
{
    CLARINET_PROTO_NONE = 0,                /* None */
    CLARINET_PROTO_UDP = (1 << 1),          /* User Datagram Protocol (RFC768) */
    CLARINET_PROTO_DTLC = (1 << 2),         /* Datagram Transport Layer Connectivity (Custom) */
    CLARINET_PROTO_DTLS = (1 << 3),         /* Datagram Transport Layer Security (RFC6347) */
    CLARINET_PROTO_UDTP = (1 << 4),         /* User Datagram Transmission Protocol (Custom) */
    CLARINET_PROTO_UDTPS = (1 << 5),        /* User Datagram Transmission Protocol Secure (Custom) */
    CLARINET_PROTO_ENET = (1 << 6),         /* ENet (http://enet.bespin.org/index.html) */
    CLARINET_PROTO_ENETS = (1 << 7),        /* ENet Secure (Custom) */
    CLARINET_PROTO_TCP = (1 << 8),          /* Transmission Control Protocol (RFC793) */
    CLARINET_PROTO_TLS = (1 << 9),          /* Transport Layer Security (RFC8446) */
};

typedef enum clarinet_proto clarinet_proto;

enum clarinet_feature
{
    CLARINET_FEATURE_NONE = 0,
    CLARINET_FEATURE_DEBUG = (1 << 0),
    CLARINET_FEATURE_PROFILE = (1 << 1),
    CLARINET_FEATURE_LOG = (1 << 2),
    CLARINET_FEATURE_IPV6 = (1 << 3),
    CLARINET_FEATURE_IPV6DUAL = (1 << 4),
};

typedef enum clarinet_feature clarinet_feature;

CLARINET_API uint32_t    clarinet_get_semver(void);
CLARINET_API const char* clarinet_get_version(void);
CLARINET_API const char* clarinet_get_name(void);
CLARINET_API const char* clarinet_get_description(void);
CLARINET_API uint32_t    clarinet_get_protocols(void);
CLARINET_API uint32_t    clarinet_get_features(void);


/***********************************************************************************************************************
 * IPv4/IPv6 support
 *
 * Network layer address definitions and transport endpoint definition common to UDP and TCP. 
 * 
 * ISO C++ does not support anonymous structs but supports anonymous unions. Unfortunately, C99 does not support 
 * anonymous bitfields so we still have to rely on named members for padding. We define our own structures for inet and 
 * inet6 addresses to mantain the public API as system agnostic as possible and avoid creating a dependency on 
 * non-standard headers for sockaddr, sockaddr_in and sockaddr_in6.
 **********************************************************************************************************************/

struct clarinet_ipv4_addr
{
    uint32_t padding[4]; /* padding to overlay on IPv4-mapped IPv6 addresses */
    union 
    {
        uint8_t  byte[4];
        uint16_t word[2];
        uint32_t dword;
    };
};

typedef struct clarinet_ipv4_addr clarinet_ipv4_addr; 

struct clarinet_ipv6_addr 
{
    uint32_t flowinfo; 
    union 
    {
        uint8_t  byte[16];
        uint16_t word[8];
    };
    uint32_t scope_id;
};

typedef struct clarinet_ipv6_addr clarinet_ipv6_addr; 

#define CLARINET_ADDR_IS_IPV4(addr)                 TODO
#define CLARINET_ADDR_IS_IPV6(addr)                 TODO
#define CLARINET_ADDR_IS_IPV4_MAPPED_TO_IPV6(addr)  TODO

#define CLARINET_ADDR_MAP_TO_IPV4(addr)             TODO
#define CLARINET_ADDR_MAP_TO_IPV6(addr)             TODO
        
#define CLARINET_ADD_IS_LOOPBACK(addr)              TODO
        
#define CLARINET_ADDR_IPV6_LOOPBACK                 TODO
#define CLARINET_ADDR_IPV4_LOOPBACK                 TODO
        
#define CLARINET_ADDR_IPV6_ANY                      TODO
#define CLARINET_ADDR_IPV4_ANY                      TODO

struct clarinet_addr
{
    uint16_t family;
    union
    {
        clarinet_ipv4_addr ipv4;
        clarinet_ipv6_addr ipv6;
    };   
};

typedef struct clarinet_addr clarinet_addr; 

struct clarinet_endpoint
{
    clarinet_addr addr;
    uint16_t port; 
}

typedef struct clarinet_endpoint clarinet_endpoint;


/***********************************************************************************************************************
 * UDP
 *
 * Creates a UDP socket that can be open and bound to a local endpoint in a single operation using clarinet_udp_open(2).
 * The 'settings' parameter can specify options that must be configured before the bind and so cannot be modified by 
 * calling clarinet_udp_setopt(5). Sockets are blocking and dual stack is disabled by default despite some operating
 * systems having a specific configuration for it (e.g. sysctl:/proc/sys/net/ipv6/bindv6only on Linux). This is
 * to ensure portability is consistent and predictable. See UDP flags for more information. Timeout settings are ignored
 * if the socket is non-blocking. Binding is exact and exclusive so reuse-address follows the same semantics as in Linux 
 * with [SO_REUSEADDR|SO_REUSEPORT] or Windows with [SO_REUSEADDR|SO_EXCLUSIVEADDR]. There is no UDP "connect" 
 * functionality because dgram packet delivery rules may be substantially different between platforms. On BSD/Linux 
 * (including macOS) when a UDP socket is bound to a foreign address by connect(2) it effectively assumes a 5-tuple 
 * identity so when a dgram arrives the system first selects all sockets associated with the src address of the 
 * packet and then picks the socket with the most specific local address matching the destination address of the 
 * packet. On windows however, UDP associations established with connect(2) do not affect routing. They only serve as 
 * defaults for send(2) and recv(2) so on Windows all UDP sockets have a foreign address *:* and the first entry 
 * on the routing table with a local address that matches the destination address of the arriving packet is picked. This
 * basically prevents UDP servers from ever using connect(2) and operate with multiple sockets as with TCP. Besides 
 * "connected" UDP sockets by definition prevent upper layers from implementing any support to IP mobility.
 *
 * Note that besides platform support, dual-stack also requires a local IPv6 address (either an explicit one or the 
 * wildcard [::] which is equivalent to CLARINET_IPV6_ANY). The ability to interact with IPv4 addresses requires the use 
 * of the IPv4-mapped IPv6 address format. Any IPv4 addresses must be represented in the IPv4-mapped IPv6 address format 
 * which enables an IPv6-only application to communicate with an IPv4 node. The IPv4-mapped IPv6 address format allows 
 * the IPv4 address of an IPv4 node to be represented as an IPv6 address. The IPv4 address is encoded into the low-order
 * 32 bits of the IPv6 address, and the high-order 96 bits hold the fixed prefix 0:0:0:0:0:FFFF. The IPv4-mapped IPv6 
 * address format is specified in RFC4291. Applications must take care to handle these IPv4-mapped IPv6 addresses 
 * appropriately and only use them with dual stack sockets. If an IP address is to be passed to a regular IPv4 socket, 
 * the address must be a regular IPv4 address not a IPv4-mapped IPv6 address. 
 *
 * An application with a UDP socket bound to [::] (IPv6) and dual-stack enabled occupies the port on both IPv6 and IPv4. 
 * Therefore a second UDP socket cannot be bound to 0.0.0.0 (IPv4 only) on the same port unless 
 * CLARINET_UDP_FLAG_REUSEADDR is used. Note however that in this particular case it is impossible to determine which 
 * socket will handle incoming IPv4 packets and behaviour will depend on the operating system.
 **********************************************************************************************************************/

enum clarinet_udp_flag
{
    CLARINET_UDP_FLAG_NONE = 0,
    CLARINET_UDP_FLAG_NONBLOCK = (1 << 0),
    CLARINET_UDP_FLAG_REUSEADDR = (1 << 1),
    CLARINET_UDP_FLAG_IPV6DUAL = (1 << 2)
}

typedef enum clarinet_udp_flag clarinet_udp_flag;

/** 
 * Options corresponding to flags are defined in the range [1, 32] for convenience so we can transform option to flag 
 * easily with CLARINET_OPTION_TO_FLAG(opt). All other options are associated with non-boolean values 
 * and thus are defined in the range [33, 65535].
 */
enum clarinet_udp_option
{
    CLARINET_UDP_OPTION_NONE = 0,
    
    CLARINET_UDP_OPTION_NONBLOCK = 1,
    CLARINET_UDP_OPTION_REUSEADDR = 2,
    CLARINET_UDP_OPTION_IPV6DUAL = 3,
    
    CLARINET_UDP_OPTION_ERROR = 33
    CLARINET_UDP_OPTION_SNDBUF = 34,
    CLARINET_UDP_OPTION_RCVBUF = 35,
    CLARINET_UDP_OPTION_SNDTIMEO = 36,
    CLARINET_UDP_OPTION_RCVTIMEO = 37,
    CLARINET_UDP_OPTION_TTL = 38   
};

typedef enum clarinet_udp_option clarinet_udp_option;

/** 
 * Converts a clarinet_udp_option 'opt' in the range [1, 32] to a bitmask = 2^(opt-1) without branching. The macro 
 * produces 0 if opt <= 0 and 2^(opt-1 mod 32) if opt >= 33. 
 *
 * The number of bits in an enum clarinet_udp_option type is determined at compile time by 
 * (sizeof(clarinet_udp_option) << 3) then (opt-1) is negated and the most significant bit is shifted n-1 bits to yield 
 * either 0 or 1 and form the first operand of the actual bitmask shift. Finally ((opt-1) & 0x1F) is used as the second 
 * operand to shift left no more than 31 bits. Note that if 'opt' is a compile time constant then all this is calculated 
 * at compile time so no runtime penalty.
 */
#define CLARINET_OPTION_TO_FLAG(opt) (((~((clarinet_udp_option)(opt) -1)) >> ((sizeof(clarinet_udp_option) << 3)-1)) << (((clarinet_udp_option)(opt) -1) & 0x1F))

struct clarinet_udp_settings
{
    uint32_t send_buffer_size; 
    uint32_t recv_buffer_size;
    uint32_t send_timeout; 
    uint32_t recv_timeout;
    uint8_t ttl;
};

typedef struct clarinet_udp_settings clarinet_udp_settings;

CLARINET_API int clarinet_udp_open(const clarinet_endpoint* CLARINET_RESTRICT endpoint, 
                                   const clarinet_udp_settings* CLARINET_RESTRICT settings,
                                   uint32_t flags;

CLARINET_API int clarinet_udp_close(int sockfd);

CLARINET_API int clarinet_udp_send(int sockfd, 
                                   const void* CLARINET_RESTRICT buf,
                                   size_t len, 
                                   const clarinet_endpoint* CLARINET_RESTRICT dst);

CLARINET_API int clarinet_udp_recv(int sockfd, 
                                   void* CLARINET_RESTRICT buf, 
                                   size_t len, 
                                   clarinet_endpoint* CLARINET_RESTRICT src);

CLARINET_API int clarinet_udp_setopt(int sockfd, 
                                     int proto, 
                                     int optname,
                                     const void CLARINET_RESTRICT *optval, 
                                     size_t optlen);

CLARINET_API int clarinet_udp_getopt(int sockfd, 
                                     int proto, 
                                     int optname, 
                                     void* CLARINET_RESTRICT optval, 
                                     size_t* CLARINET_RESTRICT optlen);

/***********************************************************************************************************************
 * DTLC
 *
 * Creates a DTLC socket. DTLC (Datagram Transport Layer Connectivity) is a custom lightweight alternative to DTLS 
 * that offers end-to-end connectivity, integrity check, optional IP mobility and optional encryption but does not
 * support either server or client authentication.
 * 
 * The underlying UDP socket must be non-blocking so the flag CLARINET_UDP_FLAG_NONBLOCK is automatically added if not 
 * provided by the user in clarinet_enet_open(3).
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * DTLS
 *
 * Creates a DTLS 1.2 socket as specified by RFC6347. Optional IP mobility is implemented with the connection id 
 * extension proposed by Rescorla et al. in "Connection Identifiers for DTLS 1.2 (draft-ietf-tls-dtls-connection-id-13)"
 * <https://datatracker.ietf.org/doc/draft-ietf-tls-dtls-connection-id/>
 *
 * The underlying UDP socket must be non-blocking so the flag CLARINET_UDP_FLAG_NONBLOCK is automatically added if not 
 * provided by the user in clarinet_enet_open(3).
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * UDTP
 *
 * Creates a UDTP socket based on a non-blocking DTLC socket for the transport layer connection. UDTP (User Datagram 
 * Transmission Protocol) is a connection-oriented transport protocol with delay-based congestion control, reliable and 
 * unreliable sequenced delivery, messaage batching and up to 16 independent transmission channels.
 *
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * UDTPS
 *
 * Creates a UDTP socket based on a non-blocking DTLS socket instead of DTLC.
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * ENET
 *
 * Creates an ENet socket following the same protocol proposed and implemented by Lee Salzman at
 * <http://enet.bespin.org/index.html>. This implementation differs slightly from the original in terms of control but 
 * the protocol should be 100% compatible. This was required because the original ENet implementation was IPv4 only and 
 * employed a single update function (enet_host_service(3)) that also serves to poll for network events whereas all 
 * protocol interfaces implemeted by Clarinet communicate network events when a recv is performed and rely on two 
 * separate update chains - one for receiving data up the stack (update) and another for sending data down the stack 
 * (flush).
 *
 * The underlying UDP socket must be non-blocking so the flag CLARINET_UDP_FLAG_NONBLOCK is automatically added if not 
 * provided by the user in clarinet_enet_open(3).
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * ENETS
 *
 * Creates an ENet socket based on a non-blocking DTLS socket instead of UDP.
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * TCP
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * TLS
 **********************************************************************************************************************/


#ifdef __cplusplus
}
#endif

#endif /* CLARINET_H */