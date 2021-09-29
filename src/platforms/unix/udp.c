#include "platforms/unix/sys.h"
#include "portable/addr.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <string.h>

struct clarinet_udp_socket
{
    /* A clarinet_socket must be the first member, so we can cast to clarinet_socket* safely */
    clarinet_socket base;
};

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
    socklen_t sslen;
    const int errcode = clarinet_endpoint_to_sockaddr(&ss, &sslen, local);
    if (errcode != CLARINET_ENONE)
        return errcode;

    const uint32_t ipv6dual = flags & clarinet_so_flag(CLARINET_SO_IPV6DUAL);
    if (ipv6dual && (local->addr.family != CLARINET_AF_INET6 || !(CLARINET_FEATURE_IPV6DUAL & clarinet_get_features())))
        return CLARINET_EINVAL;

    const int on = 1;
    const int off = 0;

    const int ipv6only = ipv6dual ? off : on;
    const int reuseaddrport = (flags & clarinet_so_flag(CLARINET_SO_REUSEADDR)) ? on : off;
#if defined(SO_EXCLBIND)
    const int exclbind = (~reuseaddrport) & 1;
#endif
    const int ttl = settings->ttl;

    const int sendbuf = (int)(settings->send_buffer_size);
    const int recvbuf = (int)(settings->recv_buffer_size);

#if defined(__linux__)
    /* Linux doubles the buffer sizes passed to setsockopt, so we need to half them here to keep consistent with how
     * bufffer sizes are set in other platforms */
    const int sendbufval = sendbuf >> 1;
    const int recvbufval = recvbuf >> 1;
#else
    const int sendbufval = sendbuf;
    const int recvbufval = recvbuf;
#endif

    int sockfd = socket(ss.ss_family, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd != INVALID_SOCKET)
    {
        /* Set socket options and bind.
         * By default, Linux UDP does path MTU (Maximum Transmission Unit) discovery.  This means the kernel will keep
         * track of the MTU to a specific target IP address and return EMSGSIZE when a UDP packet write exceeds it.
         * When this happens, the application should decrease the packet size.  Path MTU discovery can be also turned
         * off using the IP_MTU_DISCOVER socket option or the /proc/sys/net/ipv4/ip_no_pmtu_disc file; see ip(7) for
         * details. When turned off, UDP will fragment outgoing UDP packets that exceed the interface MTU. */
        if ((sendbuf > 0 && setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (const void*)&sendbufval, sizeof(sendbufval)) == SOCKET_ERROR)
            || (recvbuf > 0 && setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (const void*)&recvbufval, sizeof(recvbufval)) == SOCKET_ERROR)
            || setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void*)&reuseaddrport, sizeof(reuseaddrport)) == SOCKET_ERROR
            /* Some BSD systems support a special reuse port mode that promotes load balancing between connections */
            #if defined(SO_REUSEPORT_LB)
            || setsockoptdep(sockfd, SOL_SOCKET, SO_REUSEPORT_LB, (const void*)&reuseaddrport, sizeof(reuseaddrport)) == SOCKET_ERROR
            /* SO_REUSEPORT is only supported on Linux >= 3.9 but also promotes load balancing on UDP - may be present on some patched 2.6 systems (e.g. REHL 2.6.32) */
            #elif defined(SO_REUSEPORT)
            || setsockoptdep(sockfd, SOL_SOCKET, SO_REUSEPORT, (const void*)&reuseaddrport, sizeof(reuseaddrport)) == SOCKET_ERROR
            #endif
            /* Solaris supports SO_EXCLBIND similarly to SO_EXCLUSIVEADDRUSE on Windows */
            #if defined(SO_EXCLBIND)
            || setsockoptdep(sockfd, SOL_SOCKET, SO_EXCLBIND, (const void*)&exclbind, sizeof(exclbind)) == SOCKET_ERROR
            #endif
            #if defined(IP_MTU_DISCOVER)
            || setsockoptdep(sockfd, IPPROTO_IP, IP_MTU_DISCOVER, (const void*)&off, sizeof(off)) == SOCKET_ERROR
            #endif
            || (ss.ss_family == AF_INET && ttl > 0 && setsockopt(sockfd, IPPROTO_IP, IP_TTL, (const void*)&ttl, sizeof(ttl)) == SOCKET_ERROR)
            #if CLARINET_ENABLE_IPV6 && HAVE_SOCKADDR_IN6_SIN6_ADDR
            || (ss.ss_family == AF_INET6 && setsockopt(sockfd, IPPROTO_IPV6, IPV6_V6ONLY, (const void*)&ipv6only, sizeof(ipv6only)) == SOCKET_ERROR)
            || (ss.ss_family == AF_INET6 && ttl > 0 && setsockopt(sockfd, IPPROTO_IPV6, IPV6_UNICAST_HOPS, (void*)&ttl, sizeof(ttl)) == SOCKET_ERROR) /* curiously IPV6_HOPLIMIT corresponds to IP_RECV_TTL not IP_TTL */
            #endif
            /* Linux has an undocumented flag to enable/disable UDP checksum */
            #if defined(SO_NO_CHECK)
            || (ss.ss_family == AF_INET && setsockoptdep(sockfd, SOL_SOCKET, SO_NO_CHECK, (const void*)&off, sizeof(off)) == SOCKET_ERROR)
            #endif
            /* macOS has an undocumented flag to enable/disable UDP checksum */
            #if defined(UDP_NOCKSUM)
            || (ss.ss_family == AF_INET && setsockoptdep(sockfd, IPPROTO_UDP, UDP_NOCKSUM, (const void*)&off, sizeof(off)) == SOCKET_ERROR)
            #endif
            || setnonblock(sockfd) == SOCKET_ERROR
            || bind(sockfd, (struct sockaddr*)&ss, sslen) == SOCKET_ERROR)
        {
            int err = errno;
            /* There is not much we can do if close fails... perhaps log the additional error if logging is enabled. */
            close(sockfd);
            return clarinet_error_from_errno(err);
        }
    }

    *spp = (clarinet_udp_socket*)clarinet_malloc(sizeof(clarinet_udp_socket));
    if (!*spp)
    {
        close(sockfd);
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
clarinet_udp_get_endpoint(clarinet_udp_socket* restrict sp,
                          clarinet_endpoint* restrict endpoint)
{
    return clarinet_socket_get_endpoint((clarinet_socket*)sp, endpoint);
}

int
clarinet_udp_send(clarinet_udp_socket* restrict sp,
                  const void* restrict buf,
                  size_t buflen,
                  const clarinet_endpoint* restrict dst)
{
    return clarinet_socket_send((clarinet_socket*)sp, buf, buflen, dst);
}

int
clarinet_udp_recv(clarinet_udp_socket* restrict sp,
                  void* restrict buf,
                  size_t buflen,
                  clarinet_endpoint* restrict src)
{
    return clarinet_socket_recv((clarinet_socket*)sp, buf, buflen, src);
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

    return CLARINET_ENOTIMPL;
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

    return CLARINET_ENOTIMPL;
}
