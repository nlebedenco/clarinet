// TODO: decide what to do about non-blocking connect(2) and SO_ERROR detection. Are we going to provide a clarinet_socket_poll() ?
//     /* In the case of a non-blocking socket, connect(2) may return -1/EINPROGRESS or -1/EWOUDLBLOCK. In this case, the
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
//     */


/***********************************************************************************************************************
 *
 * NOTES:
 *
 * - In POSIX, names ending with _t are reserved. Since we're targeting at least one POSIX system (i.e. Linux) typenames
 * defined here MUST NOT end with _t.
 *
 * - Macros that receive arguments should normally be defined in lower case following the same name convention of
 * functions, so no builds will break if we eventually replace those macros with real functions.
 *
 * - Unfortunately, struct and union initialization in C/C++ is a mess. C99 supports member designators but C++ does not.
 * C99 supports compound literals but C++ does not save for certain compiler extensions. Currently, GCC and clang are
 * known to support compound literals in C++. MSVC supports it in C but not in C++. The solution for now is to export
 * global consts which can be used for initialization.
 *
 * - Global constants can be used for initialization in C++ (instead of compound literals which some compilers don't
 * support e.g. MSVC)
 *
 * - Structs must be compared member by member. It's not safe to compare structs using memcmp() because content of
 * padding spaces is undefined and memcmp() will blindly compare every byte of allocated memory.
 *
 * - A socket must be open and bound to a local address in two distinct operations. Note that certain options may only 
 * be set BEFORE the socket is bound. 
 *
 * - Default socket options may vary according to the platform. Some platforms may even provide system wide settings in 
 * which case only the programmer can decide whether or not an application should override a certain option. For 
 * example, the global default for dual stack support on Linux can be configured by the file
 * /proc/sys/net/ipv6/bindv6only.
 *
 * - Normally two sockets with the same protocol cannot be bound to the same local address and port. This is called a
 * bind conflict. Binding socket A to proto/ipA:portA and socket B to proto/ipB:portB is always possible if either
 * portA != portB or proto/ipA != proto/ipB, where proto is either UDP or TCP. E.g. socket A belongs to an FTP server
 * that is bound to 192.168.0.1:21 and socket B belongs to another FTP server program bound to 10.0.0.1:21, both
 * bindings will succeed. Keep in mind, though, that a socket may be locally bound to "any address" also denoted a
 * wildcard which is represented by the address 0.0.0.0 on ipv4 and :: on ipv6. If a socket is bound to 0.0.0.0:21, it
 * is bound to all existing local addresses at the same time in ipv4 space in which case no other socket can be bound to
 * port 21 in the same address space, regardless of which specific IP address it tries to bind to since the wildcard
 * conflicts with all existing local addresses in its space. This is of particular significance when binding to :: with
 * dual stack mode enabled because then the ipv6 wildcard :: occupies all addresses in both ipv6 and ipv4 space. See
 * clarinet_socket_open() for details.
 *
 * - Note that clarinet_socket_connect() has different semantics for UDP and TCP and a slightly different behaviour
 * for UDP depending on the platform. On Unix (including macOS) when a UDP socket is bound to a foreign address by
 * clarinet_socket_connect() it effectively assumes a 5-tuple identity so when a datagram arrives, the system first
 * selects all sockets associated with the src address of the packet and then selects the socket with the most specific
 * local address matching the destination address of the packet. On Windows however, UDP associations established with
 * clarinet_socket_connect() do not affect routing. They only serve as defaults for clarinet_socket_send() and
 * clarinet_socket_recv() so on Windows all UDP sockets have a foreign address *:* and the first entry on the routing
 * table with a local address that matches the destination address of the arriving packet is picked (generally the last
 * socket open on the same port). This basically prevents UDP servers from ever using clarinet_socket_connect() to
 * operate with multiple client sockets like TCP does.
 *
 * - Note that besides platform support, dual-stack also requires a local IPv6 address (either an explicit one or the
 * wildcard [::] which is equalt to clarinet_addr_ipv6_any). The ability to interact with IPv4 hosts requires the use
 * of the IPv4MappedToIPv6 address format. Any IPv4 address must be represented in the IPv4MappedToIPv6 address format
 * which enables an IPv6-only application to communicate with an IPv4 node. The IPv4MappedToIPv6 address format allows
 * the IPv4 address of an IPv4 node to be represented as an IPv6 address. The IPv4 address is encoded into the low-order
 * 32 bits of the IPv6 address, and the high-order 96 bits hold the fixed prefix 0:0:0:0:0:FFFF. The IPv4MappedToIPv6
 * address format is specified in RFC4291. Applications must take care to handle these IPv4MappedToIPv6 addresses
 * appropriately and only use them with dual stack sockets. If an IP address is to be passed to a regular IPv4 socket,
 * the address must be a regular IPv4 address not a IPv4MappedToIPv6 address.
 *
 * - An application with a socket bound to [::] (IPv6) and dual-stack enabled occupies the port on both IPv6 and IPv4.
 * Therefore, a second socket cannot be bound to 0.0.0.0 (IPv4 only) with the same protocol on the same port unless
 * CLARINET_SO_REUSEADDR is used. Note however that in this particular case it is impossible to determine which
 * socket will handle incoming IPv4 packets and behaviour will depend on the operating system.
 *
 * - The socket option SO_ERROR defined by BSD sockets and Winsock is not mapped into a clarinet socket option because
 * it is of very limited use and it's behaviour may vary wildly across platforms. It is currently only used internally
 * in the connection process of non-blocking sockets using the TCP protocol. See @clarinet_socket_connect() for more
 * information.
 *
 * - All clarinet socket have a UNIQUE integer identifier (aka optname) across all levels/protocols. This is important
 * so the user does not have to pass a level/protocol identifier too. Uniqueness in this case is not just a matter of
 * convenience but safety and sanity. Consider the well-known @c setsockopt(2) function. When option levels are allowed
 * to share identifiers there is always a risk the user might pass the wrong @c optlevel by mistake and yet see no error
 * because the mistaken option level happens to define an @c optname with the same identifier. Such mistakes can be hard
 * to detect and can be fairly common with low value option identifiers (i.e. 1, 2, 3,...).
 *
 * - All boolean values are represented as integers where 0 is false and any non-zero value is true (including negative
 * values!) so users should be aware that <c>if (var == 0)</c> is equivalent to <c> if (var == FALSE) </c> but
 * the opposite does not hold true, that is, <c>if (var == 1)</c> IS NOT equivalent to <c> if (var == TRUE) </c>.
 * Boolean variables should always be evaluated implicitly as in <c>if (var) { ... } </c> instead.
 *
 **********************************************************************************************************************/
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

