#pragma once
#ifndef PORTABILITY_H
#define	PORTABILITY_H

/***********************************************************************************************************************
 * Configuration and portability definitions.
 *
 * This must be the very first header included in a src file (direct or indirectly).
 * Defines __GNU_SOURCE if any GNU function is known to be available.
 ***********************************************************************************************************************/

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#if defined(HAVE_GNU_ASPRINTF) || defined(HAVE_GNU_VASPRINTF) 
#define __GNU_SOURCE
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

#if !defined(HAVE_STRLCAT)
    #if defined(_MSC_VER) || defined(__MINGW32__)
        /* strncat_s() is supported at least since Visual Studio 2005 and we require Visual Studio 2015 or later. */
        #define strlcat(x, y, z) 	                    strncat_s((x), (z), (y), _TRUNCATE)
    #else
        size_t strlcat(char* CLARINET_RESTRICT dst, 
                       const char* CLARINET_RESTRICT src, 
                       size_t dstsize);
    #endif
#endif /* HAVE_STRLCAT */

#if !defined(HAVE_STRLCPY)
    #if defined(_MSC_VER) || defined(__MINGW32__)
        /* strncpy_s() is supported at least since Visual Studio 2005 and we require Visual Studio 2015 or later. */
        #define strlcpy(x, y, z)                        strncpy_s((x), (z), (y), _TRUNCATE)
    #else
        size_t strlcpy(char* CLARINET_RESTRICT dst, 
                       const char* CLARINET_RESTRICT src, 
                       size_t dstsize);
    #endif
#endif /* HAVE_STRLCPY */

#if !defined(HAVE_STRTOK_R)
    #if defined(_WIN32)
        /* Microsoft gives it a different name. */
        #define strtok_r(str, delim, saveptr)	        strtok_s(str, delim, saveptr)
    #else
        char* strtok_r(char *str, const char *delim, char **saveptr);
    #endif
#endif /* HAVE_STRTOK_R */

#if defined(_MSC_VER)
    /* If <crtdbg.h> has been included, and _DEBUG is defined, and __STDC__ is zero, <crtdbg.h> will define strdup() to
     * call _strdup_dbg().  So if it's already defined, don't redefine it. */
    #if !defined(strdup)
        #define strdup(s)                               _strdup(s)
    #endif
#endif

/* We want asprintf(), for some cases where we use it to construct dynamically-allocated variable-length strings. It's 
 * present on some, but not all, platforms. */
#if !defined(HAVE_GNU_ASPRINTF)
    int asprintf(char** CLARINET_RESTRICT strp, 
                 const char* CLARINET_RESTRICT fmt, ...);
#endif

#if !defined(HAVE_GNU_VASPRINTF)
    int vasprintf(char** strp CLARINET_RESTRICT, 
                  const char* fmt CLARINET_RESTRICT, 
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


/* Define a portable thread-safe strerror */
#if defined(HAVE_STRERROR_S)
    #define strerror_c(buf, buflen, errnum)             strerror_s(buf, buflen, errnum)
#elif defined(HAVE_STRERROR_R) && defined(HAVE_POSIX_STRERROR_R)        
    #define strerror_c(buf, buflen, errnum)             strerror_r(errnum, buf, buflen)
#elif defined(HAVE_STRERROR_R) && defined(HAVE_GNU_STRERROR_R)
    int strerror_c(char* buf, size_t buflen, int errnum);       
#else
    #define strerror_c(buf, buflen, errnum)             (ENOSYS)
#endif


#endif // PORTABILITY_H
