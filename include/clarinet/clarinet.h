#pragma once
#ifndef CLARINET_H
#define CLARINET_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>

#if defined(__cplusplus)
extern "C" {
#endif

/***********************************************************************************************************************
 * Utility macros
 **********************************************************************************************************************/

/* Stringification. */
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

/** 
 * Macro to explicitly declare unused parameters inside functions.
 * Unfortunately, MSVC does not appear to have an equivalent to GCC's "__attribute__((unused))" to mark a particular
 * function parameter as being known to be unused, so that the compiler won't warn about it (for example, the
 * function might have that parameter because a pointer to it is being used, and the signature of that function
 * includes that parameter). C++ lets you give a parameter a type but no name, but C doesn't have that.
 */
#define CLARINET_IGNORE(x) (void)(x)

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
 * The common case is for the header to accompany a shared library so the user must define CLARINET_STATIC_LIB if using 
 * static linkage. This is more convenient than having to define a CLARINET_DLL because CMake can automatically export 
 * compiler definitions required by dependencies and then users don't even have to be aware that such a define is 
 * needed.
 **********************************************************************************************************************/
 #if defined(CLARINET_STATIC_LIB)
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

#if defined(_WIN32)
    #define CLARINET_CALLBACK __cdecl
#else
    #define CLARINET_CALLBACK
#endif    

/* 
 * Replace 'restrict' in C++ with something supported by the compiler.
 * MSVC Intellisense doesn't like the restrict keyword either. 
 */
#ifdef __INTELLISENSE__
    #define restrict 
#elif defined(__cplusplus)
    #if defined(restrict)    
        #define RESTRICT_PREDEFINED 1
    #else       
        #if defined(_WIN32) && (_MSC_VER >= 1900) /* MSVC on VS2015+ */
            #define restrict __restrict
        #elif defined(__GNUC__) /* GCC */
            #define restrict __restrict__
        #elif defined(__clang__) /* CLANG */
            #define restrict __restrict
        #else
            #define restrict 
        #endif
    #endif
#endif


/***********************************************************************************************************************
 * Common
 *
 * In POSIX, names ending with _t are reserved. Since we're targeting at least one POSIX system (i.e. Linux) typenames 
 * defined here MUST NOT end with _t.
 *
 * Macros that receive arguments should normally be defined in lower case following the same name convention of 
 * functions so no builds will break if we eventually replace those macros with real functions.
 *
 * Unfortunately, struct and union initialization in C/C++ is a mess. C99 supports member designators but C++ does not. 
 * C99 supports compound literals but C++ does not save for certain compiler extensions. Currently GCC and clang are 
 * known to support compound literals in C++. MSVC supports it in C but not in C++. The solution for now is to export
 * global consts which can be used for initialization.
 **********************************************************************************************************************/

#define CLARINET_ERRORS(E) \
    E(CLARINET_EPERM,             "Operation is not permitted") \
    E(CLARINET_ENOSYS,            "Operation is not implemented") \
    E(CLARINET_EINTR,             "Operation interrupted") \
    E(CLARINET_EIO,               "I/O error") \
    E(CLARINET_ENOMEM,            "Not enough memory") \
    E(CLARINET_EACCES,            "Access denied") \
    E(CLARINET_EINVAL,            "Invalid Argument") \
    E(CLARINET_ENOTREADY,         "Underlying system or device not ready") \
    E(CLARINET_EALREADY,          "Operation is already in progress") \
    E(CLARINET_ENOTSOCK,          "Operation attempted with an invalid socket descriptor") \
    E(CLARINET_EMSGSIZE,          "Message too large") \
    E(CLARINET_ENOPROTOOPT,       "Protocol option not available") \
    E(CLARINET_EPROTONOSUPPORT,   "Protocol not supported") \
    E(CLARINET_ENOTSUPP,          "Operation is not supported") \
    E(CLARINET_ENOBUFS,           "Not enough buffer space or queue is full") \
    E(CLARINET_EADDRINUSE,        "Address already in use") \
    E(CLARINET_EADDRNOTAVAIL,     "Address is not available/cannot be assigned") \
    E(CLARINET_ENETDOWN,          "Network is down") \
    E(CLARINET_ENETUNREACH,       "Network is unreachable") \
    E(CLARINET_ENETRESET,         "Network reset possibly due to keepalive timeout") \
    E(CLARINET_ENOTCONN,          "Socket is not connected") \
    E(CLARINET_EISCONN,           "Socket is already connected") \
    E(CLARINET_ECONNABORTED,      "Connection aborted (closed locally)") \
    E(CLARINET_ECONNRESET,        "Connection reset by peer (closed remotely)") \
    E(CLARINET_ECONNSHUTDOWN,     "Connection is shutdown and cannot send") \
    E(CLARINET_ECONNTIMEOUT,      "Connection timeout") \
    E(CLARINET_ECONNREFUSED,      "Connection refused") \
    E(CLARINET_EHOSTDOWN,         "Host is down") \
    E(CLARINET_EHOSTUNREACH,      "No route to host.") \
    E(CLARINET_EPROCLIM,          "Too many processes or tasks") \
    E(CLARINET_EMFILE,            "Too many files") \
    E(CLARINET_ELIBACC,           "Cannot access a needed shared library") \
    E(CLARINET_ELIBBAD,           "Accessing a corrupted shared library") \

#define CLARINET_DECLARE_ENUM_ITEM(e, s) e,

enum clarinet_error
{
    CLARINET_ENONE = 0,                             /* Success */       
    CLARINET_EDEFAULT = INT_MIN,                    /* Operation failed (unspecified error) */

    CLARINET_ERRORS(CLARINET_DECLARE_ENUM_ITEM)     /* Specific error codes */
};

enum clarinet_proto
{
    CLARINET_PROTO_NONE  = 0,                   /* None */
    CLARINET_PROTO_SOCK  = (1 <<  1),           /* Network layer abstraction provided by the system (currently either inet or inet6). */
    CLARINET_PROTO_UDP   = (1 <<  2),           /* User Datagram Protocol (RFC768) */
    CLARINET_PROTO_TCP   = (1 <<  3),           /* Transmission Control Protocol (RFC793) */    
    CLARINET_PROTO_DTLC  = (1 <<  4),           /* Datagram Transport Layer Connectivity (Custom) */
    CLARINET_PROTO_DTLS  = (1 <<  5),           /* Datagram Transport Layer Security (RFC6347) */
    CLARINET_PROTO_UDTP  = (1 <<  6),           /* User Datagram Transmission Protocol (Custom) */
    CLARINET_PROTO_UDTPS = (1 <<  7),           /* User Datagram Transmission Protocol Secure (Custom) */
    CLARINET_PROTO_ENET  = (1 <<  8),           /* ENet (http://enet.bespin.org/index.html) */
    CLARINET_PROTO_ENETS = (1 <<  9),           /* ENet Secure (Custom) */    
    CLARINET_PROTO_TLS   = (1 << 10)            /* Transport Layer Security (RFC8446) */
};

enum clarinet_feature
{
    CLARINET_FEATURE_NONE     = 0,              /* None */
    CLARINET_FEATURE_DEBUG    = (1 << 0),       /* Debug information built-in */
    CLARINET_FEATURE_PROFILE  = (1 << 1),       /* Profiler instrumentation built-in */
    CLARINET_FEATURE_LOG      = (1 << 2),       /* Log built-in */
    CLARINET_FEATURE_IPV6     = (1 << 3),       /* Support for IPv6 */
    CLARINET_FEATURE_IPV6DUAL = (1 << 4)        /* Support for IPv6 in dual-stack mode */
};

CLARINET_API
uint32_t
clarinet_get_semver(void);

CLARINET_API
const char*
clarinet_get_version(void);

CLARINET_API 
const char*
clarinet_get_name(void);

CLARINET_API 
const char*
clarinet_get_description(void);

CLARINET_API 
uint32_t
clarinet_get_protocols(void);

CLARINET_API 
uint32_t
clarinet_get_features(void);

CLARINET_API
const char*
clarinet_error_name(int err);

CLARINET_API
const char*
clarinet_error_str(int err);


/***********************************************************************************************************************
 * Memory allocation
 **********************************************************************************************************************/

struct clarinet_allocator
{
    void* (CLARINET_CALLBACK * malloc)(size_t size);    /* (required) default is malloc() */
    void  (CLARINET_CALLBACK * free)(void * memory);    /* (required) default is free() */
    void  (CLARINET_CALLBACK * nomem)(void);            /* (optional) default is abort() */
};

typedef struct clarinet_allocator clarinet_allocator;

CLARINET_API
int
clarinet_set_allocator(const clarinet_allocator* allocator);

CLARINET_API
void *
clarinet_malloc(size_t size);

CLARINET_API
void
clarinet_free(void* ptr);


/***********************************************************************************************************************
 * IPv4/IPv6 support
 *
 * Network layer address definitions and transport endpoint definition common to UDP and TCP. 
 * 
 * We define our own structures for inet and inet6 addressing to mantain the public API as system agnostic as possible 
 * and avoid creating a dependency on non-standard headers for sockaddr, sockaddr_in and sockaddr_in6.
 *
 * Unfortunately, C99 does not support anonymous unions so we have to use an additional name for those. Anonymous 
 * bitfields are not supported either so we have to rely on named members for padding. 
 *
 * Structs/Unions declared just for structural purposes do not have typedefs because users should not normally have to 
 * deal with them and if they ever do it's best they have to be explicit about it.
 **********************************************************************************************************************/

/**
 * IPv4 address information (consider using clarinet_addr instead) 
 * 
 * Padding is used to align an IPv4 address struct with an IPv6 address struct so one can extract IPv4 information 
 * from IPv4MappedToIPv6 addresses without having to know the details about IPv4MappedToIPv6 format or produce a 
 * complete IPv4MappedToIPv6 address.
 */
union clarinet_addr_ipv4_octets
{
    uint8_t  byte[4];
    uint16_t word[2];
    uint32_t dword[1]; /* Hack: MSVC cannot get const qualifiers right if this is not an array. */
};

struct clarinet_addr_ipv4
{   
    uint32_t _padding[4];
    union clarinet_addr_ipv4_octets u;   
};

/**
 * IPv6 address information (consider using clarinet_addr instead) 
 * 
 * This is also used to store information of IPv4MappedToIPv6 addresses.
 */

union clarinet_addr_ipv6_octets
{
    uint8_t  byte[16];       
    uint16_t word[8];       
    uint32_t dword[4];
};

struct clarinet_addr_ipv6
{
    uint32_t flowinfo; 
    union clarinet_addr_ipv6_octets u;
    uint32_t scope_id;
};

enum clarinet_addr_family 
{
    CLARINET_AF_NONE = 0,
    CLARINET_AF_INET = 2,
    CLARINET_AF_INET6 = 10 
};

/**
 * IP address
 * 
 * Can represent both IPv4 and IPv6 addresses. The member 'family' indicates which IP version is represented according 
 * to the constants defined in enum clarinet_addr_family. Note that an IPv4MappedToIPv6 is an IPv6 address that follows 
 * a specific format specified in RFC4291. clarinet_addr_is_ipv4mapped(addr) can be used to check if an address 
 * is an IPv4MappedToIPv6 address.
 * 
 */
struct clarinet_addr
{
    uint16_t family;
    uint16_t _padding;
    union clarinet_addr_ip /* this name is just to satisfy some C++ compilers */
    {
        struct clarinet_addr_ipv6 ipv6;
        struct clarinet_addr_ipv4 ipv4;       
    } as;
};

typedef struct clarinet_addr clarinet_addr; 

struct clarinet_endpoint
{
    clarinet_addr addr;
    uint16_t port;
    uint16_t _padding;
};

typedef struct clarinet_endpoint clarinet_endpoint;

/** 
 * Global constants that can be used for initialization in C++ 
 * (instead of compound literals which some compilers don't support e.g. MSVC) 
 */
CLARINET_API 
const clarinet_addr 
clarinet_addr_none;

CLARINET_API 
const clarinet_addr 
clarinet_addr_ipv4_any;

CLARINET_API 
const clarinet_addr 
clarinet_addr_ipv4_loopback;

CLARINET_API 
const clarinet_addr 
clarinet_addr_ipv4_broadcast;

CLARINET_API 
const clarinet_addr 
clarinet_addr_ipv6_any;

CLARINET_API 
const clarinet_addr 
clarinet_addr_ipv6_loopback;

CLARINET_API 
const clarinet_addr 
clarinet_addr_ipv4mapped_loopback;

/** 
 * Maximum string length required to format an address. The longest possible string representation is that of an 
 * IPv4MappedToIPv6 address with the largest scope id that can be supported (56+1 for the nul-termination). 
 * e.g: 0000:0000:0000:0000:0000:ffff:255.255.255.255%4294967295 
 */
#define CLARINET_ADDR_STRLEN                            (56+1)

/** 
 * Maximum string length required to format an endpoint. The longest possible string representation is that of an 
 * IPv4MappedToIPv6 address with the largest scope id that can be supported and the largest port (56+8+1 for the 
 * nul-termination). Note that square brackets are used to enclose IPv6 addresss and prevent ambiguity of the ':' sign.
 * e.g: [0000:0000:0000:0000:0000:ffff:255.255.255.255%4294967295]:65535 
 */
#define CLARINET_ENDPOINT_STRLEN                        (CLARINET_ADDR_STRLEN + 8) 

#define clarinet_addr_is_ipv4(addr)                     ((addr)->family == CLARINET_AF_INET)
#define clarinet_addr_is_ipv6(addr)                     ((addr)->family == CLARINET_AF_INET6)

#define clarinet_addr_is_ipv4mapped(addr)               \
    (((addr)->family == CLARINET_AF_INET6)              \
  && (((addr)->as.ipv6.u.word[0]                        \
     | (addr)->as.ipv6.u.word[1]                        \
     | (addr)->as.ipv6.u.word[2]                        \
     | (addr)->as.ipv6.u.word[3]                        \
     | (addr)->as.ipv6.u.word[4]) == 0)                 \
  && ((addr)->as.ipv6.u.word[5] == 0xFFFF))

/* 
 * Note that structs are compared element by element. It's not safe to compare structs using memcmp() because content of 
 * padding spaces is undefined and memcmp() will blindly compare every byte of allocated memory. 
 */ 

#define clarinet_addr_is_ipv4_any(addr)                                     \
   (((addr)->family == CLARINET_AF_INET)                                    \
 && ((addr)->as.ipv4.u.dword[0] == 0))

#define clarinet_addr_is_ipv6_any(addr)                                     \
   (((addr)->family == CLARINET_AF_INET6)                                   \
 && (((addr)->as.ipv6.u.dword[0]                                            \
    | (addr)->as.ipv6.u.dword[1]                                            \
    | (addr)->as.ipv6.u.dword[2]                                            \
    | (addr)->as.ipv6.u.dword[3]) == 0)                                     \
 && ((addr)->as.ipv6.scope_id == 0))

#define clarinet_addr_is_ipv4_loopback(addr)                                \
    (((addr)->family == CLARINET_AF_INET)                                   \
  && ((addr)->as.ipv6.u.byte[12] == 127)                                    \
  && ((addr)->as.ipv6.u.byte[15] > 0 && (addr)->as.ipv6.u.byte[15] < 255)   \
  && ((addr)->as.ipv6.scope_id == 0))

#define clarinet_addr_is_ipv6_loopback(addr)                                \
    (((addr)->family == CLARINET_AF_INET6)                                  \
  && (((addr)->as.ipv6.u.dword[0]                                           \
      | (addr)->as.ipv6.u.dword[1]                                          \
      | (addr)->as.ipv6.u.dword[2]) == 0)                                   \
  && ((addr)->as.ipv6.u.byte[14] == 0)                                      \
  && ((addr)->as.ipv6.u.byte[15] == 1)                                      \
  && ((addr)->as.ipv6.scope_id == 0))

#define clarinet_addr_is_ipv4mapped_loopback(addr)                          \
    (((addr)->family == CLARINET_AF_INET6)                                  \
  && (((addr)->as.ipv6.u.word[0]                                            \
     | (addr)->as.ipv6.u.word[1]                                            \
     | (addr)->as.ipv6.u.word[2]                                            \
     | (addr)->as.ipv6.u.word[3]                                            \
     | (addr)->as.ipv6.u.word[4]) == 0)                                     \
  && ((addr)->as.ipv6.u.word[5] == 0xFFFF)                                  \
  && ((addr)->as.ipv6.u.byte[12] == 127)                                    \
  && ((addr)->as.ipv6.u.byte[15] > 0 && (addr)->as.ipv6.u.byte[15] < 255)   \
  && ((addr)->as.ipv6.scope_id == 0))

/** Returns true if the address is an IPv4 broadcast address. */
#define clarinet_addr_is_ipv4_broadcast(addr)           \
    (((addr)->family == CLARINET_AF_INET)               \
  && ((addr)->as.ipv4.u.dword[0] == 0xFFFFFFFF))
  
#define clarinet_addr_is_ipv6_multicast(addr)           \
    (((addr)->family == CLARINET_AF_INET6)              \
  && ((addr)->as.ipv6.u.byte[0] == 0xFF)                \
  && ((addr)->as.ipv6.u.byte[1] == 0x00))
                                                      
#define clarinet_addr_is_ipv6_linklocal(addr)           \
    (((addr)->family == CLARINET_AF_INET6)              \
  && ((addr)->as.ipv6.u.byte[0] == 0xFE)                \
  && (((addr)->as.ipv6.u.byte[1] & 0xC0) == 0x80))
                                                      
#define clarinet_addr_is_ipv6_sitelocal(addr)           \
    (((addr)->family == CLARINET_AF_INET6)              \
  && ((addr)->as.ipv6.u.byte[0] == 0xFE)                \
  && (((addr)->as.ipv6.u.byte[1] & 0xC0) == 0xC0))

#define clarinet_addr_is_ipv6_teredo(addr)              \
    (((addr)->family == CLARINET_AF_INET6)              \
  && ((addr)->as.ipv6.u.byte[0] == 0x20)                \
  && ((addr)->as.ipv6.u.byte[1] == 0x01)                \
  && ((addr)->as.ipv6.u.byte[2] == 0x00)                \
  && ((addr)->as.ipv6.u.byte[3] == 0x00))


/** 
 * Returns true if the address pointed by addr represents the wildcard address in either IPv4 or IPv6. Note that there 
 * is no such thing as a wildcard address in IPv4MappedToIPv6 format because by definition the wildcard address is the 
 * zero address.
 */
#define clarinet_addr_is_any(addr)                                                  \
   ((((addr)->family == CLARINET_AF_INET) || ((addr)->family == CLARINET_AF_INET6)) \
  && (((addr)->as.ipv6.u.dword[0]                                                   \
     | (addr)->as.ipv6.u.dword[1]                                                   \
     | (addr)->as.ipv6.u.dword[2]                                                   \
     | (addr)->as.ipv6.u.dword[3]) == 0)                                            \
  && ((addr)->as.ipv6.scope_id == 0))

/** 
 * Returns true if the address pointed by addr represents a loopback address. It can be either an IPv4, IPv6 or an 
 * IPv4MappedToIPv6 address. RFC122 reserves the entire 127.0.0.0/8 address block for loopback purposes so anything 
 * from 127.0.0.1 to 127.255.255.254 is looped back. RFC4291 just reserves a single IPv6 address, ::1.
 */
#define clarinet_addr_is_loopback(addr)                     \
    (clarinet_addr_is_ipv4_loopback(addr)                   \
  || clarinet_addr_is_ipv6_loopback(addr)                   \
  || clarinet_addr_is_ipv4mapped_loopback(addr))
                                                      

/** 
 * Returns true if the address pointed by addr represents a broadcast address. Always false for IPv6 addresses since 
 * broadcasting is not supportted in IPv6 even if using an IPv4MappedToIPv6 address. 
 */
#define clarinet_addr_is_broadcast(addr)                clarinet_addr_is_ipv4_broadcast(addr)

/** 
 * Returns true if addresses pointed by a and b are equal. 
 * If famlily is CLARINET_AF_INET only the last dword is required to be equal.
 * Otherwise for both CLARINET_AF_INET6 and CLARINET_AF_NONE all ipv6 fields must be equal. 
 * Note that flowinfo is not considered an identifying part of an IPv6 address so two addresses A and B that conly 
 * differ by flowinfo are considered equal.
 */
#define clarinet_addr_is_equal(a, b)                            \
    ((a)->family == (b)->family                                 \
  && (a)->as.ipv6.u.dword[3] == (b)->as.ipv6.u.dword[3]         \
  && ((a)->family == CLARINET_AF_INET                           \
   || ((a)->as.ipv6.u.dword[0] == (b)->as.ipv6.u.dword[0]       \
    && (a)->as.ipv6.u.dword[1] == (b)->as.ipv6.u.dword[1]       \
    && (a)->as.ipv6.u.dword[2] == (b)->as.ipv6.u.dword[2]       \
    && (a)->as.ipv6.scope_id == (b)->as.ipv6.scope_id)))

/** 
 * Returns true if addresses pointed by a and b are equivalent but not necessarily equal. This could be the case when 
 * comparing an IPv4 address with an IPv4mappedToIPv6 address. They are never equal because the families involved are 
 * different (one is INET the other INET6) but could be equivalent if both represent the same (ipv4) network address.
 * Note that flowinfo is not considered an identifying part of an IPv6 address so two addresses A and B that conly 
 * differ by flowinfo are considered equal.
 */
#define clarinet_addr_is_equivalent(a, b)                                       \
    (clarinet_addr_is_equal(a, b)                                               \
  || (((a)->as.ipv6.u.dword[3] == (b)->as.ipv6.u.dword[3])                      \
   && ((clarinet_addr_is_ipv4(a) && clarinet_addr_is_ipv4mapped(b))             \
    || (clarinet_addr_is_ipv4(b) && clarinet_addr_is_ipv4mapped(a)))))

#define clarinet_endpoint_is_equal(a, b)                (((a)->port == (b)->port) && clarinet_addr_is_equal(&(a)->addr, &(b)->addr))

#define clarinet_endpoint_is_equivalent(a, b)           (((a)->port == (b)->port) && clarinet_addr_is_equivalent(&(a)->addr, &(b)->addr))

/** 
 * Converts the IPv4MappedToIPv6 address pointed by src into an IPv4 address and copies it into the memory pointed by 
 * dst. If src points to an IPv4 address then a simple copy is performed. On success returns CLARINET_ENONE. If either 
 * dst or src are NULL or the address pointed by src is neither an IPv4MappedToIPv6 nor an IPv4 address then 
 * CLARINET_EINVAL is returned instead.
 */
CLARINET_API
int
clarinet_addr_map_to_ipv4(clarinet_addr* restrict dst, 
                          const clarinet_addr* restrict src);

/** 
 * Converts the IPv4 address pointed by src into an IPv4MappedToIPv6 address and copies it into the memory pointed by 
 * dst. If src points to an IPv6 address then a simple copy is performed. On success returns CLARINET_ENONE. 
 * If either dst or src are NULL or src does not point to an IPv4 address then CLARINET_EINVAL is returned instead.
 */                                      
CLARINET_API
int
clarinet_addr_map_to_ipv6(clarinet_addr* restrict dst, 
                          const clarinet_addr* restrict src);
                                       
/**
 * Converts the addres pointed by src into a string in Internet standard format and store it in the buffer pointed by 
 * dst. CLARINET_EINVAL is returned if either src or dst are NULL, if the address pointed by src is invalid or dstlen 
 * is not enough to contain the nul-terminated string. On success returns the number of characters written into dst not 
 * counting the terminating null character. IPv4 addresses are converted to decimal form ddd.ddd.ddd.ddd while IPv6 
 * addresses are converted according to RFC4291 and RFC5952 which favors the more compact form when more than one 
 * representation is possible. Additionally a numeric scope id may be appended following a '%' sign when the address 
 * scope id is non-zero. Addresses with a text scope id or an empty scope id are not supported. 
 * E.g: '::1%eth0' or '::1%', will never be produced and cannot be converted into valid address structures using 
 * clarinet_addr_from_string(). On the other hand '::1' and '::1%0' are both valid representations containing a 
 * numeric scope_id of zero although this function would only ever produce the former. Note that CLARINET_EINVAL is 
 * returned instead of CLARINET_ENOTSUP when src is a well formed IPv6 address but the library has not been compiled 
 * with ipv6 support. The src parameter in this case is considered invalid. This is consistent with the baehaviour of 
 * clarinet_address_from_string() which cannot determine if a string is a valid IPv6 address when the library has no 
 * IPv6 support compiled and thus has no other choice but to return CLARINET_EINVAL. 
 * There are valid cases when the resulting string may not match an orignal string used to initialize the address E.g.: 
 * '::1%00012345' is invalid because leading zeros are not allowed in the scope id but 
 * '0000:0000:0000:0000:0000:0000:0000:1%00012345' is valid and when converted back the result will be the shorter form 
 * '::1%12345'. Some platforms may tolerate the IPv4 dot-decimal notation inside an IPv6 address string to contain 
 * leading zeros (e.g: windows) because the IPv6 format specification is unambiguous about the dot-notation (octal 
 * numbers are not allowed). Yet some platforms may disallowed it completely (e.g: linux). As a rule of thumb leading 
 * decimal zeros should be avoided.
 */
CLARINET_API
int 
clarinet_addr_to_string(char* restrict dst,
                        size_t dstlen,
                        const clarinet_addr* restrict src);
                                                 

/**
 * Converts the string pointed by src into an address and stores it in the buffer pointed by dst. srclen must contain 
 * the size of the string pointed by src not counting the termination character. If src does not point to a valid 
 * address representation the function returns CLARINET_EINVAL. If either dst or src are NULL or src does not contain 
 * a valid address representation with exact srclen size the conversion fails and the function returns CLARINET_EINVAL. 
 * On success, returns CLARINET_ENONE. Note that leading zeros are not allowed in the ipv4 decimal notation and neither 
 * in the ipv6 scope id.
 */
CLARINET_API 
int 
clarinet_addr_from_string(clarinet_addr* restrict dst,
                          const char* restrict src,
                          size_t srclen);


/**
 * Converts the endpoint pointed by src into a string in Internet standard format and store it in the buffer pointed by
 * dst. CLARINET_EINVAL is returned if either src or dst are NULL, if the address pointed by src is invalid or dstlen is
 * not enough to contain the complete string. On success returns the number of characters written to dst not counting 
 * the terminating null character. Note that a port number is always included even if the port number is zero. If this 
 * is not desired, one can always check the port number is zero and use clarinet_addr_to_string passing the endpoint's 
 * addr field instead. 
 * There are valid cases when the resulting string may not match an orignal string used to initialize the endpoint. 
 * E.g.: '[::1%00012345]:00123' is not valid because of leading zeros in the scope id and port number but 
 * '[0000:0000:0000:0000:0000:0000:0000:1%12345]:123' will produce an IPv6 endpoint with scope_id = 12345 and port 
 * number = 123. When this endpoint is converted back to a string the result will be the shortest form 
 * '[::1%12345]:123'.
 */
CLARINET_API 
int 
clarinet_endpoint_to_string(char* restrict dst,
                            size_t dstlen,
                            const clarinet_endpoint* restrict src);

/**
 * Converts the string pointed by src into an endpoint and stores it in the buffer pointed by dst. srclen must contain
 * the size of the string pointed by src not counting the termination character. If src does not point to a valid
 * endpoint representation the function returns CLARINET_EINVAL. If either dst or src are NULL or src does not contain
 * a valid endpoint representation with exact srclen size the conversion fails and the function returns CLARINET_EINVAL.
 * On success, returns CLARINET_ENONE. Note that a string without port number is not a valid endpoint representation.
 * Leading zeros are not allowed in the port number, in the ipv4 decimal fields or in the ipv6 scope id.
 */
CLARINET_API
int
clarinet_endpoint_from_string(clarinet_endpoint* restrict dst,
                              const char* restrict src,
                              size_t srclen);


/***********************************************************************************************************************
 * SOCKET
 *
 * All basic socket operations are non-blocking. There is no point in having blocking operations because to efficiently 
 * support user space protocols the network stack is modelled assuming a lockstep loop with bottom-up update and 
 * top-down flush. It must be possible to handle multiple sockets in a single thread and a layer must be able to respond
 * to time events even in the abscense of incoming data.
 *
 * All sockets are represented as pointers to opaque types and thus can only be manipulated by their corresponding 
 * protocol API.
 *
 * The socket is open and bound to a local endpoint in a single operation. All the options provided in settings are 
 * applied BEFORE binding and cannot be modifiedr by calling clarinet_xxx_setopt(). By default, sockets are blocking 
 * with dual stack disabled despite some operating systems having a specific configuration for it 
 * (e.g. sysctl:/proc/sys/net/ipv6/bindv6only on Linux). This strategy provides consistency and predictability. See 
 * clarinet_socket_option for details. Send/Recv timeouts are not defined because they do not apply to non-blocking
 * sockets.
 *
 * By default, no two sockets can be bound to the same combination of source address and source port. As long as the
 * source port is different, the source address is actually irrelevant. Binding socketA to ipA:portA and socketB to
 * ipB:portB is always possible if ipA != ipB holds true, even when portA == portB. E.g. socketA belongs to a FTP server
 * program and is bound to 192.168.0.1:21 and socketB belongs to another FTP server program and is bound to 10.0.0.1:21,
 * both bindings will succeed. Keep in mind, though, that a socket may be locally bound to "any address". If a socket is
 * bound to 0.0.0.0:21, it is bound to all existing local addresses at the same time and in that case no other socket
 * can be bound to port 21, regardless which specific IP address it tries to bind to, as 0.0.0.0 conflicts with all
 * existing local IP addresses.
 * 
 * Binding with CLARINET_SO_REUSEADDR always implies exact address reuse unless explicitly unsupported by the platform
 * in which case at least the default behaviour of SO_REUSEADDR as described in BSD is guaranteed. On Windows, Linux, 
 * macOS and any other BSD compatible platform using CLARINET_SO_REUSEADDR allows a socket to reuse the exact same
 * source address and source port of another socket previously bound as long as that other socket also had
 * CLARINET_SO_REUSEADDR enabled. This is accomplished by setting SO_REUSEPORT|SO_REUSEADDR on macOS and Linux when
 * CLARINET_SO_REUSEADDR is enabled. On Windows SO_REUSEADDR is set when CLARINET_SO_REUSEADDR is enabled and
 * SO_EXCLUSIVEADDRUSE is set when CLARINET_SO_REUSEADDR is disabled.
 * 
 *
 * There is no clarinet_udp_connect() because dgram delivery rules may be quite different between platforms. On Unix 
 * (including macOS) when a UDP socket is bound to a foreign address by connect() it effectively assumes a 5-tuple 
 * identity so when a dgram arrives the system first selects all sockets associated with the src address of the 
 * packet and then picks the socket with the most specific local address matching the destination address of the 
 * packet. On windows however, UDP associations established with connect() do not affect routing. They only serve as 
 * defaults for send() and recv() so on Windows all UDP sockets have a foreign address *:* and the first entry 
 * on the routing table with a local address that matches the destination address of the arriving packet is picked. This
 * basically prevents UDP servers from ever using connect() and operate with multiple sockets as with TCP. Besides 
 * "connected" UDP sockets by definition prevent upper layers from implementing any support to IP mobility.
 *
 * Note that besides platform support, dual-stack also requires a local IPv6 address (either an explicit one or the 
 * wildcard [::] which is equivalent to CLARINET_IPV6_ANY). The ability to interact with IPv4 addresses requires the use 
 * of the IPv4MappedToIPv6 address format. Any IPv4 addresses must be represented in the IPv4MappedToIPv6 address format 
 * which enables an IPv6-only application to communicate with an IPv4 node. The IPv4MappedToIPv6 address format allows 
 * the IPv4 address of an IPv4 node to be represented as an IPv6 address. The IPv4 address is encoded into the low-order
 * 32 bits of the IPv6 address, and the high-order 96 bits hold the fixed prefix 0:0:0:0:0:FFFF. The IPv4MappedToIPv6 
 * address format is specified in RFC4291. Applications must take care to handle these IPv4MappedToIPv6 addresses 
 * appropriately and only use them with dual stack sockets. If an IP address is to be passed to a regular IPv4 socket, 
 * the address must be a regular IPv4 address not a IPv4MappedToIPv6 address. 
 *
 * An application with a socket bound to [::] (IPv6) and dual-stack enabled occupies the port on both IPv6 and IPv4. 
 * Therefore a second socket cannot be bound to 0.0.0.0 (IPv4 only) with the same protocol on the same port unless 
 * CLARINET_SO_REUSEADDR is used. Note however that in this particular case it is impossible to determine which 
 * socket will handle incoming IPv4 packets and behaviour will depend on the operating system.
 *
 **********************************************************************************************************************/
 
/** 
 * Socket options corresponding to flags are conveniently defined in the range [1, 32] so they can be internally 
 * converted to flags more efficiently using clarinet_socket_option_to_flag. All other options are associated with 
 * non-boolean values and thus are defined in the range [33, 65535].
 */
enum clarinet_socket_option
{
    CLARINET_SO_NONE = 0,
    
    /* Flags */
    CLARINET_SO_REUSEADDR,
    CLARINET_SO_KEEPALIVE,
    CLARINET_SO_IPV6DUAL,
    
    /* Properties */
    CLARINET_SO_ERROR = 33,
    CLARINET_SO_SNDBUF,
    CLARINET_SO_RCVBUF,
    CLARINET_SO_TTL,
    CLARINET_SO_LINGER,
    CLARINET_SO_DONTLINGER, /* disable linger without affecting the timeout already configured - in TCP forces a RST and no TIME_WAIT on close*/
};

/** 
 * Converts a clarinet_socket_option 'opt' in the range [1, 32] to a bitmask = 2^(x-1) without branching. The macro 
 * produces 0 if opt <= 0 and 2^(opt-1 mod 32) if opt >= 33. 
 *
 * The number of bits in an enum clarinet_socket_option type is determined at compile time by 
 * (sizeof(clarinet_socket_option) << 3) then (opt-1) is negated and the most significant bit is shifted n-1 bits to yield 
 * either 0 or 1 and form the first operand of the actual bitmask shift. Finally ((opt-1) & 0x1F) is used as the second 
 * operand to shift left no more than 31 bits. Note that if 'opt' is a compile time constant then all this is calculated 
 * at compile time so no runtime penalty.
 */
#define clarinet_socket_option_to_flag(opt)    (uint32_t)(((~((uint32_t)((opt) -1))) >> ((sizeof(uint32_t) << 3)-1)) << (((uint32_t)((opt) -1)) & 0x1F))


/**
 * These are options the users can only set when the socket is first open. This is due to platforms providing distinct 
 * operations for creation and binding of a socket and not allowing certain options to be modified after bind. Since we 
 * combine creation and binding in a single operation there is no other opportunity to setup these options. Note that 
 * platforms may impose different limitations on each option which might also depend on system configuration. 
 * For example, Linux imposes a limit on the sizes of RCVBUF and SNDBUF which is adjustable via sysctl. 
 */
struct clarinet_socket_settings
{
    uint32_t send_buffer_size;      /* send buffer size in bytes */
    uint32_t recv_buffer_size;      /* receive buffer size in bytes */
    uint8_t ttl;                    /* packet time-to-live (hop limit on ipv6) */
};


/***********************************************************************************************************************
 * UDP
 **********************************************************************************************************************/

typedef struct clarinet_udp_socket clarinet_udp_socket;
typedef struct clarinet_socket_settings clarinet_udp_settings;

CLARINET_API 
const clarinet_udp_settings 
clarinet_udp_settings_default;

CLARINET_API 
int
clarinet_udp_open(clarinet_udp_socket** spp,
                  const clarinet_endpoint* restrict endpoint, 
                  const clarinet_udp_settings* restrict settings,
                  uint32_t flags);

CLARINET_API 
int 
clarinet_udp_close(clarinet_udp_socket** spp);

CLARINET_API
int
clarinet_udp_get_endpoint(clarinet_udp_socket* restrict sp,
                          clarinet_endpoint* restrict endpoint);

CLARINET_API 
int 
clarinet_udp_send(clarinet_udp_socket* restrict sp,
                  const void* restrict buf,
                  size_t len, 
                  const clarinet_endpoint* restrict dst);
CLARINET_API 
int 
clarinet_udp_recv(clarinet_udp_socket* restrict sp,
                  void* restrict buf, 
                  size_t len, 
                  clarinet_endpoint* restrict src);

CLARINET_API 
int 
clarinet_udp_set_option(clarinet_udp_socket* restrict sp,
                        int proto, 
                        int optname,
                        const void* restrict optval, 
                        size_t optlen);

CLARINET_API 
int 
clarinet_udp_get_option(clarinet_udp_socket* restrict sp,
                        int proto, 
                        int optname, 
                        void* restrict optval, 
                        size_t* restrict optlen);


/***********************************************************************************************************************
 * TCP
 **********************************************************************************************************************/

/* For now there is no option to disable delayed acks because only Linux and Windows seem to provide the means to do it 
 * and yet there are several conflicting details. See https://github.com/dotnet/runtime/issues/798 for a discussion on 
 * the topic. 
 */
enum clarinet_tcp_option
{
    /* Options */
    CLARINET_TCP_NODELAY = 33,
    CLARINET_TCP_KEEPCNT,
    CLARINET_TCP_KEEPIDLE,
    CLARINET_TCP_KEEPINTVL,
};

typedef struct clarinet_tcp_socket clarinet_tcp_socket;
typedef struct clarinet_socket_settings clarinet_tcp_settings;

CLARINET_API 
const clarinet_tcp_settings 
clarinet_tcp_settings_default;

CLARINET_API 
int
clarinet_tcp_listen(clarinet_tcp_socket** restrict spp,
                    const clarinet_endpoint* restrict local, 
                    const clarinet_udp_settings* restrict settings,
                    uint32_t flags);

CLARINET_API 
int 
clarinet_tcp_connect(clarinet_tcp_socket** restrict spp,
                     const clarinet_endpoint* restrict local, 
                     const clarinet_endpoint* restrict remote, 
                     const clarinet_udp_settings* restrict settings,
                     uint32_t flags);


CLARINET_API 
int 
clarinet_tcp_close(clarinet_tcp_socket** spp);

CLARINET_API
int
clarinet_tcp_get_endpoint(clarinet_tcp_socket* restrict sp,
                          clarinet_endpoint* restrict endpoint);

CLARINET_API 
int 
clarinet_tcp_send(clarinet_tcp_socket* restrict sp,
                  const void* restrict buf,
                  size_t len, 
                  const clarinet_endpoint* restrict dst);

CLARINET_API 
int 
clarinet_tcp_recv(clarinet_tcp_socket* restrict sp,
                  void* restrict buf, 
                  size_t len, 
                  clarinet_endpoint* restrict src);

CLARINET_API 
int 
clarinet_tcp_setopt(clarinet_tcp_socket* restrict sp,
                    int proto, 
                    int optname,
                    const void* restrict optval, 
                    size_t optlen);

CLARINET_API 
int 
clarinet_tcp_getopt(clarinet_tcp_socket* restrict sp,
                    int proto, 
                    int optname, 
                    void* restrict optval, 
                    size_t* restrict optlen);




/***********************************************************************************************************************
 * DTLC
 *
 * Creates a DTLC socket. DTLC (Datagram Transport Layer Connectivity) is a custom lightweight alternative to DTLS 
 * that offers end-to-end connectivity, integrity check, optional IP mobility and optional encryption but does not
 * support either server or client authentication.
 * 
 * The underlying UDP socket must be non-blocking so the flag CLARINET_UDP_FLAG_NONBLOCK is automatically added if not 
 * provided by the user in clarinet_enet_open().
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * DTLS
 *
 * Creates a DTLS 1.2 socket as specified by RFC6347. Optional IP mobility is implemented with the connection id 
 * extension proposed by Rescorla et al. in "Connection Identifiers for DTLS 1.2 (draft-ietf-tls-dtls-connection-id-13)"
 * <https://datatracker.ietf.org/doc/draft-ietf-tls-dtls-connection-id/>
 *
 * The underlying UDP socket must be non-blocking so the flag CLARINET_UDP_FLAG_NONBLOCK is automatically added if not 
 * provided by the user in clarinet_enet_open().
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
 * employed a single update function (enet_host_service()) that also serves to poll for network events whereas all 
 * protocol interfaces implemeted by Clarinet communicate network events when a recv is performed and rely on two 
 * separate update chains - one for receiving data up the stack (update) and another for sending data down the stack 
 * (flush).
 *
 * The underlying UDP socket must be non-blocking so the flag CLARINET_UDP_FLAG_NONBLOCK is automatically added if not 
 * provided by the user in clarinet_enet_open().
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * ENETS
 *
 * Creates an ENet socket based on a non-blocking DTLS socket instead of UDP.
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * TLS
 **********************************************************************************************************************/


#if defined(__cplusplus)
#if defined(restrict) && !RESTRICT_PREDEFINED
#undef restrict
#endif
#endif

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif /* CLARINET_H */
