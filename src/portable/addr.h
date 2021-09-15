#pragma once
#ifndef PORTABLE_ADDR_H
#define PORTABLE_ADDR_H

#include "portability.h"
#include "clarinet/clarinet.h"

int 
clarinet_endpoint_to_sockaddr(struct sockaddr_storage* restrict dst,
                              const clarinet_endpoint* restrict src);

int 
clarinet_endpoint_from_sockaddr(clarinet_endpoint* restrict dst,
                                const struct sockaddr_storage* restrict src);

#endif /* PORTABLE_ADDR_H */
