/**
 * @defgroup library Library Info
 *
 * @defgroup addr Addresses
 *
 * Network layer address definitions and transport endpoint definitions are common to UDP and TCP.
 *
 * Custom types are defined for mac,  inet and inet6 addresses to mantain the public API as system agnostic as possible
 * and avoid creating a dependency on non-standard headers for <tt>struct sockaddr, struct sockaddr_in,
 * struct sockaddr_in6 and sockaddr_storage</tt>.
 *
 * @defgroup iface Interfaces
 *
 * @defgroup socket Sockets
 *
 * This module provides a socket abstraction with consistent behaviour across multiple platforms and when complete
 * conformity is not entirely possible, a clear definition of the differences with minimum divergence.
 *
 * Only UDP and TCP sockets are supported out-of-the box. Refer to @ref clarinettls for TLS/DTLS support. For an
 * alternative connection-oriented protocol with support to IP mobility, security and partial reliability refer to
 * @ref clarinetdtp.
 *
 * @par Details
 *
 * A socket must be open and bound to a local address in two distinct operations following the same conventions of
 * BSD-sockets. Note that certain options may only be set BEFORE the socket is bound
 *
 * Default socket options may vary according to the platform. Some platforms may even provide system wide settings in
 * which case only the programmer can decide whether or not an application should override a certain option.
 * For example, dual stack support  (@c CLARINET_SO_IPV6ONLY) on Linux has a global default value defined in
 * /proc/sys/net/ipv6/bindv6only.
 *
 * Normally two sockets with the same protocol cannot be bound to the same local address and port. This is deemed a bind
 * conflict. Binding socket @a A to proto/ipA:portA and socket @a B to proto/ipB:portB is always possible if either
 * portA != portB or proto/ipA != proto/ipB, where proto is either UDP or TCP. E.g. socket @a A belongs to an FTP server
 * that is bound to 192.168.0.1:21 and socket @a B belongs to another FTP server program bound to 10.0.0.1:21, both
 * bindings will succeed. Keep in mind, though, that a socket may be locally bound to "any address" also denoted a
 * wildcard which is represented by the address 0.0.0.0 on ipv4 and :: on ipv6. If a socket is bound to 0.0.0.0:21, it
 * is effectively bound to *all* existing local addresses at the same time in ipv4 space in which case no other socket
 * can be bound to port 21 in the same address space, regardless of which specific IP address it tries to bind to since
 * the wildcard conflicts with all existing local addresses in its space. This is of particular significance when
 * binding to :: with dual stack mode enabled because then the ipv6 wildcard :: occupies all addresses in both ipv6 and
 * ipv4 space. See clarinet_socket_open() for details.
 *
 * @c clarinet_socket_connect() has different semantics for UDP and TCP and a slightly different behaviour for UDP
 * depending on the platform. On Unix (including macOS), when a UDP socket is associated with a foreign address by
 * @c clarinet_socket_connect(), it effectively assumes a 5-tuple identity so when a datagram arrives, the system first
 * selects all sockets associated with the src address of the packet and then selects the socket with the most specific
 * local address matching the destination address of the packet. On Windows, however, UDP associations established with
 * @c clarinet_socket_connect() do not affect routing. They only serve as defaults for @c clarinet_socket_send() and
 * @c clarinet_socket_recv() so [on Windows] all UDP sockets have a foreign address *:* and the first entry in the
 * routing table with a local address that matches the destination address of the arriving packet is picked (generally
 * the last socket open on the same port). This basically prevents UDP servers from ever using
 * @c clarinet_socket_connect() to operate with multiple client sockets like TCP does.
 *
 * Besides platform support, dual-stack also requires a local IPv6 address (either an explicit one or the ipv6 wildcard
 * which is defined by the global variable @c clarinet_addr_ipv6_any. The ability to interact with IPv4 hosts requires
 * the use of an ipv4-mapped-to-ipv6 address format. Any IPv4 address must be represented in this ipv4-mapped-to-ipv6
 * address format which enables an IPv6-only application to communicate with an IPv4 node. The ipv4-mapped-to-ipv6
 * address format allows the IPv4 address of an IPv4 node to be represented as an IPv6 address. The IPv4 address is
 * encoded into the low-order 32 bits of the IPv6 address, and the high-order 96 bits hold the fixed prefix
 * 0:0:0:0:0:FFFF. The ipv4-mapped-to-ipv6 address format is specified in
 * <a href="https://datatracker.ietf.org/doc/html/rfc4291">RFC4291</a>. Applications must take care to handle these
 * ipv4-mapped-to-ipv6 addresses appropriately and only use them with dual stack sockets. If an IP address is to be
 * passed to a regular IPv4 socket, the address must be a regular IPv4 address not an ipv4-mapped-to-ipv6 address.
 *
 * An application with a socket bound to [::] (IPv6) and dual-stack enabled occupies the port on both ipv6 and ipv4
 * space. Therefore, a second socket cannot be bound to 0.0.0.0 (IPv4 only) with the same protocol on the same port
 * unless @c CLARINET_SO_REUSEADDR is used. Note however that in this case it becomes impossible to determine which
 * socket will handle an incoming IPv4 packet and behaviour will depend on the platform.
 *
 * All clarinet socket options have a UNIQUE integer identifier (aka @a optname) across all levels/protocols. This is
 * important so the user does not have to pass a level/protocol identifier as well. Uniqueness in this case is not just
 * a matter of convenience but safety and sanity. Consider the well-known @c setsockopt(2) function. When option levels
 * are allowed to have colliding identifiers there is always a chance the user might pass the wrong @a optlevel by
 * mistake and yet have no error reported because the mistaken option level happens to define another @a optname with
 * the same identifier. Such mistakes can be hard to detect and can be fairly common with low value option identifiers
 * (i.e. 1, 2, 3,...).
 *
 * @file
 *
 * @note All boolean values are represented as integers where 0 evaluates to false and any non-zero value evaluates to
 * true, including negative values! Mind that <tt>if (var == 0)</tt> is equivalent to <tt>if (var == FALSE)</tt> but the
 * opposite is not necessarily true, that is, <tt>if (var == 1)</tt> IS NOT equivalent to <tt>if (var == TRUE)</tt>.
 * Boolean variables should always be evaluated implicitly as in <tt>if (var) { ... }</tt> instead.
 *
 * @note Public macros that receive arguments are normally defined in lower case following the same name convention of
 * functions, so no builds should break if we eventually replace those macros with actual function calls.
 *
 * @note Unfortunately, struct/union initialization in C/C++ is a mess. C99 supports member designators but C++ does
 * not. C99 supports compound literals but C++ does not, save for certain compilers with custom extensions. Currently,
 * GCC and CLANG are known to support compound literals in C++ but MSVC only supports it for C and not C++. The solution
 * for now is to export global const variables which can be used for local assignment in C++ without having to resort to
 * compound literals.
 *
 * @note All structs and @c clarinet_addr in particular must be compared member by member. It's not safe to compare
 * structs using @c memcmp() because content of padding spaces is undefined and @c memcmp() will blindly compare every
 * byte of allocated memory.
 *
 * @note Unfortunately, C99 does not support anonymous unions, so we have to use an additional member for those.
 * Anonymous bitfields are not supported either, so we have to rely on named members for padding.
 *
 * @note Structs and unions declared just for structural purposes do not have typedefs because users should not normally
 * have to deal with them and if they ever do it is best they have to be explicit about it.

 * @note In the POSIX standard, names ending with @a _t are reserved. Since we're targeting at least one POSIX system
 * (i.e. Linux) typenames defined in this file NEVER end with @a _t.
 *
 */
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

/** region Utility Macros */

#define CLARINET_STR(s)     #s
#define CLARINET_XSTR(s)    CLARINET_STR(s)

/* @formatter:off */

/** Check whether this is GCC major.minor or a later release. */
#if !defined(__GNUC__)
    /* Not GCC and not "just like GCC" */
    #define CLARINET_IS_AT_LEAST_GNUC_VERSION(major, minor) 0
#else
    /* GCC or "just like GCC" */
    #define CLARINET_IS_AT_LEAST_GNUC_VERSION(major, minor) (__GNUC__ > (major) || (__GNUC__ == (major) && __GNUC_MINOR__ >= (minor)))
#endif

/** Check whether this is CLANG major.minor or a later release. */
#if !defined(__clang__)
    /* Not CLANG */
    #define CLARINET_IS_AT_LEAST_CLANG_VERSION(major, minor) 0
#else
    /* CLANG */
    #define CLARINET_IS_AT_LEAST_CLANG_VERSION(major, minor) (__clang_major__ > (major) || (__clang_major__ == (major) && __clang_minor__ >= (minor)))
#endif

#if defined(__clang__) /* CLANG */
    #define CLARINET_UNUSED __attribute__((unused))
#elif defined(__JETBRAINS_IDE__)
    #define CLARINET_UNUSED __attribute__((unused))
#else
    /* GCC and MSVC cannot detect unused struct fields so there is no point in flagging them */
    #define CLARINET_UNUSED
#endif

/* @formatter:on*/

/* endregion */

/* region API symbols */

/**
 * @file
 *
 * @note The macro @c CLARINET_EXPORT is defined by the build system when building as a shared library. In this case
 * we can arrange to export only the necessary symbols by defining @c CLARINET_EXTERN. Similarly @c CLARINET_IMPORT is
 * defined by the build system for targets consuming this header with a shared library. It is an error to have both @c
 * CLARINET_EXPORT and @c CLARINET_IMPORT defined at the same time.
 *
 * @note @b WINDOWS: According to the documentation
 * <a href="https://docs.microsoft.com/en-us/cpp/build/importing-into-an-application-using-declspec-dllimport?view=msvc-160">
 * here </a> headers accompanying DLLs should declare consumed symbols with @c __declspec(dllimport) because the
 * compiler can alledgedly produce more efficient code if that attribute is present.
 */

/* @formatter:off */

#if defined(CLARINET_EXPORT) && defined(CLARINET_IMPORT)
    #error "Define either CLARINET_EXPORT or CLARINET_IMPORT but not both."
#endif

#if defined(_WIN32)
    #if defined(CLARINET_EXPORT)
        #define CLARINET_VISIBLITY     __declspec(dllexport)
    #elif defined(CLARINET_IMPORT)
        #define CLARINET_VISIBLITY     __declspec(dllimport)
    #else
        #define CLARINET_VISIBLITY
    #endif
#else /* UN*X */
    #if defined(CLARINET_EXPORT)
        #if CLARINET_IS_AT_LEAST_GNUC_VERSION(3,4)
            /* GCC 3.4 and later (or some compiler asserting compatibility with
             * GCC 3.4 and later) so we have __attribute__((visibility()). */
            #define CLARINET_VISIBLITY __attribute__((visibility("default")))
        #elif defined(__clang__) /* CLANG */
            #define CLARINET_VISIBLITY __attribute__((visibility("default")))
        #else
            #define CLARINET_VISIBLITY
        #endif
    #else
        #define CLARINET_VISIBLITY
    #endif
#endif

#define CLARINET_EXTERN CLARINET_VISIBLITY extern

#if defined(_WIN32)
    #define CLARINET_CALLBACK __cdecl
#else
    #define CLARINET_CALLBACK
#endif

/* Replace 'restrict' in C++ with something supported by the compiler. MSVC Intellisense doesn't like the "restrict"
 * keyword either. */
#ifdef __INTELLISENSE__
    #ifndef restrict
        #define restrict
    #endif
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
/* @formatter:on */

/** Declare enum item inside of an enum { } */
#define CLARINET_DECLARE_ENUM_ITEM(e, v, s) e = (v), /**< s */

/* endregion */

/* region Error Codes */

#define CLARINET_ERRORS(E) \
    E(CLARINET_ENONE,             0, "Success") \
    E(CLARINET_EDEFAULT,         -1, "Unspecified error") \
    E(CLARINET_ESYS,             -2, "Internal system error") \
    E(CLARINET_EPERM,            -3, "Operation is not permitted") \
    E(CLARINET_ENOTIMPL,         -4, "Operation is not implemented") \
    E(CLARINET_EINTR,            -5, "Operation interrupted") \
    E(CLARINET_EIO,              -6, "I/O error") \
    E(CLARINET_ENOMEM,           -7, "Not enough memory") \
    E(CLARINET_EACCES,           -8, "Access denied") \
    E(CLARINET_EINVAL,           -9, "Invalid argument") \
    E(CLARINET_ENOTREADY,       -10, "Underlying system or device not ready") \
    E(CLARINET_ENOTFOUND,       -11, "Data not found") \
    E(CLARINET_EAGAIN,          -12, "Operation could not be completed immediately or resource temporarily unavailable") \
    E(CLARINET_EALREADY,        -13, "Operation already performed") \
    E(CLARINET_EINPROGRESS,     -14, "Operation is already in progress") \
    E(CLARINET_ENOTSOCK,        -15, "Operation attempted with an invalid socket") \
    E(CLARINET_EMSGSIZE,        -16, "Message too large") \
    E(CLARINET_ENOTSUP,         -17, "Operation is not supported") \
    E(CLARINET_ENOBUFS,         -18, "Not enough buffer space or queue is full") \
    E(CLARINET_EAFNOSUPPORT,    -20, "Address family not supported") \
    E(CLARINET_EPROTONOSUPPORT, -21, "Protocol is not supported") \
    E(CLARINET_EADDRINUSE,      -22, "Address already in use") \
    E(CLARINET_EADDRNOTAVAIL,   -23, "Address is not available/cannot be assigned") \
    E(CLARINET_ENETDOWN,        -24, "Network is down") \
    E(CLARINET_ENETUNREACH,     -25, "Network is unreachable") \
    E(CLARINET_ENETRESET,       -26, "Network reset possibly due to keepalive timeout") \
    E(CLARINET_ENOTCONN,        -27, "Socket is not connected")    \
    E(CLARINET_EISCONN,         -28, "Socket is already connected") \
    E(CLARINET_ECONNABORTED,    -29, "Connection aborted") \
    E(CLARINET_ECONNRESET,      -30, "Connection reset by peer") \
    E(CLARINET_ECONNSHUTDOWN,   -31, "Connection is shutdown") \
    E(CLARINET_ECONNTIMEOUT,    -32, "Connection timeout") \
    E(CLARINET_ECONNREFUSED,    -33, "Connection refused") \
    E(CLARINET_EHOSTDOWN,       -34, "Host is down") \
    E(CLARINET_EHOSTUNREACH,    -35, "No route to host")  \
    E(CLARINET_EPROTO,          -36, "Protocol error") \
    E(CLARINET_EPROCLIM,        -37, "Too many processes or tasks") \
    E(CLARINET_EMFILE,          -38, "Too many files") \
    E(CLARINET_ELIBACC,         -39, "Cannot access a needed shared library") \
    E(CLARINET_ELIBBAD,         -40, "Accessing a corrupted shared library") \

