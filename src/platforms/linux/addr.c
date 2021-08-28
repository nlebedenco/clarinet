#include "platforms/linux/addr.h"

#include <stdlib.h>

int
clarinet_endpoint_to_sockaddr(struct sockaddr* CLARINET_RESTRICT dst,
                              size_t dstlen,
                              const clarinet_endpoint* CLARINET_RESTRICT src)
{
}

int
clarinet_endpoint_from_sockaddr(clarinet_endpoint* CLARINET_RESTRICT dst,
                                const struct sockaddr* CLARINET_RESTRICT src,
                                size_t srclen)
{
    
}                                    

int
clarinet_addr_to_string(char* CLARINET_RESTRICT dst,
                        size_t dstlen,
                        const clarinet_addr* CLARINET_RESTRICT src)
{
}

int 
clarinet_addr_from_string(clarinet_addr* CLARINET_RESTRICT dst,
                          const char* CLARINET_RESTRICT src,
                          size_t srclen)
{
}

int
clarinet_endpoint_to_string(char* CLARINET_RESTRICT dst,
                            size_t dstlen,
                            const clarinet_endpoint* CLARINET_RESTRICT src)
{
}

int 
clarinet_endpoint_from_string(clarinet_endpoint* CLARINET_RESTRICT dst,
                              const char* CLARINET_RESTRICT src,
                              size_t srclen)
{
}

