#pragma once
#ifndef PLATFORMS_WINDOWS_ADDR_H
#define PLATFORMS_WINDOWS_ADDR_H

#include "portability.h"
#include "clarinet/clarinet.h"

#include <winsock2.h>
#include <ws2tcpip.h>

/* TODO:
    - convert struct sockaddr to/from clarinet_endpoint
    - convert struct sockaddr_in to/from clarinet_endpoint
    - convert struct sockaddr_in6 to/from clarinet_endpoint
    - convert in_addr to/from clarinet_addr_ipv4
    - convert in6_addr to/from clarinet_addr_ipv6
*/


#endif /* PLATFORMS_WINDOWS_ADDR_H */