/**
 * Error codes that can be returned by the library functions. Valid error numbers are all negative integers and all
 * symbolic names have distinct numeric values associated.
 */
enum clarinet_error
{
    CLARINET_ERRORS(CLARINET_DECLARE_ENUM_ITEM)
};

#define CLARINET_ERROR_NAME_INVALID             "(invalid)"
#define CLARINET_ERROR_NAME_UNDEFINED           "(undefined)"

#define CLARINET_ERROR_DESC_INVALID             "Invalid error code"
#define CLARINET_ERROR_DESC_UNDEFINED           "Undefined error code"

/**
 * @brief Obtains the symbolic name associated with an error code.
 *
 * @param [in] errcode: Predefined error code.
 *
 * @return This function returns a pointer to a constant string corresponding to the symbolic name of the error code
 * passed in the argument @p errcode.
 * @return  @c CLARINET_ERROR_NAME_INVALID is returned if @p errcode is a positve integer greater than zero.
 * @return  @c CLARINET_ERROR_NAME_UNDEFINED is returned if @p errcode is a negative integer not mapped to a symbolic
 * error name.
 *
 * @see @c clarinet_error
 */
CLARINET_EXTERN
const char*
clarinet_error_name(int errcode);

/**
 * @brief Obtains the description associated with an error code.
 *
 * @param [in] errcode: Predefined error code.
 *
 * @return This function returns a pointer to a constant string corresponding to the description of the error code
 * passed in the argument @p errcode.
 * @return  @c CLARINET_ERROR_DESC_INVALID is returned if @p errcode is a positve integer greater than zero.
 * @return  @c CLARINET_ERROR_DESC_UNDEFINED is returned if @p errcode is a negative integer not mapped to a symbolic
 * error name.
 *
 * @see @c clarinet_error
 */
CLARINET_EXTERN
const char*
clarinet_error_description(int err);

/* endregion */

/* region Family Codes */


/* Address Families */

#define CLARINET_FAMILIES(E) \
    E(CLARINET_AF_UNSPEC,        0, "Unspecified") \
    E(CLARINET_AF_INET,          2, "IPv4") \
    E(CLARINET_AF_INET6,        10, "IPv6") \
    E(CLARINET_AF_LINK,         18, "MAC") \

/** Socket address families recognized by the library. */
enum clarinet_family
{
    CLARINET_FAMILIES(CLARINET_DECLARE_ENUM_ITEM)
};

#define CLARINET_FAMILY_NAME_INVALID        "(invalid)"

#define CLARINET_FAMILY_DESC_INVALID        "Invalid address family"

/**
 * @brief Obtains the symbolic name of an address family code.
 *
 * @param [in] family: Predefined address family code.
 *
 * @return This function returns a pointer to a constant string corresponding to the symbolic name of the address family
 * passed in the argument @p family.
 * @return  @c CLARINET_FAMILY_NAME_INVALID is returned if @p family is a recognized address family.
 *
 * @see @c clarinet_family
 */
CLARINET_EXTERN
const char*
clarinet_family_name(int family);

/**
 * @brief Obtains the symbolic name of an address family code.
 *
 * @param [in] family: Predefined address family code.
 *
 * @return This function returns a pointer to a constant string corresponding to the symbolic name of the address family
 * passed in the argument @p family.
 * @return  @c CLARINET_FAMILY_NAME_INVALID is returned if @p family is a recognized address family.
 *
 * @see @c clarinet_family
 */
CLARINET_EXTERN
const char*
clarinet_family_description(int family);

/* endregion */

/* region Protocol Codes */

#define CLARINET_PROTOS(E) \
    E(CLARINET_PROTO_NONE,      0x00000000, "None") \
    E(CLARINET_PROTO_UDP,       0x00000004, "User Datagram Protocol (RFC768)") \
    E(CLARINET_PROTO_TCP,       0x00000008, "Transmission Control Protocol (RFC793)") \
    E(CLARINET_PROTO_DTLC,      0x00000200, "Datagram Transport Layer Connectivity (Custom protocol over UDP)") \
    E(CLARINET_PROTO_DTLS,      0x00000400, "Datagram Transport Layer Security (RFC6347)") \
    E(CLARINET_PROTO_TLS,       0x00000800, "Transport Layer Security (RFC8446)") \
    E(CLARINET_PROTO_GDTP,      0x00100000, "Game Data Transport Protocol (Custom protocol over DTLC)") \
    E(CLARINET_PROTO_GDTPS,     0x00200000, "Game Data Transport Protocol Secure (UDT over DTLS)") \
    E(CLARINET_PROTO_ENET,      0x00400000, "ENet (Custom protocol based on http://enet.bespin.org/index.html)") \
    E(CLARINET_PROTO_ENETS,     0x00800000, "ENet Secure (Custom ENet over DTLS)") \


enum clarinet_proto
{
    CLARINET_PROTOS(CLARINET_DECLARE_ENUM_ITEM)
};

CLARINET_EXTERN
const char*
clarinet_proto_name(int err);

CLARINET_EXTERN
const char*
clarinet_proto_description(int proto);

/* endregion */

/* region Feature Flags */

#define CLARINET_FEATURE_NONE       0x00    /**< None */
#define CLARINET_FEATURE_DEBUG      0x01    /**< Debug information built-in */
#define CLARINET_FEATURE_PROFILE    0x02    /**< Profiler instrumentation built-in */
#define CLARINET_FEATURE_LOG        0x04    /**< Log built-in */
#define CLARINET_FEATURE_IPV6       0x08    /**< Support for IPv6 */
#define CLARINET_FEATURE_IPV6DUAL   0x10    /**< Support for IPv6 in dual-stack mode */

/* endregion */

/* region Socket Options (each one must have a unique identifier) */

/**
 * Enable/disable non-blocking mode. @a optval is @c uint32_t. Valid values are limited to 0 (false) and non-zero
 * (true). This option is write-only.
 */
#define CLARINET_SO_NONBLOCK        1

