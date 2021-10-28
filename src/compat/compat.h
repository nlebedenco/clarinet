/***********************************************************************************************************************
 * Configuration and compatibility definitions.
 *
 * This must be the very first header included in any private src file (direct or indirectly).
 * Defines _GNU_SOURCE if any GNU function is known to be available.
 *
 * Note that _WIN32 is known to be defined by Clang/LLVM(Windows target) 32/64-bit, Clang/LLVM(MinGW target) 32/64-bit,
 * GNU GCC/G++(Windows target) 32-bit, GNU GCC/G++(MinGW target) 32-bit, Intel ICC/ICPC 32/64-bit, Portland PGCC/PGCPP
 * 32/64-bit and Microsoft Visual Studio 32/64-bit. Oddly enough, GCC under Cygwin predefines UNIX macros even when
 * building Windows applications. Some on-line advice recommends checking for _MSC_VER. The macro is defined with the
 * compiler version number for Clang/LLVM, ICC, and Visual Studio, but it isn't defined by GCC or Portland Group
 * compilers.
 *
 ***********************************************************************************************************************/
/* @formatter:off */
#pragma once
#ifndef COMPAT_H
#define COMPAT_H

#if HAVE_CONFIG_H
    #include "config.h"
#endif

/* Use UNICODE variants of the WINAPI. This is important because the ANSI version of some functions have limitations.
 * For example, GetAddrInfoExW can perform asynchronously but GetAddrInfoExA cannot.
 */
#if defined(_WIN32)
    #define UNICODE
    #define _UNICODE
#endif

/* Intellisense doesn't like the restrict keyword */
#ifdef __INTELLISENSE__
    #ifndef restrict
        #define restrict
    #endif
#endif

/* Define a macro to force inline for internal use. */
#if defined(_MSC_VER)
    #define CLARINET_INLINE __forceinline
#elif defined(__GNUC__)
    #define CLARINET_INLINE inline __attribute__((__always_inline__))
#elif defined(__CLANG__)
    #if __has_attribute(__always_inline__)
        #define CLARINET_INLINE inline __attribute__((__always_inline__))
    #else
        #define CLARINET_INLINE inline
    #endif
#else
    #define CLARINET_INLINE inline
#endif

#define CLARINET_STATIC_INLINE static CLARINET_INLINE

/* Define a macro to no return for internal use. */
#if defined(HAVE_STDRETURN_H)
    #include <stdnoreturn.h>
    #define CLARINET_NORETURN noreturn
#else
    #if defined(_MSC_VER)
        #define CLARINET_NORETURN __declspec(noreturn)
    #elif defined(__GNUC__)
        #define CLARINET_NORETURN __attribute__((noreturn))
    #elif defined(__CLANG__)
        #if __has_attribute(__noreturn__)
            #define CLARINET_NORETURN __attribute__((__noreturn__))
        #else
            #define CLARINET_NORETURN
        #endif
    #else
        #define CLARINET_NORETURN
    #endif
#endif

/**
 * Macro to explicitly declare unused parameters inside functions.
 * Unfortunately, MSVC does not appear to have an equivalent to GCC's "__attribute__((unused))" to mark a particular
 * function parameter as being known to be unused, so that the compiler won't warn about it (for example, the
 * function might have that parameter because a pointer to it is being used, and the signature of that function
 * includes that parameter). C++ lets you give a parameter a type but no name, but C doesn't have that.
 */
#define CLARINET_IGNORE_PARAM(x) (void)(x)

#include <stdarg.h>
#include <stddef.h>

/* Fallbacks for un*x systems that don't define timer macros */
#if !defined(_WIN32)

    #include <sys/time.h>

    #ifndef timeradd
        #define timeradd(a, b, result) do {                  \
            (result)->tv_sec = (a)->tv_sec + (b)->tv_sec;    \
            (result)->tv_usec = (a)->tv_usec + (b)->tv_usec; \
            if ((result)->tv_usec >= 1000000) {              \
              ++(result)->tv_sec;                            \
              (result)->tv_usec -= 1000000;                  \
            }                                                \
          } while (0)
    #endif /* timeradd */

    #ifndef timersub
        #define timersub(a, b, result) do {                  \
            (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;    \
            (result)->tv_usec = (a)->tv_usec - (b)->tv_usec; \
            if ((result)->tv_usec < 0) {                     \
              --(result)->tv_sec;                            \
              (result)->tv_usec += 1000000;                  \
            }                                                \
          } while (0)
    #endif /* timersub */

    /** Returns the time in milliseconds represented the struct timeval pointed ot by @p tv */
    #define tv2ms(t)  ((t)->tv_sec * 1000 + ((t)->tv_usec + 500)/ 1000)

    /** Initializes the struct timeval pointed to by @p t to the time @p m in milliseconds */
    #define ms2tv(t, m) do { \
        (t)->tv_sec = m / 1000; \
        (t)->tv_usec = (m % 1000) * 1000; \
    } while (0)

#endif /* __WIN32 */

#if HAVE_FFS
    #if HAVE_FFS_IN_STRINGS_H
        #include "strings.h"
    #endif
#else
    int
    ffs(int i);
#endif

/* Include stdlib.h to check for min/max as this is normally where they are defined. */
#include <stdlib.h>

/* Define max if not defined. */
#ifndef max
    #define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

/* Define min if not defined. */
#ifndef min
    #define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef clamp
    #define clamp(v, a, b) ((a) > (v) ? (a) : (b) < (v) ? (b) : (v))
#endif

/* Include socket headers. */
#if defined(_WIN32)
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>

    #if(_WIN32_WINNT < 0x0600)
        typedef short sa_family_t;
    #else
        typedef ADDRESS_FAMILY sa_family_t;
    #endif
#else /* BSD, Linux, macOS, iOS, Android, PS4, PS5 */
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netdb.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <errno.h>

    #ifndef INVALID_SOCKET
        #define INVALID_SOCKET (~0)
    #endif
    #ifndef SOCKET_ERROR
        #define SOCKET_ERROR (-1)
    #endif
#endif

/* Fallback socklen_t */
#if !HAVE_SOCKLEN_T
    typedef size_t socklen_t;
#endif

/* Fallback sockaddr_storage struct */
#if !HAVE_STRUCT_SOCKADDR_STORAGE
struct sockaddr_storage
{   
    union 
    {
        struct 
        {
            #if HAVE_STRUCT_SOCKADDR_SA_LEN
            uint8_t ss_len;
            #endif

            sa_family_t ss_family;
        };

        #if HAVE_SOCKADDR_IN6_SIN6_ADDR
        struct sockaddr_in6 __ss_padding;
        #else
        struct sockaddr_in __ss_padding;
        #endif /* HAVE_SOCKADDR_IN6_SIN6_ADDR */
    };
};
#endif /* !HAVE_STRUCT_SOCKADDR_STORAGE */

#endif /* COMPAT_H */

/* @formatter:on */
