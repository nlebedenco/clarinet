#pragma once
#ifndef PORTABILITY_H
#define	PORTABILITY_H

#include <stdarg.h>
#include "config.h"

#ifdef HAVE_STRLCAT
    #define pcap_strlcat	strlcat
#else
  #if defined(_MSC_VER) || defined(__MINGW32__)
    /*
     * strncat_s() is supported at least back to Visual
     * Studio 2005; we require Visual Studio 2015 or later.
     */
    #define pcap_strlcat(x, y, z) \
	strncat_s((x), (z), (y), _TRUNCATE)
  #else
    /*
     * Define it ourselves.
     */
    extern size_t pcap_strlcat(char * restrict dst, const char * restrict src, size_t dstsize);
  #endif
#endif

#ifdef HAVE_STRLCPY
  #define pcap_strlcpy	strlcpy
#else
  #if defined(_MSC_VER) || defined(__MINGW32__)
    /*
     * strncpy_s() is supported at least back to Visual
     * Studio 2005; we require Visual Studio 2015 or later.
     */
    #define pcap_strlcpy(x, y, z) \
	strncpy_s((x), (z), (y), _TRUNCATE)
  #else
    /*
     * Define it ourselves.
     */
    extern size_t pcap_strlcpy(char * restrict dst, const char * restrict src, size_t dstsize);
  #endif
#endif

#ifdef _MSC_VER
  /*
   * If <crtdbg.h> has been included, and _DEBUG is defined, and
   * __STDC__ is zero, <crtdbg.h> will define strdup() to call
   * _strdup_dbg().  So if it's already defined, don't redefine
   * it.
   */
  #ifndef strdup
  #define strdup	_strdup
  #endif
#endif

/*
 * We want asprintf(), for some cases where we use it to construct
 * dynamically-allocated variable-length strings; it's present on
 * some, but not all, platforms.
 */
#ifdef HAVE_ASPRINTF
#define pcap_asprintf asprintf
#else
extern int pcap_asprintf(char **, PCAP_FORMAT_STRING(const char *), ...)
    PCAP_PRINTFLIKE(2, 3);
#endif

#ifdef HAVE_VASPRINTF
#define pcap_vasprintf vasprintf
#else
extern int pcap_vasprintf(char **, const char *, va_list ap);
#endif

/* For Solaris before 11. */
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

#ifdef HAVE_STRTOK_R
  #define pcap_strtok_r	strtok_r
#else
  #ifdef _WIN32
    /*
     * Microsoft gives it a different name.
     */
    #define pcap_strtok_r	strtok_s
  #else
    /*
     * Define it ourselves.
     */
    extern char *pcap_strtok_r(char *, const char *, char **);
  #endif
#endif /* HAVE_STRTOK_R */

#ifdef _WIN32
  #if !defined(__cplusplus)
    #define inline __inline
  #endif
#endif /* _WIN32 */

#endif // PORTABILITY_H