/**
 * Controls how @c clarinet_socket_bind() should handle local address/port conflicts. @a optval is a 32-bit integer.
 * Valid values are limited to 0 (false) and non-zero (true).
 *
 * @details A <b>partial conflict</b> is said to occur when a socket tries to bind to a specific local address despite a
 * pre-existing socket bound to a wildcard in the same address space.
 *
 * @details An <b>exact conflict</b> occurs when a socket tries to bind to the EXACT same local address/port of a
 * pre-existing socket regardless of the local address being specific or a wildcard.
 *
 * @details When a socket is allowed to bind despite of a conflict it is said to be reusing the address/port. Not all
 * underlying systems can provide the same level of support to address/port reuse and a few discrepancies are
 * inevitable. Also note that even if broadcasting and multicasting are disregarded, address reuse has different
 * implications in UDP and TCP sockets because there is no TIME_WAIT state involved in UDP.
 *
 * @note For this option to have any effect it must be set before calling @c clarinet_socket_bind(), otherwise
 * behaviour is undefined.
 *
 * @note For unicast UDP sockets, underlying implementations are known to operate as follows:
 *
 * @note @b LINUX @< 3.9: The flag @c SO_REUSEADDR allows exact same address/port reuse but is required in all sockets
 * sharing address/port including wildcard sockets. Sockets can be from any process.
 *
 * @note @b LINUX @>= 3.9: The flag @c SO_REUSEPORT behaves exactly like old @c SO_REUSEADDR but restricts reuse to
 * sockets created by the same effective UID to prevent hijacking. @c SO_REUSEADDR is still defined for compatibility
 * but is not required if @c SO_REUSEPORT si enabled. Kernel provides recv load balancing between sockets bound to the
 * exact same address/port.
 *
 * @note @b BSD/DARWIN: The flag @c SO_REUSEADDR allows a socket to bind a specific address/port while another socket
 * holds a wildcard address on the same port regardless of the wildcard socket having @c SO_REUSEADDR. It does not allow
 * exact same address/port reuse. Two wildcard sockets on the same port are considered an exact conflict.
 * @c SO_REUSEPORT allows exact same address/port reuse in which case it is required on all conflicting sockets. When
 * there is a partial conflict some BSD systems still require SO_REUSEADDR to allow address reuse but on modern systems
 * such as Darwin, @c SO_REUSEPORT behaves like @c SO_REUSEADDR in this case and allows a specific socket to bind on top
 * of a previous wildcard socket with the same port regardless of the wildcard socket having @c SO_REUSEPORT itself.
 * In any case only exact conflicts require all sockets involved to have @c SO_REUSEPORT. An alternative flag
 * @c SO_REUSEPORT_LB can be used on modern systems to provide load balancing like @c SO_REUSEPORT on Linux.
 *
 * @note @b WINDOWS: The flag @c SO_REUSEADDR allows exact same address/port reuse if the other socket also has
 * @c SO_REUSEADDR defined. This includes exact same and wildcard conflicts. SO_EXCLUSIVEADDRUSE can be used to disallow
 * reuse address by other sockets. This applies to specific and wildcard sockets. @c SO_EXCLUSIVEADDRUSE cannot be used
 * with @c SO_REUSEADDR in the same socket. A wildcard that specifies @c SO_EXCLUSIVEADDRUSE occupies the whole address
 * space but does not disallow reuse by a specifc addresses socket even if the socket does not have @c SO_REUSEADDR
 * enabled which can be confusing. The following table shows all possible conflicts of wildcard and specific sockets and
 * the result depending on what flags are enabled
 * [<a href="https://docs.microsoft.com/en-us/windows-hardware/drivers/network/sharing-transport-addresses">source</a>]:
 *
 * @note @verbatim
 *
 *                                   +--------------------------------------------+
 *                                   |                  FIRST SOCKET              |
 *                                   |-------+--------------+---------------------|
 *                                   |  NONE | SO_REUSEADDR | SO_EXCLUSIVEADDRUSE |
 *                                   |  W S  |     W S      |        W S          |
 * +--------+------------------------+-------+--------------+---------------------|
 * |        |                NONE W  |  0 1  |     0 1      |        0 1          |
 * |        |                     S  |  ? 0  |     ? 0      |        0 0          |
 * |        +------------------------+-------+--------------+---------------------|
 * | SECOND |        SO_REUSEADDR W  |  0 1  |     1 1      |        0 1          |
 * | SOCKET |                     S  |  ? 0  |     1 1      |        0 0          |
 * |        +------------------------+-------+--------------+---------------------|
 * |        | SO_EXCLUSIVEADDRUSE W  |  0 0  |     0 0      |        0 0          |
 * |        |                     S  |  ? 0  |     ? 0      |        0 0          |
 * +--------+------------------------+-------+--------------+---------------------|
 *  W = wildcard; S = specific;
 *  0 = failure; 1 = success; ? = depends on security credentials probably success
 *
 * @endverbatim
 *
 * @note @b SOLARIS: There is no @c SO_REUSEPORT. @c SO_REUSEADDR only allows partial conflicts (i.e. specific to
 * wildcard). @c SO_EXCLBIND is like @c SO_EXCLUSIVEADDRUSE on Windows and can be used to disallow any reuse of specifc
 * addresses with the same port as the wildcard already bound.
 *
 * @note @b ANDROID: It has a Linux kernel so should be identical to LINUX
 *
 * @note @b IOS: It (supposedly) has a macOS kernel so should be identical to BSD/DARWIN
 *
 * @note For simplicity and minimum divergence between platforms there is no direct mapping between clarinet socket
 * options and platform specific flags such as @c SO_REUSEADDR, @c SO_REUSEPORT, @c SO_EXCLUSIVEADDRUSE and
 * @c SO_EXCLBIND. Instead, @c CLARINET_SO_REUSEADDR values 1 and 0 translates to different combinations of platform
 * dependent socket options as shown in the following table:
 *
 * @note @verbatim
 *
 *     +--------------+-----------------------+---+---+
 *     | Clarinet     | CLARINET_SO_REUSEADDR | 1 | 0 |
 *     +--------------+-----------------------+---+---+
 *     | Linux (old)  | SO_REUSEADDR          | 1 | 0 |
 *     +--------------+-----------------------+---+---+
 *     | Linux >= 3.9 | SO_REUSEADDR          | 1 | 0 |
 *     |              | SO_REUSEPORT          | 1 | 0 |
 *     +--------------+-----------------------+---+---+
 *     | BSD/Darwin   | SO_REUSEADDR          | 1 | 0 |
 *     |              | SO_REUSEPORT_LB       | 1 | 0 |
 *     +--------------+-----------------------+---+---+
 *     | Windows      | SO_REUSEADDR          | 1 | 0 |
 *     |              | SO_EXCLUSIVEADDRUSE   | 0 | 1 |
 *     +--------------+-----------------------+---+---+
 *     | Solaris      | SO_REUSEADDR          | 1 | 0 |
 *     |              | SO_EXCLBIND           | 0 | 1 |
 *     +--------------+-----------------------+---+---+
 *
 * @endverbatim
 *
 * @note This way all expected results are supported by the majority of the platforms and the complete behaviour of
 * @c CLARINET_SO_REUSEADDR regarding two sockets bound using @c clarinet_socket_bind() can be completely defined as
 * follows:
 *
 * @note @verbatim
 *
 * +------------------------+-------------------+---------------------+------------+
 * |  # | First Socket      | Second Socket     | Result              | Platform   |
 * |----+-------------------+-------------------+---------------------+------------|
 * |    | IPADDR | V6O | RA | IPADDR | V6O | RA |                     |            |
 * |----+--------+-----+----+--------+-----+----+---------------------+------------|
 * |  0 | ipv4 W |  1  |  0 | ipv4 W |     |    | CLARINET_EADDRINUSE | All        |
 * |  1 | ipv4 W |  1  |  0 | ipv4 S |     |    | CLARINET_EADDRINUSE | All        |
 * |  2 | ipv4 S |  1  |  0 | ipv4 W |     |    | CLARINET_EADDRINUSE | All        |
 * |  3 | ipv4 S |  1  |  0 | ipv4 S |     |    | CLARINET_EADDRINUSE | All        |
 * |  4 | ipv4 W |  1  |  0 | ipv4 W |     |  1 | CLARINET_EADDRINUSE | All        |
 * |  5 | ipv4 W |  1  |  0 | ipv4 S |     |  1 | CLARINET_ENONE      | BSD/Darwin |
 * |    |        |     |    |        |     |    | CLARINET_EADDRINUSE | Others     |
 * |  6 | ipv4 S |  1  |  0 | ipv4 W |     |  1 | CLARINET_EADDRINUSE | Linux      |
 * |    |        |     |    |        |     |    | CLARINET_ENONE      | Others     |
 * |  7 | ipv4 S |  1  |  0 | ipv4 S |     |  1 | CLARINET_EADDRINUSE | All        |
 * |  8 | ipv4 W |  1  |  1 | ipv4 W |     |    | CLARINET_EADDRINUSE | All        |
 * |  9 | ipv4 W |  1  |  1 | ipv4 S |     |    | CLARINET_ENONE      | Windows    |
 * |    |        |     |    |        |     |    | CLARINET_EADDRINUSE | Others     |
 * | 10 | ipv4 S |  1  |  1 | ipv4 W |     |    | CLARINET_EADDRINUSE | All        |
 * | 11 | ipv4 S |  1  |  1 | ipv4 S |     |    | CLARINET_EADDRINUSE | All        |
 * | 12 | ipv4 W |  1  |  1 | ipv4 W |     |  1 | CLARINET_ENONE      | All        |
 * | 13 | ipv4 W |  1  |  1 | ipv4 S |     |  1 | CLARINET_ENONE      | All        |
 * | 14 | ipv4 S |  1  |  1 | ipv4 W |  1  |  1 | CLARINET_ENONE      | All        |
 * | 15 | ipv4 S |  1  |  1 | ipv4 S |  1  |  1 | CLARINET_ENONE      | All        |
 * | 16 | ipv6 W |  1  |  0 | ipv6 W |  1  |  0 | CLARINET_EADDRINUSE | All        |
 * | 17 | ipv6 W |  1  |  0 | ipv6 S |  1  |  0 | CLARINET_EADDRINUSE | All        |
 * | 18 | ipv6 S |  1  |  0 | ipv6 W |  1  |  0 | CLARINET_EADDRINUSE | All        |
 * | 19 | ipv6 S |  1  |  0 | ipv6 S |  1  |  0 | CLARINET_EADDRINUSE | All        |
 * | 20 | ipv6 W |  1  |  0 | ipv6 W |  1  |  1 | CLARINET_EADDRINUSE | All        |
 * | 21 | ipv6 W |  1  |  0 | ipv6 S |  1  |  1 | CLARINET_ENONE      | BSD/Darwin |
 * |    |        |     |    |        |     |    | CLARINET_EADDRINUSE | Others     |
 * | 22 | ipv6 S |  1  |  0 | ipv6 W |  1  |  1 | CLARINET_EADDRINUSE | Linux      |
 * |    |        |     |    |        |     |    | CLARINET_ENONE      | Others     |
 * | 23 | ipv6 S |  1  |  0 | ipv6 S |  1  |  1 | CLARINET_EADDRINUSE | All        |
 * | 24 | ipv6 W |  1  |  1 | ipv6 W |  1  |  0 | CLARINET_EADDRINUSE | All        |
 * | 25 | ipv6 W |  1  |  1 | ipv6 S |  1  |  0 | CLARINET_ENONE      | Windows    |
 * |    |        |     |    |        |     |    | CLARINET_EADDRINUSE | Others     |
 * | 26 | ipv6 S |  1  |  1 | ipv6 W |  1  |  0 | CLARINET_EADDRINUSE | All        |
 * | 27 | ipv6 S |  1  |  1 | ipv6 S |  1  |  0 | CLARINET_EADDRINUSE | All        |
 * | 28 | ipv6 W |  1  |  1 | ipv6 W |  1  |  1 | CLARINET_ENONE      | All        |
 * | 29 | ipv6 W |  1  |  1 | ipv6 S |  1  |  1 | CLARINET_ENONE      | All        |
 * | 30 | ipv6 S |  1  |  1 | ipv6 W |  1  |  1 | CLARINET_ENONE      | All        |
 * | 31 | ipv6 S |  1  |  1 | ipv6 S |  1  |  1 | CLARINET_ENONE      | All        |
 * | 32 | ipv6 W |  1  |  0 | ipv4 W |  1  |  0 | CLARINET_ENONE      | All        |
 * | 33 | ipv6 W |  1  |  0 | ipv4 S |  1  |  0 | CLARINET_ENONE      | All        |
 * | 34 | ipv6 S |  1  |  0 | ipv4 W |  1  |  0 | CLARINET_ENONE      | All        |
 * | 35 | ipv6 S |  1  |  0 | ipv4 S |  1  |  0 | CLARINET_ENONE      | All        |
 * | 36 | ipv6 W |  1  |  0 | ipv4 W |  1  |  1 | CLARINET_ENONE      | All        |
 * | 37 | ipv6 W |  1  |  0 | ipv4 S |  1  |  1 | CLARINET_ENONE      | All        |
 * | 38 | ipv6 S |  1  |  0 | ipv4 W |  1  |  1 | CLARINET_ENONE      | All        |
 * | 39 | ipv6 S |  1  |  0 | ipv4 S |  1  |  1 | CLARINET_ENONE      | All        |
 * | 40 | ipv6 W |  1  |  1 | ipv4 W |  1  |  0 | CLARINET_ENONE      | All        |
 * | 41 | ipv6 W |  1  |  1 | ipv4 S |  1  |  0 | CLARINET_ENONE      | All        |
 * | 42 | ipv6 S |  1  |  1 | ipv4 W |  1  |  0 | CLARINET_ENONE      | All        |
 * | 43 | ipv6 S |  1  |  1 | ipv4 S |  1  |  0 | CLARINET_ENONE      | All        |
 * | 44 | ipv6 W |  1  |  1 | ipv4 W |  1  |  1 | CLARINET_ENONE      | All        |
 * | 45 | ipv6 W |  1  |  1 | ipv4 S |  1  |  1 | CLARINET_ENONE      | All        |
 * | 46 | ipv6 S |  1  |  1 | ipv4 W |  1  |  1 | CLARINET_ENONE      | All        |
 * | 47 | ipv6 S |  1  |  1 | ipv4 S |  1  |  1 | CLARINET_ENONE      | All        |
 * | 48 | ipv4 W |  1  |  0 | ipv6 W |  1  |  0 | CLARINET_ENONE      | All        |
 * | 49 | ipv4 W |  1  |  0 | ipv6 S |  1  |  0 | CLARINET_ENONE      | All        |
 * | 40 | ipv4 S |  1  |  0 | ipv6 W |  1  |  0 | CLARINET_ENONE      | All        |
 * | 51 | ipv4 S |  1  |  0 | ipv6 S |  1  |  0 | CLARINET_ENONE      | All        |
 * | 52 | ipv4 W |  1  |  0 | ipv6 W |  1  |  1 | CLARINET_ENONE      | All        |
 * | 53 | ipv4 W |  1  |  0 | ipv6 S |  1  |  1 | CLARINET_ENONE      | All        |
 * | 54 | ipv4 S |  1  |  0 | ipv6 W |  1  |  1 | CLARINET_ENONE      | All        |
 * | 55 | ipv4 S |  1  |  0 | ipv6 S |  1  |  1 | CLARINET_ENONE      | All        |
 * | 56 | ipv4 W |  1  |  1 | ipv6 W |  1  |  0 | CLARINET_ENONE      | All        |
 * | 57 | ipv4 W |  1  |  1 | ipv6 S |  1  |  0 | CLARINET_ENONE      | All        |
 * | 58 | ipv4 S |  1  |  1 | ipv6 W |  1  |  0 | CLARINET_ENONE      | All        |
 * | 59 | ipv4 S |  1  |  1 | ipv6 S |  1  |  0 | CLARINET_ENONE      | All        |
 * | 60 | ipv4 W |  1  |  1 | ipv6 W |  1  |  1 | CLARINET_ENONE      | All        |
 * | 61 | ipv4 W |  1  |  1 | ipv6 S |  1  |  1 | CLARINET_ENONE      | All        |
 * | 62 | ipv4 S |  1  |  1 | ipv6 W |  1  |  1 | CLARINET_ENONE      | All        |
 * | 63 | ipv4 S |  1  |  1 | ipv6 S |  1  |  1 | CLARINET_ENONE      | All        |
 * | 64 | ipv6 W |  0  |  0 | ipv4 W |  1  |  0 | CLARINET_EADDRINUSE | All        |
 * | 65 | ipv6 W |  0  |  0 | ipv4 S |  1  |  0 | CLARINET_EADDRINUSE | All        |
 * | 66 | ipv6 S |  0  |  0 | ipv4 W |  1  |  0 | CLARINET_ENONE      | All        |
 * | 67 | ipv6 S |  0  |  0 | ipv4 S |  1  |  0 | CLARINET_ENONE      | All        |
 * | 68 | ipv6 W |  0  |  0 | ipv4 W |  1  |  1 | CLARINET_EADDRINUSE | All        |
 * | 69 | ipv6 W |  0  |  0 | ipv4 S |  1  |  1 | CLARINET_ENONE      | BSD/Darwin |
 * |    |        |     |    |        |     |    | CLARINET_EADDRINUSE | Others     |
 * | 70 | ipv6 S |  0  |  0 | ipv4 W |  1  |  1 | CLARINET_ENONE      | All        |
 * | 71 | ipv6 S |  0  |  0 | ipv4 S |  1  |  1 | CLARINET_ENONE      | All        |
 * | 72 | ipv6 W |  0  |  1 | ipv4 W |  1  |  0 | CLARINET_ENONE      | Windows    |
 * |    |        |     |    |        |     |    | CLARINET_EADDRINUSE | Others     |
 * | 73 | ipv6 W |  0  |  1 | ipv4 S |  1  |  0 | CLARINET_ENONE      | Windows    |
 * |    |        |     |    |        |     |    | CLARINET_EADDRINUSE | Others     |
 * | 74 | ipv6 S |  0  |  1 | ipv4 W |  1  |  0 | CLARINET_ENONE      | All        |
 * | 75 | ipv6 S |  0  |  1 | ipv4 S |  1  |  0 | CLARINET_ENONE      | All        |
 * | 76 | ipv6 W |  0  |  1 | ipv4 W |  1  |  1 | CLARINET_ENONE      | All        |
 * | 77 | ipv6 W |  0  |  1 | ipv4 S |  1  |  1 | CLARINET_ENONE      | All        |
 * | 78 | ipv6 S |  0  |  1 | ipv4 W |  1  |  1 | CLARINET_ENONE      | All        |
 * | 79 | ipv6 S |  0  |  1 | ipv4 S |  1  |  1 | CLARINET_ENONE      | All        |
 * | 80 | ipv4 W |  1  |  0 | ipv6 W |  0  |  0 | CLARINET_ENONE      | BSD/Darwin |
 * |    |        |     |    |        |     |    | CLARINET_EADDRINUSE | Others     |
 * | 81 | ipv4 W |  1  |  0 | ipv6 S |  0  |  0 | CLARINET_ENONE      | All        |
 * | 82 | ipv4 S |  1  |  0 | ipv6 W |  0  |  0 | CLARINET_EADDRINUSE | All        |
 * | 83 | ipv4 S |  1  |  0 | ipv6 S |  0  |  0 | CLARINET_ENONE      | All        |
 * | 84 | ipv4 W |  1  |  0 | ipv6 W |  0  |  1 | CLARINET_EADDRINUSE | Linux      |
 * |    |        |     |    |        |     |    | CLARINET_ENONE      | Others     |
 * | 85 | ipv4 W |  1  |  0 | ipv6 S |  0  |  1 | CLARINET_ENONE      | All        |
 * | 86 | ipv4 S |  1  |  0 | ipv6 W |  0  |  1 | CLARINET_EADDRINUSE | Linux      |
 * |    |        |     |    |        |     |    | CLARINET_ENONE      | Others     |
 * | 87 | ipv4 S |  1  |  0 | ipv6 S |  0  |  1 | CLARINET_ENONE      | All        |
 * | 88 | ipv4 W |  1  |  1 | ipv6 W |  0  |  0 | CLARINET_ENONE      | BSD/Darwin |
 * |    |        |     |    |        |     |    | CLARINET_EADDRINUSE | Others     |
 * | 89 | ipv4 W |  1  |  1 | ipv6 S |  0  |  0 | CLARINET_ENONE      | All        |
 * | 90 | ipv4 S |  1  |  1 | ipv6 W |  0  |  0 | CLARINET_EADDRINUSE | All        |
 * | 91 | ipv4 S |  1  |  1 | ipv6 S |  0  |  0 | CLARINET_ENONE      | All        |
 * | 92 | ipv4 W |  1  |  1 | ipv6 W |  0  |  1 | CLARINET_ENONE      | All        |
 * | 93 | ipv4 W |  1  |  1 | ipv6 S |  0  |  1 | CLARINET_ENONE      | All        |
 * | 94 | ipv4 S |  1  |  1 | ipv6 W |  0  |  1 | CLARINET_ENONE      | All        |
 * | 95 | ipv4 S |  1  |  1 | ipv6 S |  0  |  1 | CLARINET_ENONE      | All        |
 * +----+--------+-----+----+--------+-----+----+---------------------+------------+
 *   V6O = CLARINET_IP_V6ONLY; RA = CLARINET_SO_REUSEADDR;
 *   W = wildcard address; S = specific unicast address
 *
 * @endverbatim
 */
