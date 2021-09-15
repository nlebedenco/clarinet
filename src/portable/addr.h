#pragma once
#ifndef PORTABLE_ADDR_H
#define PORTABLE_ADDR_H

#include "portability.h"
#include "clarinet/clarinet.h"

/**
 * Convert and endpoint to a sockaddr_storage that can later be cast to sockaddr_in or sockaddr_in6. Some systems are more picky about the size of the sockaddr struct you pass
 * to methods such as bind(2) and sendto(2) (e.g. macOS) and will not accept sizeof(struct sockaddr_storage) despite ss_family being all that is needed to imply size. As a convenience
 * if  dstlen is not null it will be assigned the size of the corresponding sockaddr struct used.
 */
int 
clarinet_endpoint_to_sockaddr(struct sockaddr_storage* restrict dst,
                              socklen_t* restrict dstlen, /* optional */
                              const clarinet_endpoint* restrict src);

int 
clarinet_endpoint_from_sockaddr(clarinet_endpoint* restrict dst,
                                const struct sockaddr_storage* restrict src);

#endif /* PORTABLE_ADDR_H */
