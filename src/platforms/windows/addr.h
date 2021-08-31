#pragma once
#ifndef PLATFORMS_WINDOWS_ADDR_H
#define PLATFORMS_WINDOWS_ADDR_H

#include "portability.h"

/* winsock2.h and windows.h define some STATUS codes but not all. The most obvious ones are missing like STATUS_SUCCESS. 
 * So we have to define WIN32_NO_STATUS before including winsock2.h/windows.h to avoid redefinition errors from 
 * ntstatus.h. 
 */
#define WIN32_NO_STATUS
#include <winsock2.h>
#include <windows.h>
#undef WIN32_NO_STATUS
#include <ntstatus.h>

int 
clarinet_endpoint_to_sockaddr(struct sockaddr* CLARINET_RESTRICT dst,
                              size_t dstlen,
                              const clarinet_endpoint* CLARINET_RESTRICT src);

int 
clarinet_endpoint_from_sockaddr(clarinet_endpoint* CLARINET_RESTRICT dst,
                                const struct sockaddr* CLARINET_RESTRICT src,
                                size_t srclen);


#endif /* PLATFORMS_WINDOWS_ADDR_H */
