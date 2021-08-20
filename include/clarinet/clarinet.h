#pragma once
#ifndef CLARINET_H
#define CLARINET_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * Utility macros
 */
#define CLARINET_STR(s) #s
#define CLARINET_XSTR(s) CLARINET_STR(s)

/**
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
 */
#ifdef CLARINET_STATIC
    #define CLARINET_VISIBLITY
#else 
    #ifdef _WIN32
        #ifdef CLARINET_EXPORT
            #define CLARINET_VISIBLITY     __declspec(dllexport)
        #else
            #define CLARINET_VISIBLITY     __declspec(dllimport)
        #endif
    #else /* UN*X */
        #ifdef CLARINET_EXPORT
            #if CLARINET_IS_AT_LEAST_GNUC_VERSION(3,4)
                /*
                * GCC 3.4 and later (or some compiler asserting compatibility with
                * GCC 3.4 and later) so we have __attribute__((visibility()).
                */
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

/** 
 * Constants
 */

enum clarinet_errcode
{
    CLARINET_ENONE = 0,             /* Success */ 
    CLARINET_EFAIL = -1,            /* Operation failed (unspecified error) */
    CLARINET_ESHUTDOWN = -2,        /* Not initialized */
    CLARINET_EALREADY = -3,         /* Operation already in progress */
    CLARINET_EINVAL = -4,           /* Invalid Argument */
    CLARINET_ENOTSUP = -5,          /* Operation is not supported */
    CLARINET_EPERM = -6,            /* Operation is not permitted */
    CLARINET_EPROTO = -7,           /* Protocol error */
    CLARINET_EPROTONOSUPPORT = -8,  /* Protocol not supported */
    CLARINET_ETIME = -9,            /* Timeout */
    CLARINET_EOVERFLOW = -10,       /* Buffer overflow */
};

typedef enum clarinet_errcode clarinet_errcode;

enum clarinet_proto
{
    CLARINET_PROTO_NONE = 0,        /* None */
    CLARINET_PROTO_TCP = 1,         /* Transmission Control Protocol (RFC793) */
    CLARINET_PROTO_UDP = 2,         /* User Datagram Protocol (RFC768) */
    CLARINET_PROTO_TLS = 3,         /* Transport Layer Security (RFC8446) */
    CLARINET_PROTO_DTLS = 4,        /* Datagram Transport Layer Security (RFC6347) */
    CLARINET_PROTO_DTLC = 5,        /* Datagram Transport Layer Connectivity (Custom) */
    CLARINET_PROTO_UDTP = 6,        /* User Datagram Transmission Protocol (Custom) */
    CLARINET_PROTO_UDTPS = 7,       /* User Datagram Transmission Protocol Secure (Custom) */
    CLARINET_PROTO_ENET = 8,        /* ENet (http://enet.bespin.org/index.html) */
    CLARINET_PROTO_ENETS = 9,       /* ENet Secure (Custom) */
};

typedef enum clarinet_proto clarinet_proto;


/** 
 * Data Structures 
 *
 * In POSIX, names ending with _t are reserved. Since we're targeting at least one POSIX system (i.e. Linux) typenames 
 * defined here MUST NOT end with _t.
 */



/**
 * Details Interface
 */
 
CLARINET_API    uint32_t clarinet_getlibnver(void);
CLARINET_API const char* clarinet_getlibsver(void);
CLARINET_API const char* clarinet_getlibname(void);
CLARINET_API const char* clarinet_getlibdesc(void);

/**
 * Control Interface
 */
 
CLARINET_API clarinet_errcode clarinet_initialize(void);
CLARINET_API clarinet_errcode clarinet_finalize(void);


#ifdef __cplusplus
}
#endif
#endif // CLARINET_H