#pragma once
#ifndef PLATFORMS_WINDOWS_ADDR_H
#define PLATFORMS_WINDOWS_ADDR_H

#include "portability.h"
#include "clarinet/clarinet.h"

#include <winsock2.h>

int 
clarinet_endpoint_to_sockaddr(struct sockaddr* CLARINET_RESTRICT dst,
                              size_t dstlen,
                              const clarinet_endpoint* CLARINET_RESTRICT src);

int 
clarinet_endpoint_from_sockaddr(clarinet_endpoint* CLARINET_RESTRICT dst,
                                const struct sockaddr* CLARINET_RESTRICT src
                                size_t srclen);


#endif /* PLATFORMS_WINDOWS_ADDR_H */