/* region Utility Macros */

#define CLARINET_STR(s)     #s
#define CLARINET_XSTR(s)    CLARINET_STR(s)

/**
 * Returns the smallest even integer that is greater than or equal to the integer provided.
 * This macro can be particular useful when settings buffer sizes from variables.
 */
#define CLARINET_EVEN(i)    ((((i) + 1) >> 1) << 1)


/* @formatter:off */

/** Check whether this is GCC major.minor or a later release. */
#if !defined(__GNUC__)
    /* Not GCC and not "just like GCC" */
    #define CLARINET_IS_AT_LEAST_GNUC_VERSION(major, minor) 0
#else
    /* GCC or "just like GCC" */
    #define CLARINET_IS_AT_LEAST_GNUC_VERSION(major, minor) (__GNUC__ > (major) || (__GNUC__ == (major) && __GNUC_MINOR__ >= (minor)))
#endif

/** Check whether this is Clang major.minor or a later release. */
#if !defined(__clang__)
    /* Not Clang */
    #define CLARINET_IS_AT_LEAST_CLANG_VERSION(major, minor) 0
#else
    /* Clang */
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

/***********************************************************************************************************************
 * API symbols
 *
 * CLARINET_EXPORT is defined by the build system when building as a shared library. In this case we can arrange to
 * export only the necessary symbols by defining CLARINET_EXTERN accordingly.
 *
 * On Windows it is advantageous for headers accompanying DLLs to declare consumed symbols with
 * __declspec(dllimport) because the compiler can alledgedly produce more efficient code if the attribute is present.
 * See https://docs.microsoft.com/en-us/cpp/build/importing-into-an-application-using-declspec-dllimport?view=msvc-160
 *
 * CLARINET_IMPORT is defined by the build system for targets consuming this header with a shared library.
 *
 * It is an error to have both CLARINET_EXPORT and CLARINET_IMPORT defined at the same time.
 *
 **********************************************************************************************************************/

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

