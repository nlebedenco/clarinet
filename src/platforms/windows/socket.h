#pragma once
#ifndef PLATFORMS_WINDOWS_SYS_H
#define PLATFORMS_WINDOWS_SYS_H

#include "compat/compat.h"
#include "clarinet/clarinet.h"

/* region Socket */

/**
 * A socket structure that we can easily expand or invalidate.
 *
 * Initially the idea was to handle socket identifiers without the need for any complex mapping to the underlying socket
 * descriptor used by the platform. Under most Unix systems, sockets are part of the standard C library, no additional
 * dependencies are required and file descriptors (integers) are used as identifiers. On Windows we have to use WinSock
 * to which socket descriptors are an opaque type (usually normal NT kernel object handles). This means that instead of
 * type int, sockets need to be of type SOCKET and the classic comparison of < 0 to detect error conditions or a bad
 * descriptor is no longer valid - one has to use the INVALID_SOCKET constant instead. We worked around all this by
 * using an opaque pointer to represent the socket which also serves to keep a reference to internal state. It's worth
 * noting though that while winsock sockets are supposed to be interchangeable with instances of HANDLE in many windows
 * APIs, Microsoft chose to use a different representation so casting is necessary when sockets are used with normal
 * Win32  function calls. Unfortunately, this dependency on SOCKET translates to a dependency on <winsock2.h> which
 * everyone actively tries to avoid. According to the documentation at
 * https://docs.microsoft.com/en-us/windows/win32/winsock/socket-data-type-2 no assumption can be made about the range
 * of SOCKET other than that it is a value in the rance [0, INVALID_SOCKET-1]. On the other hand, Microsoft guarantees a
 * stable ABI so one could take advantage of that and rely on the underlying type used for handles (UINT_PTR) which
 * matches the more standard uintptr_t from <stdint.h>. Furthermore, according to this article
 * https://docs.microsoft.com/en-us/windows/win32/sysinfo/kernel-objects windows has a per-process limit on kernel
 * handles of 2^24 which makes it relatively safe to cast to intptr_t or even int under the normal assumption that
 * sizeof(int) >= sizeof(intptr_t). The fact that INVALID_SOCKET has always been defined as (~0) seems to support the
 * notion that casting from SOCKET to int and back should not cause loss of data. All this will make purists whimper but
 * looking at the big picture Microsoft could have simply used int from the very beginning.
 */
struct cl_socket
{
    SOCKET handle;
};

typedef struct cl_socket cl_socket;

/**
 * Close socket respecting the linger option. If the socket is successully closed the optional callback function
 * pointed by destructor will be executed before the struct memory is freed.
 * */
int
cl_socket_close(cl_socket** spp,
                void(* destructor)(cl_socket* sp));

int
cl_socket_send(cl_socket* restrict sp,
               const void* restrict buf,
               size_t buflen,
               const cl_endpoint* restrict dst);

int
cl_socket_recv(cl_socket* restrict sp,
               void* restrict buf,
               size_t buflen,
               cl_endpoint* restrict src);

int
cl_socket_get_endpoint(cl_socket* restrict sp,
                       cl_endpoint* restrict endpoint);

/* endregion */

/* region Helpers */

/** Helper for setting the socket as non-blocking */
int
setnonblock(SOCKET sock);

/**
 * Helper for setting the socket to not reset when receiving an ICMP Type 3 Code 3 Destination Port Unreachable
 * This is needed because a single UDP socket could be sending data to multiple remote hosts.
 * -- https://docs.microsoft.com/en-us/windows/win32/winsock/winsock-ioctls
 */
int
setnonreset(SOCKET sock);


/**
 * Helper for setting a socket option and return WSAEOPNOTSUPP instead of WSAEINVAL if optval or optlen are invalid.
 * This is used so that the converted cl_error code (CL_ESYS instead of CL_EINVAL) makes sense to the end user if a
 * setsockopt(2) fails in the context of a larger operation. Otherwise the user would interpret the error as being
 * an invalid argument passed to the larger operation.
 */
int
initsockopt(SOCKET sock,
            int level,
            int optname,
            const char* optval,
            int optlen);

/**
 * Helper for ignoring WSAENOPROTOOPT.
 * This is used so that a call to setsockopt(2) that fails in the context of a larger operation does not compromise the
 * entire operation.
 */
int
trysockopt(SOCKET sock,
           int level,
           int optname,
           const char* optval,
           int optlen);

/* endregion */

#endif /* PLATFORMS_WINDOWS_SYS_H */
