#pragma once
#ifndef PORTABILITY_H
#define	PORTABILITY_H

/***********************************************************************************************************************
 * Configuration and portability definitions.
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
 ***********************************************************************************************************************/

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#if defined(HAVE_GNU_ASPRINTF) || defined(HAVE_GNU_VASPRINTF) 
#define _GNU_SOURCE
#endif

/* Intellisense doesn't like the restrict keyword. */
#ifdef __INTELLISENSE__
    #define restrict 
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

#include <stdarg.h>

#if defined(HAVE_STRLCAT)
    #if defined(HAVE_STRLCAT_IN_BSD_STRING_H)
        #include <bsd/string.h>
    #endif
#else
    #if defined(_MSC_VER) || defined(__MINGW32__)
        #define strlcat(dst, src, dstsize) 	            strncat_s((dst), (dstsize), (src), _TRUNCATE)
    #else
        size_t strlcat(char* restrict dst, 
                       const char* restrict src, 
                       size_t dstsize);
    #endif
#endif /* HAVE_STRLCAT */

#if defined(HAVE_STRLCPY)
    #if defined(HAVE_STRLCPY_IN_BSD_STRING_H)
        #include <bsd/string.h>
    #endif
#else
    #if defined(_MSC_VER) || defined(__MINGW32__)
        #define strlcpy(dst, src, dstsize)              strncpy_s((dst), (dstsize), (src), _TRUNCATE)
    #else
        size_t strlcpy(char* restrict dst, 
                       const char* restrict src, 
                       size_t dstsize);
    #endif
#endif /* HAVE_STRLCPY */

#if !defined(HAVE_STRTOK_R)
    #if defined(_WIN32)
        #define strtok_r(str, delim, saveptr)	        strtok_s(str, delim, saveptr)
    #else
        char* strtok_r(char *str, const char *delim, char **saveptr);
    #endif
#endif /* HAVE_STRTOK_R */

/* If <crtdbg.h> has been included, and _DEBUG is defined, and __STDC__ is zero, <crtdbg.h> will define strdup() to
 * call _strdup_dbg().  So if it's already defined, don't redefine it. 
 */
#if defined(_MSC_VER)
    #if !defined(strdup)
        #define strdup(s)                               _strdup(s)
    #endif
#endif

/* We want asprintf(), for some cases where we use it to construct dynamically-allocated variable-length strings. It's 
 * present on some, but not all, platforms. */
#if !defined(HAVE_GNU_ASPRINTF)
    int asprintf(char** restrict strp, 
                 const char* restrict fmt, ...);
#endif

#if !defined(HAVE_GNU_VASPRINTF)
    int vasprintf(char** restrict strp, 
                  const char* restrict fmt, 
                  va_list ap);
#endif

/* Fallbacks for un*x systems that don't define timer macros */
#if !defined(_WIN32)
    #include <sys/time.h>
    #ifndef timeradd
        #define timeradd(a, b, result)                       \
          do {                                               \
            (result)->tv_sec = (a)->tv_sec + (b)->tv_sec;    \
            (result)->tv_usec = (a)->tv_usec + (b)->tv_usec; \
            if ((result)->tv_usec >= 1000000) {              \
              ++(result)->tv_sec;                            \
              (result)->tv_usec -= 1000000;                  \
            }                                                \
          } while (0)
    #endif /* timeradd */

    #ifndef timersub
        #define timersub(a, b, result)                       \
          do {                                               \
            (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;    \
            (result)->tv_usec = (a)->tv_usec - (b)->tv_usec; \
            if ((result)->tv_usec < 0) {                     \
              --(result)->tv_sec;                            \
              (result)->tv_usec += 1000000;                  \
            }                                                \
          } while (0)
    #endif /* timersub */
#endif /* __WIN32 */


/* Define strerror_s if not available */
#if !defined(HAVE_STRERROR_S)
    #if defined(HAVE_STRERROR_R) && defined(HAVE_POSIX_STRERROR_R)        
        #define strerror_s(buf, buflen, errnum)         strerror_r(errnum, buf, buflen)
    #elif defined(HAVE_STRERROR_R) && defined(HAVE_GNU_STRERROR_R)
        #include <stddef.h>
        int strerror_s(char* buf, size_t buflen, int errnum);       
    #else /* if there is no safe alternative for strerror_s define as if it returned an error itself */
        #include <errno.h>
        #define strerror_c(buf, buflen, errnum)         (ENOSYS)
    #endif    
#endif

#if defined(HAVE_FFS)
    #if defined(HAVE_FFS_IN_STRINGS_H)
        #include "strings.h"
    #endif
#else 
    int ffs(int i);
#endif

/* Include stdlib.h to check for min/max as this is normally where they are defined. */
#include <stdlib.h>

/* Define max if not defined. */
#ifndef max
#define max(a,b)                                        (((a) (b)) ? (a) : (b))
#endif

/* Define min if not defined. */
#ifndef min
#define min(a,b)                                        (((a) < (b)) ? (a) : (b))
#endif

#endif // PORTABILITY_H