/*
 * Replace 'restrict' in C++ with something supported by the compiler.
 * MSVC Intellisense doesn't like the "restrict" keyword either.
 */
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
#define CLARINET_DECLARE_ENUM_ITEM(e, v, s) e = (v),

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
    E(CLARINET_ENOTSOCK,        -15, "Operation attempted with an invalid socket descriptor") \
    E(CLARINET_EMSGSIZE,        -16, "Message too large") \
    E(CLARINET_ENOTSUP,         -17, "Operation is not supported") \
    E(CLARINET_ENOBUFS,         -18, "Not enough buffer space or queue is full") \
    E(CLARINET_EAFNOSUPPORT,    -19, "Address family not supported") \
    E(CLARINET_EPROTONOSUPPORT, -20, "Protocol not supported") \
    E(CLARINET_EADDRINUSE,      -22, "Address already in use") \
    E(CLARINET_EADDRNOTAVAIL,   -23, "Address is not available/cannot be assigned") \
    E(CLARINET_ENETDOWN,        -24, "Network is down") \
    E(CLARINET_ENETUNREACH,     -25, "Network is unreachable") \
    E(CLARINET_ENETRESET,       -26, "Network reset possibly due to keepalive timeout") \
    E(CLARINET_ENOTCONN,        -27, "Socket is not connected") \
    E(CLARINET_EISCONN,         -28, "Socket is already connected") \
    E(CLARINET_ECONNABORTED,    -29, "Connection aborted (closed locally)") \
    E(CLARINET_ECONNRESET,      -30, "Connection reset by peer (closed remotely)") \
    E(CLARINET_ECONNSHUTDOWN,   -31, "Connection is shutdown and cannot send") \
    E(CLARINET_ECONNTIMEOUT,    -32, "Connection timeout") \
    E(CLARINET_ECONNREFUSED,    -33, "Connection refused") \
    E(CLARINET_EHOSTDOWN,       -34, "Host is down") \
    E(CLARINET_EHOSTUNREACH,    -35, "No route to host")     \
    E(CLARINET_EPROCLIM,        -36, "Too many processes or tasks") \
    E(CLARINET_EMFILE,          -37, "Too many files") \
    E(CLARINET_ELIBACC,         -38, "Cannot access a needed shared library") \
    E(CLARINET_ELIBBAD,         -39, "Accessing a corrupted shared library") \


enum clarinet_error
{
    CLARINET_ERRORS(CLARINET_DECLARE_ENUM_ITEM)
};

CLARINET_EXTERN
const char*
clarinet_error_name(int err);

CLARINET_EXTERN
const char*
clarinet_error_description(int err);

/* endregion */

/* region Family Codes */


/* Address Families */

#define CLARINET_FAMILIES(E) \
    E(CLARINET_AF_UNSPEC,         0, "Unspecified") \
    E(CLARINET_AF_INET,           2, "IPv4") \
    E(CLARINET_AF_INET6,         10, "IPv6") \
    E(CLARINET_AF_LINK,          18, "MAC") \

enum clarinet_family
{
    CLARINET_FAMILIES(CLARINET_DECLARE_ENUM_ITEM)
};

CLARINET_EXTERN
const char*
clarinet_family_name(int err);

CLARINET_EXTERN
const char*
clarinet_family_description(int err);

/* endregion */

/* region Protocol Codes */

#define CLARINET_PROTOS(E) \
    E(CLARINET_PROTO_NONE,        0,       "None") \
    E(CLARINET_PROTO_UDP,         1 <<  2, "User Datagram Protocol (RFC768)") \
    E(CLARINET_PROTO_TCP,         1 <<  3, "Transmission Control Protocol (RFC793)") \
    E(CLARINET_PROTO_DTLC,        1 << 10, "Datagram Transport Layer Connectivity (Custom protocol over UDP)") \
    E(CLARINET_PROTO_DTLS,        1 << 11, "Datagram Transport Layer Security (RFC6347)") \
    E(CLARINET_PROTO_TLS,         1 << 12, "Transport Layer Security (RFC8446)") \
    E(CLARINET_PROTO_GDTP,        1 << 20, "Game Data Transport Protocol (Custom protocol over DTLC)") \
    E(CLARINET_PROTO_GDTPS,       1 << 21, "Game Data Transport Protocol Secure (UDT over DTLS)") \
    E(CLARINET_PROTO_ENET,        1 << 22, "ENet (Custom protocol based on http://enet.bespin.org/index.html)") \
    E(CLARINET_PROTO_ENETS,       1 << 23, "ENet Secure (Custom ENet over DTLS)") \


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

