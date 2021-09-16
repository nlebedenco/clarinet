#pragma once
#ifndef PLATFORMS_WINDOWS_SYS_H
#define PLATFORMS_WINDOWS_SYS_H

#include "portability.h"
#include "clarinet/clarinet.h"

struct clarinet_socket
{
    SOCKET handle;
};

typedef struct clarinet_socket clarinet_socket;

/** 
 * Converts a platform error code to clarinet_error. Not all possible system errors are necessarily covered, only 
 * those expected to reach the end-user. 
 */
int 
clarinet_error_from_wsaerr(const int err);

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
                     size_t buflen,
                     const clarinet_endpoint* restrict dst);

int
clarinet_socket_recv(clarinet_socket* restrict sp,
                     void* restrict buf,
                     size_t buflen,
                     clarinet_endpoint* restrict src);

/** Helper for setting the socket as non-blocking */
CLARINET_INLINE
int 
setsocknonblock(SOCKET sock)
{
    ULONG mode = 1;
    return ioctlsocket(sock, FIONBIO, &mode);  
}
                     
#endif /* PLATFORMS_WINDOWS_SYS_H */
