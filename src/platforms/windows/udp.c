#include "platforms/windows/sys.h"
#include "portable/addr.h"

#include <assert.h>

struct clarinet_udp_socket
{
    /* A clarinet_socket must be the first member acting as a pseudo base class */
    clarinet_socket base;
};

const clarinet_udp_settings 
clarinet_udp_settings_default = { 8192, 8192, 64 };

int
clarinet_udp_open(clarinet_udp_socket** spp,
                  const clarinet_endpoint* restrict local,
                  const clarinet_udp_settings* restrict settings,
                  uint32_t flags)
{
    if (!spp || *spp || !local || !settings)
        return CLARINET_EINVAL;

#if !CLARINET_ENABLE_IPV6 || !HAVE_SOCKADDR_IN6_SIN6_ADDR
    if (local->addr.family == CLARINET_AF_INET6)
        return CLARINET_EADDRNOTAVAIL;
#endif

    /* Restrict buffer sizes to INT_MAX because the type expected by setsockopt is an int */
    if (settings->send_buffer_size > INT_MAX || settings->recv_buffer_size > INT_MAX)
        return CLARINET_EINVAL;
        
    struct sockaddr_storage ss;
    const int errcode = clarinet_endpoint_to_sockaddr(&ss, local);
    if (errcode != CLARINET_ENONE)
        return errcode;
   
    /* The simplest apprach is to call WSAStartup every time. According to 
     * https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-wsastartup all calls to WSAStartup are
     * ref counted and the library will only be unloaded after the same number of WSACleanup calls are performed. */
    
    /* Version 2.2. is the one for Windows Vista and later */    
    const WORD wsa_version = MAKEWORD(2, 2);
    WSADATA wsa_data;
    /* WSAStartup returns a wsaerr directly, WSAGetLastError should not be used in this case. */
    int wsaerr = WSAStartup(wsa_version, &wsa_data);
    if (wsaerr != 0)
        return clarinet_error_from_wsaerr(wsaerr);
     
    SOCKET sockfd = socket(ss.ss_family, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd != INVALID_SOCKET)
    {
        /* Set socket options and bind.
         * Technically IP_TTL is not required to fail on setsockopt so one should check whether IP_TTL is supported by 
         * using getsockopt first to get the current value and if getsockopt fails then IP_TTL is not supported but all 
         * windows versions since Windows Vista support this option. The same applies to IPV6_UNICAST_HOPS. */
        const ULONG reuseaddr = (flags & clarinet_socket_option_to_flag(CLARINET_SO_REUSEADDR)) ? 1 : 0;
        const ULONG exclusiveaddr = (~reuseaddr) & 1;
        const DWORD ipv6only = (flags & clarinet_socket_option_to_flag(CLARINET_SO_IPV6DUAL)) ? 0 : 1;
        const DWORD ttl = (DWORD)settings->ttl;
        const int sendbuf = (int)settings->send_buffer_size;
        const int recvbuf = (int)settings->recv_buffer_size;
        if ( setsockopt(sockfd, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (const char*)&exclusiveaddr, sizeof(exclusiveaddr)) == SOCKET_ERROR
         || setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuseaddr, sizeof(reuseaddr)) == SOCKET_ERROR
         || setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (const char*)&sendbuf, sizeof(sendbuf)) == SOCKET_ERROR   
         || setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (const char*)&recvbuf, sizeof(recvbuf)) == SOCKET_ERROR   
         || (ss.ss_family == AF_INET && setsockopt(sockfd, IPPROTO_IP, IP_TTL, (const char*)&ttl, sizeof(ttl)) == SOCKET_ERROR)
    #if CLARINET_ENABLE_IPV6 && HAVE_SOCKADDR_IN6_SIN6_ADDR
         || (ss.ss_family == AF_INET6 && setsockopt(sockfd, IPPROTO_IPV6, IPV6_V6ONLY, (const char*)&ipv6only, sizeof(ipv6only)) == SOCKET_ERROR)         
         || (ss.ss_family == AF_INET6 && setsockopt(sockfd, IPPROTO_IPV6, IPV6_UNICAST_HOPS, (const char*)&ttl, sizeof(ttl)) == SOCKET_ERROR) /* curiously IPV6_HOPLIMIT corresponds to IP_RECV_TTL not IP_TTL */
    #endif         
         || setsocknonblock(sockfd) == SOCKET_ERROR
         || bind(sockfd, (struct sockaddr*)&ss, sizeof(ss)) == SOCKET_ERROR)
        {
            wsaerr = WSAGetLastError();
            /* There is not much we can do if close fails here... 
             * perhaps log the additional error if logging is enabled. */
            closesocket(sockfd); 
            WSACleanup();
            return clarinet_error_from_wsaerr(wsaerr);
        }
    }
    
    *spp = (clarinet_udp_socket*)clarinet_malloc(sizeof(clarinet_udp_socket));
    if (!*spp)
    {
        closesocket(sockfd); 
        WSACleanup();
        return CLARINET_ENOMEM;
    }
    
    memset(*spp, 0, sizeof(clarinet_udp_socket)); /* sanity initialization */
    (*spp)->base.handle = sockfd;
    return CLARINET_ENONE;
}                                   

int
clarinet_udp_close(clarinet_udp_socket** spp)
{   
    return clarinet_socket_close((clarinet_socket**)spp, 0);
}

int
clarinet_udp_send(clarinet_udp_socket* restrict sp,
                  const void* restrict buf,
                  size_t len,
                  const clarinet_endpoint* restrict dst)
{
    CLARINET_IGNORE(sp);
    CLARINET_IGNORE(buf);
    CLARINET_IGNORE(len);
    CLARINET_IGNORE(dst);
    return CLARINET_ENOSYS;
}

int
clarinet_udp_get_endpoint(clarinet_udp_socket* restrict sp,
                          clarinet_endpoint* restrict endpoint)
{
    return clarinet_socket_get_endpoint(&sp->base, endpoint);
}


int
clarinet_udp_recv(clarinet_udp_socket* restrict sp,
                  void* restrict buf,
                  size_t len,
                  clarinet_endpoint* restrict src)
{
    CLARINET_IGNORE(sp);
    CLARINET_IGNORE(buf);
    CLARINET_IGNORE(len);
    CLARINET_IGNORE(src);
    return CLARINET_ENOSYS;
}

int
clarinet_udp_set_option(clarinet_udp_socket* restrict sp,
                        int proto,
                        int optname,
                        const void* restrict optval,
                        size_t optlen)
{
    /* TODO: validate proto and optname */
    
    CLARINET_IGNORE(sp);
    CLARINET_IGNORE(proto);
    CLARINET_IGNORE(optname);
    CLARINET_IGNORE(optval);
    CLARINET_IGNORE(optlen);
    
    return CLARINET_ENOSYS;
}

int
clarinet_udp_get_option(clarinet_udp_socket* restrict sp,
                        int proto,
                        int optname,
                        void* restrict optval,
                        size_t* restrict optlen)
{
    /* TODO: validate proto and optname */
    
    CLARINET_IGNORE(sp);
    CLARINET_IGNORE(proto);
    CLARINET_IGNORE(optname);
    CLARINET_IGNORE(optval);
    CLARINET_IGNORE(optlen);
    
    return CLARINET_ENOSYS;
}