/* region Feature flags */

#define CLARINET_FEATURE_NONE       (0)         /**< None */
#define CLARINET_FEATURE_DEBUG      (1 << 0)    /**< Debug information built-in */
#define CLARINET_FEATURE_PROFILE    (1 << 1)    /**< Profiler instrumentation built-in */
#define CLARINET_FEATURE_LOG        (1 << 2)    /**< Log built-in */
#define CLARINET_FEATURE_IPV6       (1 << 3)    /**< Support for IPv6 */
#define CLARINET_FEATURE_IPV6DUAL   (1 << 4)    /**< Support for IPv6 in dual-stack mode */

/* endregion */

/* region Socket Shutdown Flags */

#define CLARINET_SD_NONE            (0)
#define CLARINET_SD_RECV            (1 << 0)                                /**< Shutdown Receive */
#define CLARINET_SD_SEND            (1 << 1)                                /**< Shutdown Send */
#define CLARINET_SD_BOTH            (CLARINET_SD_RECV|CLARINET_SD_SEND)     /**< Shutdown Both */

/* endregion */

/* region Socket Options (all socket options must have a unique integer identifier) */

#define CLARINET_SO_NONBLOCK        1           /**< Enable/disable non-blocking mode */

/**
 * Controls how @c clarinet_socket_bind() should handle local address/port conflicts internally.
 * This option stores a 32-bit integer value. Valid values are limited to 0 (false) and non-zero (true).
 *
 * @details A <b>partial conflict</b> is said to occur when a socket tries to bind to a specific local address despite a
 * pre-existing socket bound to a wildcard in the same space. An <b>exact conflict</b> occurs when a socket tries to
 * bind to the EXACT same address/port of a pre-existing socket regardless of the address being specific or a wildcard.
 * When a socket is allowed to bind despite of a conflict it is said to be reusing the address/port. Note however that
 * not all underlying systems provide the same level of support to address/port reuse and a few discrepancies are
 * inevitable. Also note that even when disregarding broadcast and multicast, address reuse on UDP sockets have slightly
 * different implications than on TCP sockets because there is no TIME_WAIT state involved.
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
 * @note @code
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
 * @endcode
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
 * @note @code
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
 * @endcode
 *
 * @note This way all expected results are supported by the majority of the platforms and the complete behaviour of
 * @c CLARINET_SO_REUSEADDR regarding two sockets bound using @c clarinet_socket_bind() can be completely defined as
 * follows:
 *
 * @note @code
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
 * @endcode
 */
#define CLARINET_SO_REUSEADDR       2

/**
 * Socket buffer size for output. This option stores 32-bit integer value. Valid values are limited to the range
 * [1, INT_MAX]. Behaviour is undefined for negative values. A value of zero (0) may yield different results depending
 * on the platform but these should be well defined (see notes).
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
 * when it states that: "For datagram sockets, the SO_SNDBUF value imposes an upper limit on the size of outgoing
 * datagrams. This limit is calculated as the doubled (see socket(7)) option value less 32 bytes used for overhead." The
 * part about doubling is valid but all the rest is wrong. Alsoa according to the socket(7) man page, the send buffer
 * has a hard minimum value of 2048 and the recv buffer a minimum of 256 but kernel code shows that the minimum
 * send-buffer size is 4096+480 bytes on x64 (4096+384 on x86) and the minimum recv buffer is 2048+244 on x64
 * (2048+192 on x86). Default and maximum values can be verified and adjusted by the following sysctl variables:
 *
 * @note @code
 *
 *   net.core.rmem_default=212992
 *   net.core.wmem_default=212992
 *   net.core.rmem_max=212992
 *   net.core.wmem_max=212992
 *   net.core.netdev_max_backlog=1000
 *
 * @endcode
 *
 * @note These are calculated at boot according to total system memory and apply to both ipv4 and ipv6 (despite the
 * name) [<a href="https://man7.org/linux/man-pages/man7/udp.7.html">source</a>]:
 *
 * @note @code
 *
 *   net.ipv4.udp_mem.min
 *   net.ipv4.udp_mem.pressure
 *   net.ipv4.udp_mem.max
 *
 * @endcode
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
 * @note @code
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
 * @endcode
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
 * @note @code
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
 * @endcode
 *
 * @note Note that FeeeBSD (and possibly Darwin) adjusts the value from kern.ipc.maxsockbuf as follows
 * [<a href="https://github.com/freebsd/freebsd-src/blob/de1aa3dab23c06fec962a14da3e7b4755c5880cf/sys/kern/uipc_sockbuf.c#L599">source</a>]:
 *
 * @code
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
 * Socket buffer size for input.  This option stores a 32-bit integer value. Valid values are limited to [1, INT_MAX]
 *
 * @details See @c CLARINET_SO_SNDBUF for complete details and notes.
 */