#define CLARINET_SO_REUSEADDR       2

/**
 * Socket buffer size for output. @a optval is @c int32_t. Valid values are limited to the range [1, INT_MAX].
 * Behaviour is undefined for negative values. A value of zero (0) may yield different results depending on the platform
 * but these should be well defined (see notes).
 *
 * @details The options @c CLARINET_SO_SNDBUF and @c CLARINET_SO_RCVBUF correspond to the underlying @c SO_SNDBUF and
 * @c SO_RCVBUF socket options respectively. In the case of TCP sockets these values may affect the host's send and
 * receive windows. In the case of UDP, these values may affect (yet not determine) the maximum datagram that can be
 * transmitted (or received) and the maximum size of send/recv bursts. Buffer sizes are of particular significance for
 * UDP sockets communicating with multiple remote hosts. While TCP sockets have a separate pair of buffers allocated for
 * each connection, a UDP server socket must share the same pair when communicating with multiple remote hosts.
 *
 * @note In a preemptive operating system like Windows, Linux and macOS, a UDP socket may start transitting a message
 * even before the @c send(2) syscall returns control to the application. The application is interrupted by the
 * operating system so the message can be copied from application memory into protected kernel memory but contrary to
 * the normal assumption, the kernel does not necessarily copy the message onto the socket send buffer to return
 * immediately. Often the same process will remain scheduled once the kernel completes the syscall, but it's highly
 * probable that one or more I/O interruptions will occur until the application regains control so the kernel may take
 * the opportunity to package and relay data down to the network driver where DMA will allow the device to transmit
 * concurrently. On Linux, for example, "a send operation will attempt to traverse the network stack down to the driver
 * queue within the context of the send() call. However, if there is not enough room in the driver queue for the full
 * output block, then the send() call will separately queue what does not fit and raise a softirq to transmit that
 * queued data later. The send() call will also raise a softirq if the device queue is locked because another process is
 * also sending data to the NIC. When the kernel later processes the softirq is not simple to determine. First, it
 * depends on the kernel version, especially when using the @c CONFIG_PREEMPT_RT patch, and can therefore change as the
 * application is migrated to future kernels. It also depends on other kernel activity that process softirqs. One of
 * those activities is the send() call, so a subsequent send() call, even on a different socket, can process the
 * remaining packets. If the CONFIG_PREEMPT_RT patch models the softirq as a thread, then execution of the softirq may
 * also depend on the scheduling priority of the application, and it may be possible for the application to block the
 * softirq for extended periods. Otherwise, the softirq may also interrupt the application at an inopportune time.
 * Generally, conditions for softirq processing make it unusual for a softirq to remain pending for very long. However,
 * long delays can occur under heavy loads.
 * (...)
 * How much data can be sent in a single call or in the time prior to a synchronous data call without raising a softirq?
 * [i.e. without using the send buffer] It depends on the length of the driver queue, the path MTUs, and the features
 * and speed of the NIC. Based on the kernel code, Linux uses a driver queue with 64 entries. Some NIC features like
 * segmentation offload (...) allow the entry to represent more than one packet. However, a simple calculation assumes
 * that each entry represents a packet whose length is equal to the path MTU and that the NIC empties the driver queue
 * at the rate that it can transmit the data on the wire (accounting for the full length of the resulting Ethernet
 * frame). For example, with a 100 Mbit Ethernet connection and a 1500 byte MTU (a 1542 byte Ethernet frame), a send
 * call should avoid raising a softirq if it does not transmit a block larger than 92,762 bytes through a TCP socket
 * (64 packets with 1448 user bytes per packet). In another example, if the synchronous send block is 34752 bytes
 * (24 packets), then the application should not send more than 57920 bytes (40 packets) in the prior 5 milliseconds.".
 * [<a href="https://ntrs.nasa.gov/api/citations/20200002393/downloads/20200002393.pdf">source</a>].
 *
 * @note In this context, buffer size estimation can be quite difficult not only because of kernel optimizations but
 * also because some systems rely on the socket buffers to limit overall memory usage. This means protocol headers and
 * system book keeping structures take up buffer space as well. Accounting for protocol and system overhead avoids two
 * common pitfalls. First, it would be incorrect to only count the packet size disconsidering the system overhead
 * because it is possible for a system to allocate a large data structure and receive much smaller packets in
 * comparison. Likewise, it would be incorrect to only count the packet payload and not the headers because protocol
 * overhead can be significant and consume large amounts of system memory. Consider the case of packets with 1-byte
 * payloads over ipv6 where the protocol headers can be 48 bytes for UDP and 68 for TCP.
 *
 * @note @b LINUX: Clamps the option value between the minimum and maximum configured by the system so setting a value
 * of 0 effectively assumes the minimum. The default socket buffer sizes are calculated to hold 256 packets of 256-bytes
 * each plus overhead. On a 64-bit system the overhead is 576 bytes per packet so 256 * (256 + 576) = 212992.
 * [<a href="https://elixir.bootlin.com/linux/v4.5/source/net/core/sock.c#L265">source</a>].
 *
 * @note Note that the <a href="https://man7.org/linux/man-pages/man7/unix.7.html">unix(7) man page</a> is not accurate
 * when it states that "for datagram sockets, the SO_SNDBUF value imposes an upper limit on the size of outgoing
 * datagrams. This limit is calculated as the doubled (see socket(7)) option value less 32 bytes used for overhead." The
 * part about doubling is correct but all the rest is not. Also according to the socket(7) man page, the send buffer has
 * a hard minimum value of 2048 and the recv buffer a minimum of 256 but kernel code shows that the minimum send-buffer
 * size is 4096+480 bytes on x64 (4096+384 on x86) and the minimum recv buffer is 2048+244 on x64 (2048+192 on x86).
 * Default and maximum values can be verified and adjusted by the following sysctl settings:
 *
 * @note @verbatim
 *
 *   net.core.rmem_default=212992
 *   net.core.wmem_default=212992
 *   net.core.rmem_max=212992
 *   net.core.wmem_max=212992
 *   net.core.netdev_max_backlog=1000
 *
 * @endverbatim
 *
 * @note The following system settings are calculated at boot according to total system memory and apply to both ipv4
 * and ipv6 (despite the name) [<a href="https://man7.org/linux/man-pages/man7/udp.7.html">source</a>]:
 *
 * @note @verbatim
 *
 *   net.ipv4.udp_mem.min
 *   net.ipv4.udp_mem.pressure
 *   net.ipv4.udp_mem.max
 *
 * @endverbatim
 *
 * @note The large memory overhead per packet is one of the reasons why Linux doubles the values passed to
 * @c setsockopt(2) for @c SO_SNDBUF/SO_RCVBUF but the actual buffer space consumed can be in fact much larger. This is
 * because Linux does not reserve the whole buffer memory for each socket. Instead, memory is allocated and released as
 * packets are created and disposed by the network pipeline with each socket keeping track of its alloted memory in
 * order to remain within the buffer limits assigned. In order to prevent excessive heap fragmentation the Linux kernel
 * employs slab allocations which can consume considerably more memory per packet but is more efficient in terms of CPU
 * and avoids out-of-memory errros due to fragmentation.
 * [<a href="https://indico.dns-oarc.net/event/25/contributions/412/attachments/368/633/udp_buffer_tuning.pdf">source</a>]
 *
 * @note Real buffer occupation can be verified by inspecting /proc/net/udp and on Ubuntu 20.04 x64 buffer sizes relate
 * to payload sizes as follows:
 *
 * @note @verbatim
 *
 *     | Buffer Size   | IPv4 Payload Size | IPv6 Payload Size |
 *     |---------------+-------------------+-------------------|
 *     | 768           | 1-69              | 1-56              |
 *     | 1280          | 70-581            | 57-568            |
 *     | 2304          | 582-1605          | 569-1593          |
 *     | 4352          | 1606-3653         | 1594-3640         |
 *     | 8448          | 3654-3999         | 3641-3999         |
 *     +---------------+-------------------+-------------------+
 *
 * @endverbatim
 *
 * @note For consistency with other platforms, @c CLARINET_SO_SNDBUF and @c CLARINET_SO_RCVBUF are halfed on LINUX
 * before calling @c setsockopt(2) so actual values will be closer to what an unsuspecting user would expect like on
 * WINDOWS and BSD/Darwin (although even numbers are exact and odd numbers are off by 1). Since LINUX reports actual
 * buffer sizes (doubled) no change is required after getsockopt.
 *
 * @note @b BSD/DARWIN: Zero (0) is not a valid value. The minimum valid value is 1. In particular, the send buffer size
 * of a UDP socket does not limit memory allocations and serves only to limit the size of the message that can be passed
 * to @c send(2), therefore there is no point in setting SO_SNDBUF to greater than 65535. This stems from the fact that
 * on FreeBSD and Darwin "a send() on a UDP socket processes right down to the if_output [NIC]. If that fails because
 * the @a ifqueue is full, the packet will be free()d right away [dropped]." And in fact, contrary to the documentation,
 * @c send(2) cannot even block on regular UDP sockets since blocking can only occur "on the socket buffer filling up,
 * not on the interface queue. Because UDP has no output socket buffer, there is no way it can block." Incidentally,
 * @c send(2) returns @c ENOBUFS in this case.
 * [<a href="https://lists.freebsd.org/pipermail/freebsd-hackers/2004-January/005377.html"><source</a>]
 *
 * @note This also explains why there is a sysctl(3) MIB variable for the default UDP recv buffer size
 * (net.inet.udp.recvspace) but not a send buffer size - one is only defined for TCP. The recv buffer size overhead, if
 * it exists, remains to be determined. Default and maximum values are affected by the following sysctl variables:
 *
 * @note @verbatim
 *
 *     # maximum datagram that can be transmitted
 *     # (does not apply to the loopback interface)
 *     net.inet.udp.maxdgram=9216
 *
 *     # default udp socket SO_RCVBUF
 *     net.inet.udp.recvspace=42080
 *
 *     # maximum maxdgram datagram that can be transmitted
 *     # on a loopback interface
 *     net.local.dgram.maxdgram=2048
 *
 *     # default udp socket SO_RCVBUF
 *     # on the loopback interface
 *     net.local.dgram.recvspace=4096
 *
 *     # absolute maximum size for a socket buf, also used
 *     # to define the window scaling factor for TCP
 *     kern.ipc.maxsockbuf=2097152
 *
 *
 *     # governs the total amount of memory available to all
 *     # sockets opened on the system. This number tells how
 *     # many number of mbuf clusters should be allocated.
 *     # Usually each cluster has 2k bytes. For example , if
 *     # you are planning to open 1000 sockets with each having
 *     # 8k sending and 8k size recv buffer each socket will need
 *     # 16k of memory and in total you will need 16M = 16k x 1000
 *     # of memory to handle all 1000 connections.
 *     kern.ipc.nmbclusters=65536
 *
 * @endverbatim
 *
 * @note Note that FeeeBSD (and possibly Darwin) adjusts the value from kern.ipc.maxsockbuf as follows
 * [<a href="https://github.com/freebsd/freebsd-src/blob/de1aa3dab23c06fec962a14da3e7b4755c5880cf/sys/kern/uipc_sockbuf.c#L599">source</a>]:
 *
 * @note @code
 *
 *     sb_max_adj = (u_quad_t)sb_max * MCLBYTES / (MSIZE + MCLBYTES);
 *
 * @endcode
 *
 * @note So a value of 8388608 becomes 8388608 * (1 << 11) / (256 + (1 << 11)) = 8388608 * (2048) / (2304) = 7456540
 *
 * @note WINDOWS: Zero (0) is a valid setting and effectively disables the socket buffer. The default buffer size is
 * 8219 on UDP sockets for both send and recv. Empirical evidence suggests there is no overhead per packet but the last
 * byte of the buffer cannot be used so a message will only enter the buffer if its size is less than the space availble
 * (not less than or equal). For example, a message with 2048 bytes requires a buffer of at least 2048+1 bytes. Two
 * messages of 2048 bytes require a buffer size of at least 2048*2 + 1.
 *
 * @note Supposedly, Winsock employs the strategy of reaching directly to the NIC when possible. The consequence is that
 * sometimes the system will appear to be able to transmit or receive messages that are larger than the socket buffer
 * size of a UDP socket. Also send and receive operations using the loopback interface will appear much faster and
 * potentially produce significant less overhead on the buffers (or even completely bypass the send-buffer) because the
 * MTU in this case is usually large (often 65535) and tx bandwidth is orders of magnitude higher than on a regular
 * network interface.
 *
 * @note Note that in real life one would rarely specify buffer sizes to accomodate a single message or only messages of
 * the same size so care must be taken to account for overhead.  In general, buffer size estimation must take into
 * account message overhead, send/recv rates, link bandwidth, maximum expected size of data bursts, minimum time between
 * sends and maximum time between receives. A safe rule of thumb for platform independent estimates is to assume an
 * overhead per message following the allocation table described for Linux since it appears to have the largest
 * overheads compared to Windows and BSD/Darwin. All three platforms discard outbound packets that cannot be rounted or
 * that are blocked by a firewall rule before any buffering so from the perspective of the user program such packets
 * never occupy space in memory.
 *
 * @note Also contrary to popular belief socket buffers are not an effective way of limiting the datagram size the user
 * application can send or receive because of the way the kernel can completely bypass the buffers.
 *
 * @note TODO: explain how @c CLARINET_SO_SNDBUF and CLARINET_SO_RCVBUF affect TCP sockets in each platform.
 */
