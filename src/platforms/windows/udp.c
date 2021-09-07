#include "portability.h"
#include "clarinet/clarinet.h"
#include "portable/addr.h"

static uint32_t wsacounter = 0; /* number of sockets currently relying on the winsock DLL */

int
clarinet_udp_open(const clarinet_endpoint* restrict endpoint,
                  const clarinet_udp_settings* restrict settings,
                  uint32_t flags)
{
    // TODO: if WSAStartCounter == 0 then WSAStartup
    // TODO: increment WSAStartCounter
    // TODO: convert endpoint to sockaddr
    // TODO: open socket
    // TODO: set socket options
    // TODO: bind socket
    // TODO: if any error close socket, decrement WSAStartCounter, translate error code
    // TODO: if WSAStartCounter == 0 then WSACleanup
    // TODO: return error code
    return CLARINET_ENOSYS;
}

int
clarinet_udp_close(int sockfd)
{
    // TODO: close socket
    // TODO: if error translate error code
    // TODO: if success, decrement WSAStartCounter and if WSAStartCounter == 0 then WSACleanup     
    // TODO: return error code
    return CLARINET_ENOSYS;
}

int
clarinet_udp_send(int sockfd,
                  const void* restrict buf,
                  size_t len,
                  const clarinet_endpoint* restrict dst)
{
    // TODO
    return CLARINET_ENOSYS;

}

int
clarinet_udp_recv(int sockfd,
                  void* restrict buf,
                  size_t len,
                  clarinet_endpoint* restrict src)
{
    // TODO
    return CLARINET_ENOSYS;
}

int
clarinet_udp_setopt(int sockfd,
                    int proto,
                    int optname,
                    const void* restrict optval,
                    size_t optlen)
{
    // TODO
    return CLARINET_ENOSYS;
}

int
clarinet_udp_getopt(int sockfd,
                    int proto,
                    int optname,
                    void* restrict optval,
                    size_t* restrict optlen)
{
    // TODO
    return CLARINET_ENOSYS;
}