#define CLARINET_SO_RCVBUF          4

#define CLARINET_SO_SNDTIMEO        5           /**< Send buffer timeout */

#define CLARINET_SO_RCVTIMEO        6           /**< Receive buffer timeout */

#define CLARINET_SO_KEEPALIVE       7           /**< Enable/disable keepalive (currently only supported by TCP) */

/**
 * Socket linger timeout. This option stores a @c clarinet_linger.
 *
 * @details
 *
 * @note
 */
#define CLARINET_SO_LINGER          8

#define CLARINET_SO_DONTLINGER      9           /**< Enable/disable linger without affecting the timeout already configured */

/**
 * Enable/Disable Dual Stack on an IPV6 socket. This option stores a 32-bit integer value. Valid values are limited
 * to 0 (false) and non-zero (true).
 *
 * @details If this options is set to 1 , then the socket is restricted to sending and receiving IPv6 packets only.  In
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
 * Unicast Time-To-Live for IPv4 or Hop Limit for IPv6. This option stores a 32-bit integer value. Valid values are
 * limited to the range [1, 255].
 *
 * @details This is the value used in the IP header when sending unicast packets. This option is considered a hint to
 * the system and not a strict requirement. Underlying IP stacks may ignore this option without returning an error.
 *
 * @note On WINDOWS default TTL is defined by @c KEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\DefaultTTL.
 * Winsock implementations are not required to return an error if the corresponding underlying socket option is not
 * supported. Users are supposed to check if the value was effectively set by calling @c clarinet_socket_getopt() which
 * usually will return an error if the option is not supported by the platform. In any case, all Windows versions since
 * Windows 95 support this option for IPv4 and all since Windows Vista, for IPv6.
 */
#define CLARINET_IP_TTL             101

#define CLARINET_IP_MTU             102         /**< Get the current known path MTU of the current socket. */

#define CLARINET_IP_MTU_DISCOVER    103         /**< Enable/disable path MTU discovery mode. */

/* endregion */

/* region MTU Discovery Modes */

#define CLARINET_PMTUD_UNSPEC       0           /**< Use per-route settings. Socket will set DF=0 and fragment datagrams larger than IP_MTU, otherwise will set DF=1 before sending.*/
#define CLARINET_PMTUD_ON           1           /**< Always do Path MTU Discovery. Socket will set DF=1 and fail to send datagrams larger than IP_MTU. */


/**
 * Never do Path MTU Discovery.
 *
 * @details Socket will set DF=0 and fragment datagrams larger than the interfce MTU, except on Linux <= 3.15 where the
 * fragmentation does not occur and the send operation will fail instead (see notes).
 *
 * @note On Linux @c IP_PMTUDISC_DONT will handle ICMP Type 3 Code 4 packets even though datagrams are only sent with
 * DF=0 (WTF!?). So in kernel 3.13 a new mode was introduced, @c IP_PMTUDISC_INTERFACE, to ignore the path MTU estimate
 * and always use the interface MTU instead. ICMP packets, which are clearly spoofed in this case, are ignored and
 * datagrams are sent with DF=0. However @c send(2) will fail if the datagram is larger than the interface MTU
 * (WTF again!?). In kernel 3.15 another mode was introduced, @c IP_PMTUDISC_OMIT, which behaves exactly like
 * @c IP_PMTUDISC_INTERFACE but fragments datagrams larger than the interface MTU.
 *
 * @note On Windows this flag has the correct semantics and will fragment packets larger then the interface MTU.
 *
 * @note On Windows this flag has the correct semantics and will fragment packets larger then the interface MTU.
 */
