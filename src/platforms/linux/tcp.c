#include "platforms/linux/sys.h"
#include "portable/addr.h"

#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <assert.h>

struct clarinet_tcp_socket
{
    /* A clarinet_socket must be the first member so we can cast to clarinet_socket* safely */
    clarinet_socket base;
};

const clarinet_tcp_settings 
clarinet_tcp_settings_default = { 8192, 8192, 64 };

int 
clarinet_tcp_listen(clarinet_tcp_socket** spp,
                    const clarinet_endpoint* restrict local, 
                    const clarinet_tcp_settings* restrict settings,
                    uint32_t flags)
{
    CLARINET_IGNORE(spp);
    CLARINET_IGNORE(local);
    CLARINET_IGNORE(settings);
    CLARINET_IGNORE(flags);
    
    return CLARINET_ENOSYS;
}

int 
clarinet_tcp_connect(clarinet_tcp_socket** spp,
                     const clarinet_endpoint* restrict local, 
                     const clarinet_endpoint* restrict remote, 
                     const clarinet_udp_settings* restrict settings,
                     uint32_t flags)
{
    CLARINET_IGNORE(spp);
    CLARINET_IGNORE(local);
    CLARINET_IGNORE(remote);
    CLARINET_IGNORE(settings);
    CLARINET_IGNORE(flags);    
    return CLARINET_ENOSYS;
}


int 
clarinet_tcp_close(clarinet_tcp_socket** spp)
{
    return clarinet_socket_close((clarinet_socket**)spp, 0);
}

int
clarinet_tcp_get_endpoint(clarinet_tcp_socket* restrict sp,
                          clarinet_endpoint* restrict endpoint)
{
    return clarinet_socket_get_endpoint(&sp->base, endpoint);
}
