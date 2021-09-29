#pragma once
#ifndef PLATFORMS_UNIX_SYS_H
#define PLATFORMS_UNIX_SYS_H

#include "portability.h"
#include "clarinet/clarinet.h"

#include <errno.h>
#include <sys/ioctl.h>

struct clarinet_socket
{
    int handle;
};

typedef struct clarinet_socket clarinet_socket;

/** Return 1 if error code is EWOULDBLOCK or EAGAIN, otherwise 0. */
CLARINET_INLINE
int
clarinet_should_retry(int e)
{
#if HAVE_EAGAIN_SAME_AS_EWOULDBLOCK
    return ((e) == EWOULDBLOCK);
#else
    return (((e) == EWOULDBLOCK) || ((e) == EAGAIN));
#endif
}

/** 
 * Converts a platform error code to clarinet_error. Not all possible system errors are necessarily covered, only 
 * those expected to reach the end-user. 
 */
int
clarinet_error_from_errno(int err);

/** Converts a clarinet proto/option to their corresponding platform sockopt level/name. */
int
clarinet_socket_option_to_sockopt(int* restrict optlevel,
                                  int* restrict optname,
                                  int proto,
                                  int option);

/** Close socket respecting the linger option. If the socket is successully closed the optional callback function
 * pointed by destructor will be executed before the struct memory is freed.  */
int
clarinet_socket_close(clarinet_socket** spp,
                      void(* destructor)(clarinet_socket* sp));

int
clarinet_socket_get_endpoint(clarinet_socket* restrict sp,
                             clarinet_endpoint* restrict endpoint);

int
clarinet_socket_send(clarinet_socket* restrict sp,
                     const void* restrict buf,
                     size_t len,
                     const clarinet_endpoint* restrict dst);

int
clarinet_socket_recv(clarinet_socket* restrict sp,
                     void* restrict buf,
                     size_t len,
                     clarinet_endpoint* restrict src);

/** Helper for setting the socket as non-blocking */
int
setnonblock(int sock);

/** Helper for setting a socket option and return EOPNOTSUPP instead of EINVAL */
int
setsockoptdep(int sockfd,
              int level,
              int optname,
              const void* optval,
              socklen_t optlen);


#endif /* PLATFORMS_UNIX_SYS_H */