#define CLARINET_PMTUD_OFF          2
#define CLARINET_PMTUD_PROBE        3           /**< Socket will set DF=1 and send the datagram (unfragmented) even if it is larger than IP_MTU. */

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
#define CLARINET_HOSTNAME_STRLEN    (255+1)

/** Maximum string length required to store an interface name */
#define CLARINET_IFNAME_STRLEN      (15+1)

/* endregion */

/* region Metadata */

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

/* region Address Manipulation */

/* Network layer address definitions and transport endpoint definitions are common to UDP and TCP.
 *
 * We define our own structures for inet and inet6 addresses to mantain the public API as system agnostic as possible
 * and avoid creating a dependency on non-standard headers for sockaddr, sockaddr_in, sockaddr_in6 and sockaddr_storage.
 *
 * Unfortunately, C99 does not support anonymous unions, so we have to use an additional name for those. Anonymous
 * bitfields are not supported either, so we have to rely on named members for padding.
 *
 * Structs/Unions declared just for structural purposes do not have typedefs because users should not normally have to
 * deal with them and if they ever do it is best they have to be explicit about it.
 */

/**
 *
 */
union clarinet_ipv4_octets
{
    uint8_t byte[4];
    uint16_t word[2];
    uint32_t dword;
};

/**
 *
 */
union clarinet_ipv6_octets
{
    uint8_t byte[16];
    uint16_t word[8];
    uint32_t dword[4];
};

/**
 *
 */
union clarinet_mac_octets
{
    uint8_t byte[8];    /**< Using 8 octets instead of 6 to keep a nice memory alignment of 4 bytes. The 2 most significant octets (index 0 and 1) must be 0. */
    uint16_t word[4];
    uint32_t dword[2];
};

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
    union clarinet_ipv4_octets u;
    uint32_t rffu_4 CLARINET_UNUSED;
};

/**
 * IPv6 address information (consider using clarinet_addr instead). This is also used to store information of 
 * IPv4MappedToIPv6 addresses.
 */
struct clarinet_ipv6
{
    uint32_t flowinfo;
    union clarinet_ipv6_octets u;
    uint32_t scope_id;
};

struct clarinet_mac
{
    uint32_t rffu_0 CLARINET_UNUSED;
    uint32_t rffu_1 CLARINET_UNUSED;
    uint32_t rffu_2 CLARINET_UNUSED;
    union clarinet_mac_octets u;
    uint32_t rffu_3 CLARINET_UNUSED;
};

/**
 * IP address representation.
 *
 * @details It can represent both IPv4 and IPv6 addresses. The member 'family' indicates which IP version is represented
 * and may contain any constant value defined with the prefix CLARINET_AF_. Note that an IPv4MappedToIPv6 is an IPv6
 * address that follows a specific format specified in RFC4291. clarinet_addr_is_ipv4mapped(addr) can be used to check
 * if an address is an IPv4MappedToIPv6 address.
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

struct clarinet_endpoint
{
    clarinet_addr addr;
    uint16_t port;
    uint16_t rffu CLARINET_UNUSED;
};

typedef struct clarinet_endpoint clarinet_endpoint;

CLARINET_EXTERN const clarinet_addr clarinet_addr_none;
CLARINET_EXTERN const clarinet_addr clarinet_addr_any_ipv4;
CLARINET_EXTERN const clarinet_addr clarinet_addr_any_ipv6;
CLARINET_EXTERN const clarinet_addr clarinet_addr_loopback_ipv4;
CLARINET_EXTERN const clarinet_addr clarinet_addr_loopback_ipv6;
CLARINET_EXTERN const clarinet_addr clarinet_addr_loopback_ipv4mapped;
CLARINET_EXTERN const clarinet_addr clarinet_addr_broadcast_ipv4;

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

#define clarinet_addr_is_any_ipv4(addr)           (clarinet_addr_is_ipv4(addr) && ((addr)->as.ipv4.u.dword == 0))

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
#define clarinet_addr_is_broadcast_ipv4(addr)     (clarinet_addr_is_ipv4(addr) && ((addr)->as.ipv4.u.dword == 0xFFFFFFFF))

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
 * Converts the IPv4MappedToIPv6 address pointed by src into an IPv4 address and copies it into the memory pointed by
 * dst. If src points to an IPv4 address then a simple copy is performed. On success returns CLARINET_ENONE. If either
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

/* region Initialization (from this point on all macros and functions require library initialization) */

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