#define CLARINET_SO_SNDBUF          3

/**
 * Socket buffer size for input.  @a optval is @c int32_t. Valid values are limited to [1, INT_MAX].
 *
 * @details See @c CLARINET_SO_SNDBUF for a complete description and notes.
 */
#define CLARINET_SO_RCVBUF          4

/**
 * Socket send timeout in milliseconds. @a optval is @c int32_t. Valid values are limited to [1, INT_MAX].
 */
#define CLARINET_SO_SNDTIMEO        5

/**
 * Socket receive timeout in milliseconds. @a optval is @c int32_t. Valid values are limited to [1, INT_MAX].
 */
#define CLARINET_SO_RCVTIMEO        6

/**
 * Enable/disable keepalive. @a optval is @c uint32_t. Valid values are limited to 0 (false) and non-zero (true). Only
 * supported by TCP sockets.
 */
#define CLARINET_SO_KEEPALIVE       7

/**
 * Socket linger timeout. @a optval is @c clarinet_linger. Only supported by TCP sockets.
 */
#define CLARINET_SO_LINGER          8

/**
 * Enable/disable linger without affecting the timeout already configured. @a optval is @c uint32_t. Valid values are
 * limited to 0 (false) and non-zero (true). Only supported by TCP sockets.
 */
#define CLARINET_SO_DONTLINGER      9

/**
 * Error status of the socket. @a optval is @c int32_t. The value is reset after being retrieved. This option is
 * read-only.
 *
 * @details Only supported in @c clarinet_socket_getopt().
 *
 * @note This option has very limited use. It's only guaranteed to reset after being fetched by
 * @c clarinet_socket_getopt() but some platforms may reset it in other situations too.
 *
 * @note @b LINUX: This option is reset upon a call to @c clarinet_socket_recv() or @c clarinet_socket_recvfrom()
 * because the internal socket error, if defined, is already returned by those calls. CLARINET_SO_ERROR should
 * only be fecthed after an asynchronous since in this case  there is no way to determine the outcome of the operation.
 * Currently SO_ERROR has only one relevant use nowadays that is to determine the result of a
 * call to connect(2) with a non-blocking TCP socket. See @clarinet_socket_connect() for more information.

 */
#define CLARINET_SO_ERROR           10

/**
 * Enable/Disable Dual Stack on an IPV6 socket. @a optval is @c uint32_t. Valid values are limited to 0 (false) and
 * non-zero (true). Only supported by IPv6 sockets.
 *
 * @details If this options is set to 1, then the socket is restricted to sending and receiving IPv6 packets only.  In
 * this case, an IPv4 and an IPv6 application can bind to a single port at the same time. If this flag is set to false
 * (zero), then the socket can be used to send and receive packets to and from an IPv6 address or an IPv4 mapped to IPv6
 * address.
 *
 * @note The value of this option in combination with @c CLARINET_SO_REUSEADDR may affect the outcome of @c
 * clarinet_socket_bind(). See @c CLARINET_SO_REUSEADDR for more information.
 *
 * @note @b LINUX:  The default value for this option is defined by the contents of the file
 * @c /proc/sys/net/ipv6/bindv6only. The default value for that file is 0 (false).
 *
 * @note @b WINDOWS: The default value for this option is 1 (true) and there is no documented system setting associated.
 *
 * @note @b BSD/DARWIN: The default value for this option is defined by the value of sysctl(3) MIB variable
 * @c net.inet6.ip6.v6only. The default value for that variable is 0 (false).
 */
#define CLARINET_IP_V6ONLY          100

/**
 * Time-To-Live for IPv4 and (Hop Limit for) IPv6 for outgoing @b unicast packets. @a optval is @c int32_t. Valid values
 * are limited to the range [1, 255].
 *
 * @details This is the value used in the IP header when sending unicast packets. This option is considered a hint to
 * the system and not a strict requirement. Underlying IP stacks may ignore this option without returning an error.
 *
 * @note @b WINDOWS: Default value is defined by
 * @c KEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\DefaultTTL. Winsock implementations are
 * not required to return an error if the corresponding underlying socket option is not supported. Users are supposed to
 * check if the value was effectively set by calling @c clarinet_socket_getopt() which usually will return an error if
 * the option is not supported by the platform. In any case, all Windows versions since Windows 95 support this option
 * for IPv4 and all since Windows Vista, for IPv6.
 */
#define CLARINET_IP_TTL             101

/**
 * Current known path MTU of the socket. @a optval is @c int32_t. This option is read-only.
 */
#define CLARINET_IP_MTU             102

/**
 * Enable/disable path MTU discovery mode. @a optval is <tt>enum clarinet_pmtud</tt>. This option is read-only.
 */
#define CLARINET_IP_MTU_DISCOVER    103

/**
 * Enable/disable the ability to send and receive broadcast packets. @a optval is @c uint32_t. Valid values are
 * limited to 0 (false) and non-zero (true). Only supported by UDP sockets.
 */
#define CLARINET_IP_BROADCAST       104

/**
 * Time-To-Live for IPv4 and (Hop Limit for) IPv6 for outgoing @b multicast packets. @a optval is @c uint32_t. Valid
 * values are limited to the range [1, 255]. Only supported by UDP sockets.
 *
 * @details Multicast packets with a TTL of 1 are not forwarded beyond the local network. Those with a TTL of 0 will not
 * be transmitted on any network, but may be delivered locally if the sending host belongs to the destination group and
 * if multicast loopback has not been disabled on the sending socket (see @ CLARINET_IP_MCAST_LOOP). Multicast packets
 * with TTL greater than 1 may be forwarded to other networks if a multicast router is attached to the local network.
 */
#define CLARINET_IP_MCAST_TTL       105

/**
 * Enable/disable whether data sent by an application on the local computer (not necessarily by the same socket) in a
 * multicast session will be received by a socket joined to the multicast destination group on the loopback interface.
 * @a optval is @c uint32_t. Valid values are limited to the range [1, 255]. Only supported by UDP sockets.
 *
 * @details A value of true causes multicast data sent by an application on the local computer to be delivered to a
 * listening socket on the loopback interface. A value of false prevents multicast data sent by an application on the
 * local computer from being delivered to a listening socket on the loopback interface.
 *
 * @note @b WINDOWS: Default value is true.
 *
 * @note @b DARWIN/BSD: The sysctl setting @c net.inet.ip.mcast.loop controls the default setting of this socket option
 * for new sockets.
 */
#define CLARINET_IP_MCAST_LOOP      106

/**
 * Add the socket to the supplied multicast group on the specified interface. @a optval is @c clarinet_mcast_group.
 * This option is write-only. Only supported by UDP sockets.
 */
#define CLARINET_IP_MCAST_JOIN      107

/**
 * Remove the socket from the supplied multicast group on the specified interface. @a optval is @c clarinet_mcast_group.
 * This option is write-only. Only supported by UDP sockets.
 */
#define CLARINET_IP_MCAST_LEAVE     108

/* endregion */

/* region Socket Shutdown Flags */

#define CLARINET_SHUTDOWN_NONE      0x00
#define CLARINET_SHUTDOWN_RECV      0x01                                                /**< Shutdown Receive */
#define CLARINET_SHUTDOWN_SEND      0x02                                                /**< Shutdown Send */
#define CLARINET_SHUTDOWN_BOTH      (CLARINET_SHUTDOWN_RECV|CLARINET_SHUTDOWN_SEND)     /**< Shutdown Both */

/* endregion */

/* region Socket Event Flags */

/** None */
#define CLARINET_POLL_NONE          0x00

/** Invalid socket. */
#define CLARINET_POLL_INVALID       0x01

/** Socket reported an error. */
#define CLARINET_POLL_ERROR         0x02

/**
 * Socket was shutdown by the remote host.
 *
 * @details Only returned by TCP sockets in @c revents; ignored in @c events. This status flag merely indicates that the
 * remote host closed its end of the connection. Subsequent reads from the connection will return 0 (end of file) only
 * after all outstanding data in has been consumed.
 */
#define CLARINET_POLL_SHUTDOWN      0x04

/** Socket is ready to receive data without blocking. */
#define CLARINET_POLL_RECV          0x08

/** Socket is ready to send data without blocking. */
#define CLARINET_POLL_SEND          0x10

/* endregion */

/* region MTU Discovery Modes */

#define CLARINET_PMTUD_MODES(E) \
    E(CLARINET_PMTUD_UNSPEC,      0, "PMTUD Unspecified") \
    E(CLARINET_PMTUD_ON,          1, "PMTUD Enabled") \
    E(CLARINET_PMTUD_OFF,         2, "PMTUD Disabled") \
    E(CLARINET_PMTUD_PROBE,       3, "PMTUD Probe") \

/** Path MTU Discovery Modes.  */
enum clarinet_pmtud
{
    CLARINET_PMTUD_MODES(CLARINET_DECLARE_ENUM_ITEM)
};

/**
 * @var clarinet_pmtud::CLARINET_PMTUD_UNSPEC
 * @details ??
 */

/**
 * @var clarinet_pmtud::CLARINET_PMTUD_ON
 * @details Always do Path MTU Discovery. Socket will set DF=1 and fail to send datagrams larger than IP_MTU.
 */

/**
 * @var clarinet_pmtud::CLARINET_PMTUD_OFF
 * @details Socket will set DF=0 and fragment datagrams larger than the interfce MTU, except on Linux <= 3.15 where the
 * fragmentation does not occur and the send operation will fail instead (see notes).
 *
 * @note @b LINUX: The underlying system flag @c IP_PMTUDISC_DONT will handle ICMP Type 3 Code 4 packets even though
 * datagrams are only sent with DF=0 (WTF!?). So in kernel 3.13 a new flag was introduced, @c IP_PMTUDISC_INTERFACE, to
 * ignore the path MTU estimate and always use the interface MTU instead. ICMP packets, which are clearly spoofed in
 * this case, are simply ignored and datagrams are always sent with DF=0. However @c send(2) will fail if the user data
 * buffer produces a datagrams larger than the interface MTU (WTF again!?). So in kernel 3.15 another flag was
 * introduced, @c IP_PMTUDISC_OMIT, which behaves exactly like @c IP_PMTUDISC_INTERFACE but can fragment datagrams
 * larger than the interface MTU.
 *
 * @note @b WINDOWS: This flag has the expected semantics that is packets are sent with DF=0 and data buffers will be
 * fragmented if larger then the interface MTU.
 *
 * @note @b BSD/DARWIN: This flag has the expected semantics that is packets are sent with DF=0 and data buffers will be
 * fragmented if larger then the interface MTU.
 */

/**
 * @var clarinet_pmtud::CLARINET_PMTUD_PROBE
 * @details Socket will set DF=1 and send data unfragmented even if it is larger than IP_MTU.
 */

/* endregion */

/* region String Constants */

/**
 * Maximum string length required to format an address. The longest possible string representation is that of an
 * IPv4MappedToIPv6 address with the largest scope id that can be supported (56+1 for the nul-termination).
 * e.g: 0000:0000:0000:0000:0000:ffff:255.255.255.255%4294967295
 */
#define CLARINET_ADDR_STRLEN        (56+1)

