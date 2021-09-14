#pragma once
#ifndef PLATFORMS_UNIX_SYS_H
#define PLATFORMS_UNIX_SYS_H

#include "portability.h"
#include "clarinet/clarinet.h"

#include <sys/ioctl.h>

struct clarinet_socket
{
    int handle;
};

typedef struct clarinet_socket clarinet_socket;

/** 
 * Converts a platform error code to clarinet_error. Not all possible system errors are necessarily covered, only 
 * those expected to reach the end-user. 
 */
int 
clarinet_error_from_errno(const int err);

/** Converts a clarinet proto/option to their corresponding platform sockopt level/name. */
int 
clarinet_socket_option_to_sockopt(int* restrict optlevel,
                                  int* restrict optname,
                                  const int proto, 
                                  const int option);

/** Close socket respecting the linger option. If the socket is successully closed the optional callback function
 * pointed by destructor will be executed before the struct memory is freed.  */
int
clarinet_socket_close(clarinet_socket** spp,
                      void(*destructor)(clarinet_socket* sp));

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
CLARINET_INLINE
int 
setsocknonblock(int sock)
{
    /* fnctl with O_NONBLOCK is the preferred way but iotcl offers an interface analagous to that on windows AND a single sys call
     * instead of the two (read-modify-write) required by fnctl */
    int mode = 1;
    return ioctl(sock, FIONBIO, &mode);  
}
                     
#endif /* PLATFORMS_UNIX_SYS_H */