struct clarinet_socket
{
    uint16_t family;            /**< Socket fammily (note that dual stack sockets must be INET6 by definition) */
    uint16_t proto;             /**< Socket protocol */
    union
    {
        int fd;                 /**< Used in platforms that identify sockets using descriptors (e.g. unix) */
        void* handle;           /**< Used in platforms that identify sockets using handles (e.g. windows) or opaque structures (e.g. embedded systems) */
    } u;
};

/**
 * Socket handle.
 *
 * @details Must be initialized using @c clarinet_socket_init() before it can be used. Socket handles are not movable.
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
 * Initialize a socket structure
 *
 * @param [in, out] sp
 *
 * @details The memory pointed to by @p sp must have been previously allocated by the user.
 *
 */
CLARINET_EXTERN
void
clarinet_socket_init(clarinet_socket* sp);

/** 
 * Create a new socket.
 *
 * @param [in, out] sp
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
 * Close the socket.
 *
 * @param [in] sp Pointer to a socket structure previously allocated.
 *
 * @return @c CLARINET_ENONE on success
 * @return @c CLARINET_EINVAL
 * @return @c CLARINET_EALREADY
 * @return @c CLARINET_EINTR
 * @return @c CLARINET_ENETDOWN
 * @return @c CLARINET_ELIBACC
 *
 * @details On success this function reinitializes the socket structure pointed to by @p sp. Otherwise an error code is
 * returned and the memory is left unmodified. It is then up to the the user to call @c clarinet_socket_init() should
 * the same socket be reused. Therefore it is only safe to call @c clarinet_socket_close() successvely with the same
 * socket if a previous closing attempt did not fail. There might be specific situations on certain platforms in which a
 * socket could be safely closed again after a previously failed attempt but in general, unless explicitly stated
 * otherwise, any closing operation is final and should not be re-tried in case of an error. Trying to close the same
 * socket twice like this may lead to a (reused) file descriptor in another thread being closed unintentionally. This
 * might occur because the system can release the descriptor associated with the socket early in a close operation,
 * freeing it for reuse by another thread. Still the steps that might produce an error, such as flushing data, could
 * occur much later in the closing process and eventually result in an error being returned.
 *
 * @note Behaviour is undefined if the socket pointed by sp is not initialized.
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
 * Get the local endpoint bound to the socket.
 *
 * @details This is the local address previously passed to clarinet_socket_bind() or implicitly bound by 
 * clarinet_socket_connect(). This is not necessarily the same address used by a remote peer to send this host a packet 
 * because the local host could be behind a NAT and it may not even match the destination address in a received IP 
 * packet because the host can be bound multiple addresses using a wildcard address.
 */
CLARINET_EXTERN
int
clarinet_socket_local_endpoint(clarinet_socket* restrict sp,
                               clarinet_endpoint* restrict local);

/** 
 * Get the remote endpoint connected to the socket.
 *
 * @details For TCP sockets, once clarinet_socket_connect() has been performed, either socket can call 
 * clarinet_socket_remote_endpoint() to obtain the address of the peer socket.  On the other hand, UDP sockets are 
 * connectionless. Calling clarinet_socket_connect() on a UDP socket merely sets the peer address for outgoing datagrams 
 * sent with clarinet_socket_send() or clarinet_socket_recv(). The caller of clarinet_socket_connect(2) can use 
 * clarinet_socket_remote_endpoint() to obtain the peer address that it earlier set for the socket.  However, the peer
 * socket is unaware of this information, and calling clarinet_socket_remote_endpoint() on the peer socket will return 
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
 * Set socket option.
 *
 * @details All socket options have a unique @p optname across all protocols. This is safer than allowing independent
 * numbering under each option level (i.e. protocol layer). The alternative of passing an <i>option level</i> as in @c
 * setsockopt(2) may lead to accidental modifications without warning when the user mistakes the protocol level and the
 * wrong level happens to have an option name with the same value
 *
 * @param [in] sp
 * @param [in] optname
 * @param [in] optval
 * @param [in] optlen
 *
 * @return
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
 * Get socket option.
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
 * Connects the socket to a remote host.
 *
 * @param sp
 * @param remote
 *
 * @return
 *
 */