/**
 * Maximum string length required to format an endpoint. The longest possible string representation is that of an
 * IPv4MappedToIPv6 address with the largest scope id that can be supported and the largest port (56+8+1 for the
 * nul-termination). Note that square brackets are used to enclose IPv6 addresss and prevent ambiguity of the ':' sign.
 * e.g: [0000:0000:0000:0000:0000:ffff:255.255.255.255%4294967295]:65535
 */
#define CLARINET_ENDPOINT_STRLEN    (CLARINET_ADDR_STRLEN + 8)

/** Maximum string length required to store a hostname */
#define CLARINET_HOSTNAME_STRLEN    (256+1)

/** Maximum string length required to store an interface name */
#define CLARINET_IFNAME_STRLEN      (256+1)

/* endregion */

/* region Library Info */

CLARINET_EXTERN
uint32_t
clarinet_get_semver(void);

CLARINET_EXTERN
const char*
clarinet_get_version(void);

CLARINET_EXTERN
const char*
clarinet_get_name(void);

CLARINET_EXTERN
const char*
clarinet_get_description(void);

CLARINET_EXTERN
int
clarinet_get_features(void);

/* endregion */

/* region Address */

/**
 *  @file
 *
 * @note Constants defining the number of octets for each address family are defined for internal use only. Deverlopers
 * should not change them expecting that everything will magically work. For example, at least the function-like macros
 * used for address checking will certainly have to be adjusted but functions that manipulate addresses may as well
 * rely on specific values.
 */

#define CLARINET_MAC_OCTETS_SIZE    8
#define CLARINET_IPV4_OCTETS_SIZE   4
#define CLARINET_IPV6_OCTETS_SIZE   16


/** MAC address bytes in network order. */
union clarinet_mac_octets
{
    uint8_t byte[CLARINET_MAC_OCTETS_SIZE];
    uint16_t word[CLARINET_MAC_OCTETS_SIZE / 2];
    uint32_t dword[CLARINET_MAC_OCTETS_SIZE / 4];
};

/** IPv4 address bytes in network order. */
union clarinet_ipv4_octets
{
    uint8_t byte[CLARINET_IPV4_OCTETS_SIZE];
    uint16_t word[CLARINET_IPV4_OCTETS_SIZE / 2];
    uint32_t dword[CLARINET_IPV4_OCTETS_SIZE / 4];
};

/** IPv6 address bytes in network order. */
union clarinet_ipv6_octets
{
    uint8_t byte[CLARINET_IPV6_OCTETS_SIZE];
    uint16_t word[CLARINET_IPV6_OCTETS_SIZE / 2];
    uint32_t dword[CLARINET_IPV6_OCTETS_SIZE / 4];
};

#undef CLARINET_MAC_OCTETS_SIZE
#undef CLARINET_IPV4_OCTETS_SIZE
#undef CLARINET_IPV6_OCTETS_SIZE

/**
 * IPv4 address information (consider using clarinet_addr instead). Padding is used to align an IPv4 address struct with 
 * an IPv6 address struct so one can extract IPv4 information from IPv4MappedToIPv6 addresses without having to know the 
 * details about IPv4MappedToIPv6 format or produce a complete IPv4MappedToIPv6 address.
 */
struct clarinet_ipv4
{
    uint32_t rffu_0 CLARINET_UNUSED;
    uint32_t rffu_1 CLARINET_UNUSED;
    uint32_t rffu_2 CLARINET_UNUSED;
    uint32_t rffu_3 CLARINET_UNUSED;

    /** Address octets in network byte order */
    union clarinet_ipv4_octets u;

    uint32_t rffu_4 CLARINET_UNUSED;
};

/**
 * IPv6 address information (consider using clarinet_addr instead). This is also used to store information of 
 * IPv4MappedToIPv6 addresses.
 */
struct clarinet_ipv6
{
    /** Flow label as specified in <a href="https://datatracker.ietf.org/doc/html/rfc6437">RFC6437</a>. */
    uint32_t flowinfo;

    /** Address octets in network byte order */
    union clarinet_ipv6_octets u;

    /**
     * Used when an address is valid in multiple scopes. IPv6 link-local addresses for example are valid on every IPv6
     * interface, but routing them from one interface to another is not possible. So if you want to communicate with
     * link-local addresses you have to specify which interface to use. The scope-id is the interface index as obtained
     * by @c clarinet_iface_getlist() or @c clarinet_iface_getindex().
     */
    uint32_t scope_id;
};

/**
 * Media Access Control (MAC) address
 *
 * @details The maximum physical address length in all supported platforms is 8. Address bytes are aligned to the right
 * in big-endian notation which means that any non-significant excess bytes will always be zeros to the left.
 * For example, ethernet addresses are represented with <tt>clarinet_mac::length == 6</tt>,
 * <tt>clarinet_mac:u::byte[0] == 0</tt>, <tt>clarinet_mac::u::byte[1] == 0</tt> and then positions 2 to 7
 * in @c clarinet_mac::u::byte[] must contain the ethernet 6-byte address.
 */
struct clarinet_mac
{
    uint32_t rffu_0 CLARINET_UNUSED;
    uint32_t rffu_1 CLARINET_UNUSED;

    /** Address length. Different physical layers may produce MAC addresses of different lengths. Maximum value is
     * <tt>sizeof(union clarinet_mac_octets)</tt>.
     */
    uint32_t length;

    /**
     * Address octets in network byte order.
     *
     * @note Octets are aligned to the right with leading zeros when @c clarinet_mac::length is lower than
     * <tt>sizeof(union clarinet_mac_octets)</tt>.
     */
    union clarinet_mac_octets u;

    uint32_t rffu_3 CLARINET_UNUSED;
};

/**
 * IP address representation.
 *
 * @details This structure can represent both IPv4 and IPv6 addresses. The member 'family' indicates which IP version is
 * represented and may contain any constant value defined in <tt>enum clarinet_family</tt>.
 *
 * @note An IPv4 mapped to IPv6 @b is an IPv6 address that follows a specific format specified in RFC4291.
 * @c clarinet_addr_is_ipv4mapped() can be used to check if an address is an IPv4 mapped to IPv6 address.
 */
struct clarinet_addr
{
    uint16_t family;
    uint16_t rffu CLARINET_UNUSED;
    union clarinet_ip /* this name is just to satisfy C++ compilers that cannot handle unamed unions */
    {
        struct clarinet_ipv6 ipv6;
        struct clarinet_ipv4 ipv4;
        struct clarinet_mac mac;
    } as;
};

typedef struct clarinet_addr clarinet_addr;

CLARINET_EXTERN const clarinet_addr clarinet_addr_none;
CLARINET_EXTERN const clarinet_addr clarinet_addr_any_ipv4;
CLARINET_EXTERN const clarinet_addr clarinet_addr_any_ipv6;
CLARINET_EXTERN const clarinet_addr clarinet_addr_loopback_ipv4;
CLARINET_EXTERN const clarinet_addr clarinet_addr_loopback_ipv6;
CLARINET_EXTERN const clarinet_addr clarinet_addr_loopback_ipv4mapped;
CLARINET_EXTERN const clarinet_addr clarinet_addr_broadcast_ipv4;

/**
 * Multicast group representation.
 *
 * @details This structure is used with either IPv6 or IPv4 multicast addresses and is the data type handled by the
 * @c CLARINET_IP_MCAST_JOIN and @c CLARINET_IP_MCAST_LEAVE socket options.
 *
 * @c clarinet_iface_getlist() can be used to retrieve a list of network interfaces and obtain the interface index
 * information required for the @c clarinet_mcast_group::iface member.
 */
struct clarinet_mcast_group
{
    uint32_t iface;         /**< The index of the local interface on which the multicast group should be joined or dropped. */
    clarinet_addr addr;     /**< The address of the multicast group. This may be either an IPv6 or IPv4 multicast address. */
};

typedef struct clarinet_mcast_group clarinet_mcast_group;

struct clarinet_endpoint
{
    clarinet_addr addr;
    uint16_t port;
    uint16_t rffu CLARINET_UNUSED;
};

typedef struct clarinet_endpoint clarinet_endpoint;

/**
 * Returns the smallest even unsigned integer that is greater than or equal to a base value.
 *
 * @param [in] value: Unsigned integer value used as the base value
 * @return The smallest even unsigned integer that is greater than or equal to @p v.
 */
#define clarinet_xeven(value)    ((((value) + 1) >> 1) << 1)

#define clarinet_addr_is_unspec(addr)             ((addr)->family == CLARINET_AF_UNSPEC)
#define clarinet_addr_is_ipv4(addr)               ((addr)->family == CLARINET_AF_INET)
#define clarinet_addr_is_ipv6(addr)               ((addr)->family == CLARINET_AF_INET6)
#define clarinet_addr_is_mac(addr)                ((addr)->family == CLARINET_AF_LINK)

#define clarinet_addr_is_ipv4mapped(addr) \
  (clarinet_addr_is_ipv6(addr) \
&& (((addr)->as.ipv6.u.word[0] \
   | (addr)->as.ipv6.u.word[1] \
   | (addr)->as.ipv6.u.word[2] \
   | (addr)->as.ipv6.u.word[3] \
   | (addr)->as.ipv6.u.word[4]) == 0) \
&& ((addr)->as.ipv6.u.word[5] == 0xFFFF))

#define clarinet_addr_is_any_ipv4(addr)           (clarinet_addr_is_ipv4(addr) && ((addr)->as.ipv4.u.dword[0] == 0))

#define clarinet_addr_is_any_ipv6(addr) \
  (clarinet_addr_is_ipv6(addr) \
&& (((addr)->as.ipv6.u.dword[0] \
   | (addr)->as.ipv6.u.dword[1] \
   | (addr)->as.ipv6.u.dword[2] \
   | (addr)->as.ipv6.u.dword[3]) == 0) \
&& ((addr)->as.ipv6.scope_id == 0))

#define clarinet_addr_is_loopback_ipv4(addr) \
  (clarinet_addr_is_ipv4(addr) \
&& ((addr)->as.ipv6.u.byte[12] == 127) \
&& ((addr)->as.ipv6.u.byte[15] > 0 && (addr)->as.ipv6.u.byte[15] < 255) \
&& ((addr)->as.ipv6.scope_id == 0))

#define clarinet_addr_is_loopback_ipv6(addr) \
  (clarinet_addr_is_ipv6(addr) \
&& (((addr)->as.ipv6.u.dword[0] \
   | (addr)->as.ipv6.u.dword[1] \
   | (addr)->as.ipv6.u.dword[2]) == 0) \
&& ((addr)->as.ipv6.u.byte[14] == 0) \
&& ((addr)->as.ipv6.u.byte[15] == 1) \
&& ((addr)->as.ipv6.scope_id == 0))

#define clarinet_addr_is_loopback_ipv4mapped(addr) \
  (clarinet_addr_is_ipv6(addr) \
&& (((addr)->as.ipv6.u.word[0] \
   | (addr)->as.ipv6.u.word[1] \
   | (addr)->as.ipv6.u.word[2] \
   | (addr)->as.ipv6.u.word[3] \
   | (addr)->as.ipv6.u.word[4]) == 0) \
&& ((addr)->as.ipv6.u.word[5] == 0xFFFF) \
&& ((addr)->as.ipv6.u.byte[12] == 127) \
&& ((addr)->as.ipv6.u.byte[15] > 0 && (addr)->as.ipv6.u.byte[15] < 255) \
&& ((addr)->as.ipv6.scope_id == 0))

/** Returns true if the address is an IPv4 broadcast address. */
#define clarinet_addr_is_broadcast_ipv4(addr)     (clarinet_addr_is_ipv4(addr) && ((addr)->as.ipv4.u.dword[0] == 0xFFFFFFFF))

#define clarinet_addr_is_multicast_ipv6(addr) \
  (clarinet_addr_is_ipv6(addr) \
&& ((addr)->as.ipv6.u.byte[0] == 0xFF) \
&& ((addr)->as.ipv6.u.byte[1] == 0x00))

#define clarinet_addr_is_linklocal_ipv6(addr) \
  (clarinet_addr_is_ipv6(addr) \
&& ((addr)->as.ipv6.u.byte[0] == 0xFE) \
&& (((addr)->as.ipv6.u.byte[1] & 0xC0) == 0x80))

#define clarinet_addr_is_sitelocal_ipv6(addr) \
  (clarinet_addr_is_ipv6(addr) \
&& ((addr)->as.ipv6.u.byte[0] == 0xFE) \
&& (((addr)->as.ipv6.u.byte[1] & 0xC0) == 0xC0))

#define clarinet_addr_is_teredo_ipv6(addr) \
  (clarinet_addr_is_ipv6(addr) \
&& ((addr)->as.ipv6.u.byte[0] == 0x20) \
&& ((addr)->as.ipv6.u.byte[1] == 0x01) \
&& ((addr)->as.ipv6.u.byte[2] == 0x00) \
&& ((addr)->as.ipv6.u.byte[3] == 0x00))

/**
 * Returns true if the address pointed by addr represents the wildcard address in either IPv4 or IPv6. Note that there
 * is no such thing as a wildcard address in IPv4MappedToIPv6 format because by definition the wildcard address is the
 * zero address.
 */
#define clarinet_addr_is_any_ip(addr) \
 ((clarinet_addr_is_ipv4(addr) || clarinet_addr_is_ipv6(addr)) \
&& (((addr)->as.ipv6.u.dword[0] \
   | (addr)->as.ipv6.u.dword[1] \
   | (addr)->as.ipv6.u.dword[2] \
   | (addr)->as.ipv6.u.dword[3]) == 0) \
&& ((addr)->as.ipv6.scope_id == 0))