CLARINET_EXTERN
int
clarinet_socket_connect(clarinet_socket* restrict sp,
                        clarinet_endpoint* restrict remote);

/**
 * Listen for connections on a socket.
 *
 * @param [in] sp Socket pointer
 * @param [in] backlog The maximum length of the queue of pending connections. Any value between 0 and 5 should work in
 * all platforms. Values greater than 5 may not be supported in platforms with severe resource constraints. A value of
 * -1 will use whatever is the platform's maximum backlog. Other negative values are undefined.
 *
 * @return
 *
 * @details This function will fail if the socket is not from a compatible protocol. Compatible protocols are:
 * @c CLARINET_PROTO_TCP. The value provided by @p backlog is just a hint and the system is free to select a different
 * one. Calling @c clarinet_socket_listen() again on a listen socket with possibly a different @p backlog value will
 * not fail but the outcome will be platform dependent and the call may as well be entirely ignored.
 *
 * @note On Windows backlog values are clamped to the interval [200, 65535] and -1 is equivalent to 65535. A subsequent
 * call to @c clarinet_socket_listen() on the listening socket will return success without changing the value for the
 * backlog parameter. Setting the backlog parameter to 0 in a subsequent call to listen on a listening socket is not
 * considered a proper reset, especially if there are connections on the socket.
 *
 * @note On Linux the behavior of the backlog argument on TCP sockets changed with kernel 2.2.  Now it specifies the
 * queue length for completely established sockets waiting to be accepted, instead of the number of incomplete
 * connection requests.  The maximum length of the queue for incomplete sockets can be set using
 * /proc/sys/net/ipv4/tcp_max_syn_backlog.  When syncookies are enabled there is no logical maximum length and this
 * setting is ignored.  See tcp(7) for more information. If the backlog argument is greater than the value in
 * /proc/sys/net/core/somaxconn, then it is silently capped to that value.  Since Linux 5.4, the default in this file is
 * 4096; in earlier kernels, the default value is 128.  In kernels before 2.4.25, this limit was a hard coded value,
 * @c SOMAXCONN, with the value 128. A subsequent call to @c clarinet_socket_listen() on the listening socket will
 * adjust its backlog queue length using the new backlog value, but only for future connection requests. It does not
 * discard any pending connections already in the queue.
 *
 * @note On FreeBSD/Darwin the real maximum queue length will be 1.5 times more than the value specified in the backlog
 * argument. A subsequent call to @c clarinet_socket_listen() on the listening socket allows the caller to change the
 * maximum queue length using a new backlog argument.  If a connection request arrives with the queue full the client
 * may receive an error with an	indication of @c ECONNREFUSED, or,	in the case of TCP, the	connection will be silently
 * dropped. Current queue lengths of listening	sockets	can be queried using netstat(1)	command. Note that before
 * FreeBSD 4.5 and the introduction of the syncache, the backlog argument also determined the length of the	incomplete
 * connection queue, which held TCP sockets in the process of completing TCP's 3-way handshake. These incomplete
 * connections are now held entirely in the syncache, which is unaffected by queue lengths. Inflated backlog values to
 * help handle denial of service attacks are no longer necessary. The sysctl(3) MIB variable kern.ipc.soacceptqueue
 * specifies a hard	limit on backlog; if a value greater than kern.ipc.soacceptqueue or less than zero is specified,
 * backlog is silently forced to kern.ipc.soacceptqueue. If	the listen queue overflows, the	kernel will emit a
 * @c LOG_DEBUG syslog message. The sysctl(3) MIB variable kern.ipc.sooverinterval specifies a per-socket limit on how
 * often the kernel will emit	these messages.
 *
 */
CLARINET_EXTERN
int
clarinet_socket_listen(clarinet_socket* sp,
                       int backlog);

CLARINET_EXTERN
int
clarinet_socket_accept(clarinet_socket* restrict server,
                       clarinet_socket* restrict client,
                       clarinet_endpoint* restrict remote);

CLARINET_EXTERN
int
clarinet_socket_shutdown(clarinet_socket* restrict sp,
                         int flags);

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