/**
 * Returns true if the address pointed by addr represents a loopback address. It can be either an IPv4, IPv6 or an
 * IPv4MappedToIPv6 address. RFC122 reserves the entire 127.0.0.0/8 address block for loopback purposes so anything
 * from 127.0.0.1 to 127.255.255.254 is looped back. RFC4291 just reserves a single IPv6 address, ::1.
 */
#define clarinet_addr_is_loopback_ip(addr) \
  (clarinet_addr_is_loopback_ipv4(addr) \
|| clarinet_addr_is_loopback_ipv6(addr) \
|| clarinet_addr_is_loopback_ipv4mapped(addr))

/**
 * Returns true if the address pointed by addr represents a broadcast address. Always false for IPv6 addresses since
 * broadcasting is not supportted in IPv6 even if using an IPv4MappedToIPv6 address.
 */
#define clarinet_addr_is_broadcast_ip(addr)       clarinet_addr_is_broadcast_ipv4(addr)

/**
 * Returns true if addresses pointed by a and b are equal.
 * If famlily is CLARINET_AF_INET only the last dword is required to be equal.
 * Otherwise for both CLARINET_AF_INET6 and CLARINET_AF_UNSPEC all ipv6 fields must be equal.
 * Note that flowinfo is not considered an identifying part of an IPv6 address so two addresses A and B that conly
 * differ by flowinfo are considered equal.
 */
#define clarinet_addr_is_equal(a, b) \
  (((a)->family == (b)->family) \
&& (clarinet_addr_is_unspec(a) \
 || (((a)->as.ipv6.u.dword[3] == (b)->as.ipv6.u.dword[3]) \
  && (clarinet_addr_is_ipv4(a) \
   || (clarinet_addr_is_mac(a) \
    && ((a)->as.ipv6.u.dword[2] == (b)->as.ipv6.u.dword[2])) \
   || (clarinet_addr_is_ipv6(a) \
    && ((a)->as.ipv6.u.dword[0] == (b)->as.ipv6.u.dword[0]) \
    && ((a)->as.ipv6.u.dword[1] == (b)->as.ipv6.u.dword[1]) \
    && ((a)->as.ipv6.u.dword[2] == (b)->as.ipv6.u.dword[2]) \
    && ((a)->as.ipv6.scope_id == (b)->as.ipv6.scope_id))))))

/**
 * Returns true if addresses pointed by a and b are equivalent but not necessarily equal. This could be the case when
 * comparing an IPv4 address with an IPv4mappedToIPv6 address. They are never equal because the families involved are
 * different (one is INET the other INET6) but could be equivalent if both represent the same (ipv4) network address.
 * Note that flowinfo is not considered an identifying part of an IPv6 address so two addresses A and B that conly
 * differ by flowinfo are considered equal.
 */
#define clarinet_addr_is_equivalent(a, b) \
  (clarinet_addr_is_equal(a, b) \
|| (((a)->as.ipv6.u.dword[3] == (b)->as.ipv6.u.dword[3]) \
 && ((clarinet_addr_is_ipv4(a) && clarinet_addr_is_ipv4mapped(b)) \
  || (clarinet_addr_is_ipv4(b) && clarinet_addr_is_ipv4mapped(a)))))

#define clarinet_endpoint_is_equal(a, b)          (((a)->port == (b)->port) && clarinet_addr_is_equal(&(a)->addr, &(b)->addr))

#define clarinet_endpoint_is_equivalent(a, b)     (((a)->port == (b)->port) && clarinet_addr_is_equivalent(&(a)->addr, &(b)->addr))

/**
 * Converts the IPv4 mapped to IPv6 address pointed by src into an IPv4 address and copies it into the memory pointed
 * by @p dst. If src points to an IPv4 address then a simple copy is performed. On success returns CLARINET_ENONE. If either
 * dst or src are NULL or the address pointed by src is neither an IPv4MappedToIPv6 nor an IPv4 address then
 * CLARINET_EINVAL is returned instead.
 */
CLARINET_EXTERN
int
clarinet_addr_convert_to_ipv4(clarinet_addr* restrict dst,
                              const clarinet_addr* restrict src);

/**
 * Converts the IPv4 address pointed by src into an IPv4MappedToIPv6 address and copies it into the memory pointed by
 * dst. If src points to an IPv6 address then a simple copy is performed. On success returns CLARINET_ENONE.
 * If either dst or src are NULL or src does not point to an IPv4 address then CLARINET_EINVAL is returned instead.
 */
CLARINET_EXTERN
int
clarinet_addr_convert_to_ipv6(clarinet_addr* restrict dst,
                              const clarinet_addr* restrict src);

/**
 * Converts the addres pointed by src into a string in Internet standard format and store it in the buffer pointed by
 * @p dst.
 *
 * @return @c CLARINET_EINVAL if either @p src or @p dst are NULL, if the address pointed by @p src is invalid or @p
 * dstlen is not enough to contain the nul-terminated string. On success returns the number of characters written into dst not
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
CLARINET_EXTERN
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
CLARINET_EXTERN
int
clarinet_addr_from_string(clarinet_addr* restrict dst,
                          const char* restrict src,
                          size_t srclen);

/**
 * Returns the CIDR prefix length corresponding to the network mask pointed to by @p addr.
 *
 * @param addr Pointer to an address struture containing the network mask to convert.
 *
 * @return A decimal number between 0 and 128 corresponding to the CIDR prefix length
 * @return @c CLARINET_EAFNOSUPPORT: Address family not supported
 * @return @c CLARINET_EINVAL: Invalid address pointer
 */
CLARINET_EXTERN
int
clarinet_netmask_to_decimal(const clarinet_addr* addr);

/**
 * Obtains the network mask corresponding to the CIDR prefix length provided by @p prefix according to a given @p family.
 *
 * @param prefix Number of consecutive leading 1-bits counted from left to right in big-endian notation for the network
 * mask
 * @param family Address family as defined in <tt>enum clarinet_family</tt>. This argument is required because prefix
 * lengths lower than 33 are valid for multiple address families.
 * @param addr Pointer to an address struture where the network mask is to be stored.

 * @return @c CLARINET_ENONE: Success
 * @return @c CLARINET_EAFNOSUPPORT: Address family not supported
 * @return @c CLARINET_EINVAL: Invalid prefix length
 * @return @c CLARINET_EINVAL: Invalid address pointer
 */
CLARINET_EXTERN
int
clarinet_netmask_from_decimal(uint8_t cidr,
                              uint16_t family,
                              clarinet_addr* addr);

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
CLARINET_EXTERN
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
CLARINET_EXTERN
int
clarinet_endpoint_from_string(clarinet_endpoint* restrict dst,
                              const char* restrict src,
                              size_t srclen);

CLARINET_EXTERN
clarinet_addr
clarinet_make_ipv4(uint8_t a,
                   uint8_t b,
                   uint8_t c,
                   uint8_t d);

CLARINET_EXTERN
clarinet_addr
clarinet_make_ipv6(uint16_t a,
                   uint16_t b,
                   uint16_t c,
                   uint16_t d,
                   uint16_t e,
                   uint16_t f,
                   uint16_t g,
                   uint16_t h,
                   uint32_t scope_id);

CLARINET_EXTERN
clarinet_addr
clarinet_make_mac(uint8_t a,
                  uint8_t b,
                  uint8_t c,
                  uint8_t d,
                  uint8_t e,
                  uint8_t f);

CLARINET_EXTERN
clarinet_endpoint
clarinet_make_endpoint(clarinet_addr addr,
                       uint16_t port);

/* endregion */

/* region Library Initialization (from this point on all macros and functions require library initialization) */

/**
 * Static initialization. Must be called once before any function that depends on the network subsystem. This is the
 * case for all socket and host info functions. Only library and address functions can be safely called before
 * initialization (unless otherwise noted in the function description).
 */
CLARINET_EXTERN
int
clarinet_initialize(void);

/**
 * Static finalization. Must be called once for any previous calls to clarinet_initialize() that was successful.
 * Multiple calls have no effect. In many platforms this is a no-op but is strictly required on Windows to unload the
 * winsock library and release resources.
 */
CLARINET_EXTERN
int
clarinet_finalize(void);

/* endregion */

/* region Socket */

#if defined(__unix__)
#define CLARINET_SOCKET_HANDLE_TYPE     int
#else
#define CLARINET_SOCKET_HANDLE_TYPE     void*
#endif

/**
 * Abstract handle used by the underlying platform to identify a socket. Only valid while the socket is open. May be
 * reused after the socket is closed.
 */
typedef CLARINET_SOCKET_HANDLE_TYPE clarinet_socket_handle;

struct clarinet_socket
{
    uint16_t family;                /**< Address family (read-only) */
    clarinet_socket_handle handle;  /**< System handle (read-only) */
};

/**
 * Socket.
 *
 * @details Must be initialized using @c clarinet_socket_init() before it can be used. Sockets are not movable.
 * Pointers passed to functions must remain valid for the duration of the requested operation. Take care when using
 * stack allocated handles.
 */
typedef struct clarinet_socket clarinet_socket;

struct clarinet_linger
{
    uint16_t enabled;
    uint16_t seconds;
};

/** Data structure used to get/set the @c CLARINET_SO_LINGER socket option */
typedef struct clarinet_linger clarinet_linger;

/**
 * Initialize a sp structure
 *
 * @param [in] sp Socket pointer
 *
 * @details The memory pointed to by @p sp must have been previously allocated.
 *
 */
CLARINET_EXTERN
void
clarinet_socket_init(clarinet_socket* sp);

/** 
 * Create a new sp.
 *
 * @param [in] sp Socket pointer
 * @param [in] family
 * @param [in] proto
 *
 * @return @c CLARINET_ENONE on success or one of the following negative error codes:
 * @return @c CLARINET_EINVAL
 * @return @c CLARINET_EAFNOSUPPORT
 * @return @c CLARINET_EPROTONOSUPPORT
 * @return @c CLARINET_ENETDOWN
 * @return @c CLARINET_ELIBACC
 *
 * @details
 */
CLARINET_EXTERN
int
clarinet_socket_open(clarinet_socket* sp,
                     int family,
                     int proto);

/** 
 * Close the sp.
 *
 * @param [in] sp Socket pointer
 *
 * @return @c CLARINET_ENONE on success
 * @return @c CLARINET_EINVAL
 * @return @c CLARINET_EALREADY
 * @return @c CLARINET_EINTR
 * @return @c CLARINET_ENETDOWN
 * @return @c CLARINET_ELIBACC
 *
 * @details On success this function reinitializes the sp structure pointed to by @p sp. Otherwise an error code is
 * returned and the memory is left unmodified. It is then up to the the user to call @c clarinet_socket_init() should
 * the same sp be reused. Therefore it is only safe to call @c clarinet_socket_close() successvely with the same
 * sp if a previous closing attempt did not fail. There might be specific situations on certain platforms in which a
 * sp could be safely closed again after a previously failed attempt but in general, unless explicitly stated
 * otherwise, any closing operation is final and should not be re-tried in case of an error. Trying to close the same
 * sp twice like this may lead to a (reused) file descriptor in another thread being closed unintentionally. This
 * might occur because the system can release the descriptor associated with the sp early in a close operation,
 * freeing it for reuse by another thread. Still the steps that might produce an error, such as flushing data, could
 * occur much later in the closing process and eventually result in an error being returned.
 *
 * @note Behaviour is undefined if the sp pointed by sp is not initialized.
 *
 */
CLARINET_EXTERN
int
clarinet_socket_close(clarinet_socket* sp);

CLARINET_EXTERN
int
clarinet_socket_bind(clarinet_socket* restrict sp,
                     const clarinet_endpoint* restrict local);

/** 
 * Get the local endpoint bound to the sp.
 *
 * @details This is the local address previously passed to clarinet_socket_bind() or implicitly bound by 
 * clarinet_socket_connect(). This is not necessarily the same address used by a remote peer to send this host a packet 
 * because the local host could be behind a NAT and it may not even match the destination address in a received IP 
 * packet because the host can be bound multiple addresses using a wildcard address.
 *clarinet_socket_bind()
 *
 * @return @c CLARINET_ENONE: Success
 *
 * @return @c CLARINET_EINVAL: @p sp is NULL
 * @return @c CLARINET_EINVAL: @p local is NULL
 * @return @c CLARINET_EINVAL: The sp pointed to by @p sp is invalid
 * @return @c CLARINET_EINVAL: The sp pointed to by @p sp has not been bound to a local endpoint yet. A sp is
 * only bound to a local endpoint after a successful call to @c clarinet_socket_bind() or @c clarinet_socket_connect().
 */
CLARINET_EXTERN
int
clarinet_socket_local_endpoint(clarinet_socket* restrict sp,
                               clarinet_endpoint* restrict local);

/** 
 * Get the remote endpoint connected to the sp.
 *
 * @details For TCP sockets, once clarinet_socket_connect() has been performed, either sp can call
 * clarinet_socket_remote_endpoint() to obtain the address of the peer sp.  On the other hand, UDP sockets are
 * connectionless. Calling clarinet_socket_connect() on a UDP sp merely sets the peer address for outgoing datagrams
 * sent with clarinet_socket_send() or clarinet_socket_recv(). The caller of clarinet_socket_connect(2) can use 
 * clarinet_socket_remote_endpoint() to obtain the peer address that it earlier set for the sp.  However, the peer
 * sp is unaware of this information, and calling clarinet_socket_remote_endpoint() on the peer sp will return
 * no useful information (unless a clarinet_socket_connect() call was also executed on the peer).  Note also that the 
 * receiver of a datagram can obtain the address of the sender when using clarinet_socket_recvfrom().
 */
CLARINET_EXTERN
int
clarinet_socket_remote_endpoint(clarinet_socket* restrict sp,
                                clarinet_endpoint* restrict remote);

CLARINET_EXTERN
int
clarinet_socket_send(clarinet_socket* restrict sp,
                     const void* restrict buf,
                     size_t buflen);


CLARINET_EXTERN
int
clarinet_socket_sendto(clarinet_socket* restrict sp,
                       const void* restrict buf,
                       size_t buflen,
                       const clarinet_endpoint* restrict remote);


CLARINET_EXTERN
int
clarinet_socket_recv(clarinet_socket* restrict sp,
                     void* restrict buf,
                     size_t buflen);

CLARINET_EXTERN
int
clarinet_socket_recvfrom(clarinet_socket* restrict sp,
                         void* restrict buf,
                         size_t buflen,
                         clarinet_endpoint* restrict remote);

/**
 * Set sp option.
 *
 * @details All sp options have a unique @p optname across all protocols. This is safer than allowing independent
 * numbering under each option level (i.e. protocol layer). The alternative of passing an <i>option level</i> as in @c
 * setsockopt(2) may lead to accidental modifications without warning when the user mistakes the protocol level and the
 * wrong level happens to have an option name with the same value
 *
 * @param [in] sp Socket pointer
 * @param [in] optname
 * @param [in] optval
 * @param [in] optlen
 *
 * @return @c CLARINET_ENONE
 * @return @c CLARINET_EINVAL
 *
 * @details
 *
 * @note
 *
 */
CLARINET_EXTERN
int
clarinet_socket_setopt(clarinet_socket* restrict sp,
                       int optname,
                       const void* restrict optval,
                       size_t optlen);

/**
 * Get sp option.
 *
 * @param [in] sp
 * @param [in] optname
 * @param [in, out] optval
 * @param [in, out] optlen
 *
 * @return
 */
CLARINET_EXTERN
int
clarinet_socket_getopt(clarinet_socket* restrict sp,
                       int optname,
                       void* restrict optval,
                       size_t* restrict optlen);

/**
 * Connects the sp to a remote host.
 *
 * @details // TODO: document non-blocking connect(2) and SO_ERROR detection.
//      In the case of a non-blocking socket, connect(2) may return -1/EINPROGRESS or -1/EWOUDLBLOCK. In this case, the
//     * only way to determine when the connection process is complete is by calling select(2) or poll(2) and check for
//     * writeability. But this check is ambiguous. The socket will indicate that it is ready to write once the connection
//     * process is complete regardless of the outcome. The connection might as well have failed. In this case, a
//     * subsequent call to @c send(2) will fail but this means the user can only determine the error condition much later
//     * and on many occasions the application needs to receive some data before it can have anything to send. As an
//     * alternative, one could call getsockopt(2) passing SO_ERROR and check if the connection process ended with a
//     * non-zero error code. The caveat is that most Unix platforms will fetch AND RESET the socket error code with
//     * getsockopt(2) as expected but the value will remain "dirty" if getsockopt(2) is not called even after other
//     * successful calls to the socket. According to POSIX the value of SO_ERROR is only defined immediately after a
//     * socket call fails. Besides, SO_ERROR is only used to report asynchronous errors that are the result of events
//     * within the network stack and not synchronous errors that are the result of a blocking call (send/recv/connect).
//     * Synchronous results are reported via errno. Calling getsockopt(2) with SO_ERROR after a blocking library call is
//     * undefined behaviour while finding the non-blocking connect(2) result via select(2) is an example of discovering
//     * when an asynchronous result is ready to be retrieved via SO_ERROR. In some systems, trying to fetch and reset
//
 *
 * @param [in] sp
 * @param [in] remote
 *
 * @return
 *
 *
 */
CLARINET_EXTERN
int
clarinet_socket_connect(clarinet_socket* restrict sp,
                        const clarinet_endpoint* restrict remote);

/**
 * Listen for connections on a sp.
 *
 * @param [in] sp Socket pointer
 * @param [in] backlog Maximum length for the queue of pending connections. Must be greater than or equal to zero.
 * Values between 0 and 5 should work in all platforms. Values greater than 5 may not be supported on platforms
 * with particular resource constraints.
 *
 * @return @c CLARINET_ENONE: Success
 *
 * @return @c CLARINET_EINVAL: @p sp is NULL.
 * @return @c CLARINET_EINVAL: The sp pointed by @p sp is invalid. The sp must be open and bound to a local
 * endpoint.
 * @return @c CLARINET_EINVAL: @p backlog is out of range.
 *
 * @return @c CLARINET_EPROTONOSUPPORT: Operation is not supported by the sp protocol.
 *
 * @details This function will fail if the sp is not from a compatible protocol. Currently the only compatible
 * protocol is @c CLARINET_PROTO_TCP. The value provided by @p backlog is just a hint and the system is free to adjust
 * or ignore it. Calling @c clarinet_socket_listen() multiple times on a listen sp with possibly a different
 * @p backlog value is not recommended. It should not fail but the behaviour is platform dependent and can vary
 * considerably. Setting the @p backlog parameter to 0 in a subsequent call to @c clarinet_Socket_listen() on a
 * listening sp is not considered a proper reset, especially if there are connections in the queue.
 *
 * @note @b WINDOWS: The socket is considered invalid if not explicitly bound to a local endpoint first. The value of
 * @p backlog is clamped to the interval [200, 65535]. Subsequent calls to @c clarinet_socket_listen() on the same
 * listening sp with a valid @p backlog will ALWAYS succeed but the new @p backlog will be ignored by the system.
 *
 * @note @b LINUX: If the sp is not explicitly bound to a local endpoint, the system will implicitly assign a port
 * and default binding like it does when one calls @c clarinet_socket_connect() without a previous call to
 * @c clarinet_socket_bind(). The behavior of the @p backlog argument on TCP sockets changed with kernel 2.2. Now it
 * specifies the queue length for completely established sockets waiting to be accepted, instead of the number of
 * incomplete connection requests. The maximum length of the queue for incomplete sockets can be set using
 * /proc/sys/net/ipv4/tcp_max_syn_backlog. When syncookies are enabled (net.ipv4.tcp_syncookies=1) there is no logical
 * maximum length and this setting is ignored. See <a href="https://man7.org/linux/man-pages/man7/tcp.7.html">tcp(7)</a>
 * for more information. If @p backlog is greater than the value in /proc/sys/net/core/somaxconn, then it is silently
 * capped to that value. Since Linux 5.4, the default value in this file is 4096; in earlier kernels, the default value
 * is 128. In kernels before 2.4.25, this limit was a hard coded constant, @c SOMAXCONN = 128. Subsequent calls to
 * @c clarinet_socket_listen() on the listening sp will adjust the backlog queue length using the new @p backlog
 * value, but only for future connection requests. It does not discard any pending connections already in the queue.
 *
 * @note @b BSD/DARWIN: If the sp is not explicitly bound to a local endpoint, the system will implicitly assign a
 * port and default binding like it does when one calls @c clarinet_socket_connect() without a previous call to
 * @c clarinet_socket_bind(). The real maximum queue length will be 1.5 times more than the value specified by
 * @p backlog. Any subsequent call to @c clarinet_socket_listen() on the listening sp allows the caller to change
 * the maximum queue length using a new @p backlog argument. If a connection request arrives with the queue full, the
 * client may receive an error with an indication of @c CLARINET_ECONNREFUSED, or, in the case of TCP, the connection
 * will be silently dropped. Current queue lengths of listening sockets can be queried using @c netstat(1) command. Note
 * that before FreeBSD 4.5 and the introduction of the syncache, @p backlog also determined the length of the incomplete
 * connection queue, which held TCP sockets in the process of completing TCP's 3-way handshake. These incomplete
 * connections are now held entirely in the syncache, which is unaffected by queue lengths. Inflated backlog values to
 * help handle denial of service attacks are no longer necessary. The @c sysctl(3) MIB variable
 * @c kern.ipc.soacceptqueue specifies a hard limit on backlog; if a value greater than kern.ipc.soacceptqueue or less
 * than zero is specified, @p backlog is silently forced to @c kern.ipc.soacceptqueue. If the listen queue overflows,
 * the kernel will emit a @c LOG_DEBUG syslog message. The @c sysctl(3) MIB variable kern.ipc.sooverinterval specifies a
 * per-sp limit on how often the kernel will emit these messages.
 */
CLARINET_EXTERN
int
clarinet_socket_listen(clarinet_socket* sp,
                       int backlog);

/**
 *
 * @param [in] ssp Server socket pointer
 * @param [in] csp Client socket pointer
 * @param [out] remote Remote end point of the client socket
 * @return
 *
 * @note A failure to obtain the remote address of the connected socket is not considered an error because in theory a
 * platform might not be able to provide that information immediately and would require an explicit call to
 * @c clarinet_socket_remote_endpoint() for that purpose. There is currently no supported platform to which this is the
 * case.
 */
CLARINET_EXTERN
int
clarinet_socket_accept(clarinet_socket* restrict ssp,
                       clarinet_socket* restrict csp,
                       clarinet_endpoint* restrict remote);

/**
 *
 * @param [in] sp
 * @param [in] flags
 *
 * @return @c CLARINET_ENONE
 * @return @c CLARINET_ENOTCONN: The socket pointed to by @c sp is not connected.
 *
 * @details Once the shutdown function is called to disable send, receive, or both, there is no method to re-enable that
 * capability for the existing sp connection.
 *
 * @note @b WINDOWS: A UDP socket can be shutdown even when not connected so @c CLARINET_ENOTCONN will only be returned
 * for TCP sockets.
 */
CLARINET_EXTERN
int
clarinet_socket_shutdown(clarinet_socket* restrict sp,
                         int flags);


struct clarinet_socket_poll_target
{
    clarinet_socket* socket;    /**< Socket to poll. */
    uint16_t events;            /**< Socket Event flags to poll. */
};

typedef struct clarinet_socket_poll_target clarinet_socket_poll_target;

/**
 * Calculates the size in bytes required for a @c clarinet_socket_poll_list to accomodate the number of items
 * indicated by @p count.
 *
 * @param [in] count Number of items in the list
 *
 * @return @c N > 0 Size in bytes of the memory block that must be allocated for the @c clarinet_socket_poll_list.
 * @return @c CLARINET_EINVAL
 */
CLARINET_EXTERN
int
clarinet_socket_poll_context_calcsize(size_t count);

/**
 * Retrieve a specific socket poll status.
 *
 * @param [in] context Buffer used in a previous call to @c clarinet_socket_poll().
 * @param [in] index Index of the status to retrieve.
 * @param [out] status Status report of the target.
 *
 * @return @c CLARINET_ENONE
 * @return @c CLARINET_EINVAL
 *
 * @details It's guaranteed that the index @a i of every result is the same of it's corresponding target in the array
 * passed to @c clarinet_socket_poll().
 */
CLARINET_EXTERN
int
clarinet_socket_poll_context_getstatus(const void* restrict context,
                                       size_t index,
                                       uint16_t* restrict status);

/**
 * Determines the status of one or more sockets.
 *
 * @param [in] context Buffer used to store the operation results.
 * @param [in] targets Array of sockets and the corresponding events to report.
 * @param [in] count Number of elements in the @c targets array.
 * @param [in] timeout Number of milliseconds that the function should block waiting for an event on any target.
 *
 * @return @c CLARINET_ENONE
 * @return @c CLARINET_EINVAL
 *
 * @details On success the function @c clarinet_socket_poll_context_getstatus() can be used to fetch the status of a
 * specific target. The caller is responsible for allocating the memory pointed to by @c context which is where the operation
 * results are stored. The function @c clarinet_socket_poll_context_calcsize() can be used to determine the required
 * size in bytes according to the number of targets.
 */
CLARINET_EXTERN
int
clarinet_socket_poll(void* restrict context,
                     const clarinet_socket_poll_target* restrict targets,
                     size_t count,
                     int timeout);

/* endregion */

/* region Interface */

struct clarinet_iface
{
    uint32_t index;          /* Interface index */

    /**
     * Interface address. It may be contain a link, inet or inet6 address.
     *
     * @note Link addresses contain the  of the interface. For example, on an Ethernet
     * network this would be an Ethernet hardware address of 6 bytes. Other link layers may provide MAC addresses of
     * different lengths.
     */
    clarinet_addr addr;

    clarinet_addr netmask;   /*	Interface netmask associated with the interface address when applicable */
};

typedef struct clarinet_iface clarinet_iface;

CLARINET_EXTERN
int
clarinet_iface_getlist(clarinet_iface* restrict list,
                       size_t* len);

CLARINET_EXTERN
int
clarinet_iface_getindex(const char* name,
                        uint32_t* index);

CLARINET_EXTERN
int
clarinet_iface_getname(uint32_t index,
                       char* name,
                       size_t len);

/* endregion */

/* @formatter:off */
#if defined(__cplusplus)
    #if defined(restrict) && !RESTRICT_PREDEFINED
        #undef restrict
    #endif
#endif
/* @formatter:on */

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif /* CLARINET_H */
