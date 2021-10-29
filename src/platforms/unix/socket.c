#include "compat/compat.h"
#include "clarinet/clarinet.h"

#include "compat/addr.h"
#include "compat/error.h"

#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <clarinet/clarinet.h>

/* region Initialization */

int
clarinet_initialize(void)
{
    return CLARINET_ENONE;
}

int
clarinet_finalize(void)
{
    return CLARINET_ENONE;
}

/* endregion */

/* region Helpers */

/** Return non-zero (true) if error code is EWOULDBLOCK or EAGAIN, otherwise 0. */
CLARINET_STATIC_INLINE
int
clarinet_again(int e)
{
    #if HAVE_EAGAIN_EQUAL_TO_EWOULDBLOCK
    return (e == EWOULDBLOCK);
    #else
    return (e == EWOULDBLOCK) || (e == EAGAIN);
    #endif /* HAVE_EAGAIN_EQUAL_TO_EWOULDBLOCK */
}

/**
 * Helper for setting a non-mandatory socket option and ignore ENOPROTOOPT if the platform does not support the option.
 * This is used so that a call to setsockopt(2) that fails in the context of a larger operation does not compromise the
 * entire operation.
 */
CLARINET_STATIC_INLINE
int
trysockopt(int sockfd,
           int level,
           int optname,
           const void* optval,
           socklen_t optlen)
{
    if (setsockopt(sockfd, level, optname, optval, optlen) == SOCKET_ERROR)
        return (clarinet_get_sockapi_error() == ENOPROTOOPT) ? 0 : SOCKET_ERROR;

    return 0;
}

/** Helper for setting the socket blocking value.*/
CLARINET_STATIC_INLINE
int
setnonblock(int sockfd,
            int value)
{
    int flags = fcntl(sockfd, F_GETFL);
    if (flags == SOCKET_ERROR)
        return SOCKET_ERROR;

    if (value)
        flags |= O_NONBLOCK;
    else
        flags &= ~O_NONBLOCK;

    return fcntl(sockfd, F_SETFL, flags);
}

/** Helper for getting the socket blocking mode.*/
CLARINET_STATIC_INLINE
int
getnonblock(int sockfd,
            int* value)
{
    int flags = fcntl(sockfd, F_GETFL);
    if (flags == SOCKET_ERROR)
        return SOCKET_ERROR;

    *value = (flags & O_NONBLOCK) ? 1 : 0;
    return 0;
}

/* endregion */

/* region Socket */

void
clarinet_socket_init(clarinet_socket* sp)
{
    memset(sp, 0, sizeof(clarinet_socket));
    sp->u.fd = INVALID_SOCKET;
}

int
clarinet_socket_open(clarinet_socket* sp,
                     int family,
                     int proto)
{
    if (!sp || sp->family != CLARINET_AF_UNSPEC || sp->proto != CLARINET_PROTO_NONE || sp->u.fd != INVALID_SOCKET)
        return CLARINET_EINVAL;

    int sfamily;
    switch (family) // NOLINT(hicpp-multiway-paths-covered)
    {
        case CLARINET_AF_INET:
            sfamily = AF_INET;
            break;
        case CLARINET_AF_INET6:
            #if CLARINET_ENABLE_IPV6
            sfamily = AF_INET6;
            break;
            #endif /*CLARINET_ENABLE_IPV6 */
        default:
            return CLARINET_EAFNOSUPPORT;
    }

    int sdomain;
    int sproto;
    switch (proto)
    {
        case CLARINET_PROTO_UDP:
            sdomain = SOCK_DGRAM;
            sproto = IPPROTO_UDP;
            break;
        case CLARINET_PROTO_TCP:
            sdomain = SOCK_STREAM;
            sproto = IPPROTO_TCP;
            break;
        default:
            return CLARINET_EPROTONOSUPPORT;
    }

    int sockfd = socket(sfamily, sdomain, sproto);
    if (sockfd == INVALID_SOCKET)
        return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

    if (sproto == IPPROTO_UDP)
    {

        #if defined(SO_NO_CHECK) || defined(UDP_NOCKSUM)
        static const int off = 0;
        #endif

        #if defined(SO_NO_CHECK)
        /* Linux has an undocumented flag to enable/disable UDP checksum on IPv4 but we want UDP checksums even if for
         * some reason it was disabled in the system as there is no portable option for the user to set it. */
        if (sfamily == AF_INET && trysockopt(sockfd, SOL_SOCKET, SO_NO_CHECK, (const void*)&off, sizeof(off)) == SOCKET_ERROR)
        {
            const int err = clarinet_get_sockapi_error();
            close(sockfd); /* there's nothing we can do (or the user) if close fails here... */
            return clarinet_error_from_sockapi_error(err);
        }
        #endif /* defined(SO_NO_CHECK) */

        #if defined(UDP_NOCKSUM)
        /* macOS has an undocumented flag to enable/disable UDP checksum on IPv4 but we want UDP checksums even if for
         * some reason it was disabled in the system as there is no portable option for the user to set it. */
        if (sfamily == AF_INET && trysockopt(sockfd, IPPROTO_UDP, UDP_NOCKSUM, (const void*)&off, sizeof(off)) == SOCKET_ERROR)
        {
            const int err = clarinet_get_sockapi_error();
            close(sockfd); /* there's nothing we can do (or the user) if close fails here... */
            return clarinet_error_from_sockapi_error(err);
        }
        #endif /* defined(UDP_NOCKSUM) */
    }

    sp->family = (uint16_t)family;
    sp->proto = (uint16_t)proto;
    sp->u.fd = sockfd;

    return CLARINET_ENONE;
}

int
clarinet_socket_close(clarinet_socket* socket)
{
    if (!socket || socket->family == CLARINET_AF_UNSPEC || socket->proto == CLARINET_PROTO_NONE)
        return CLARINET_EINVAL;

    /* All negative descriptors are invalid and values 0, 1 and 2 are reserved for stdin, stdout and stderr and should
     * not be touched. */
    const int sockfd = socket->u.fd;
    if (sockfd < 4)
        return CLARINET_EINVAL;

    /* close(2) MAY only fail with  EBADF, EINTR and EIO. Both ENOSPC, EDQUOT only apply to actual files.
     * On Linux close(2) is expected to block even on non-blocking sockets with data to flush and a non-zero
     * linger timeout is enabled.
     *
     * Note that close(2) on Linux is final and this should be the case in all other un*x systems because retrying after
     * a failure may cause a reused file descriptor from another thread to be closed. This can occur because the kernel
     * might release the file descriptor early in the close operation, freeing it for reuse; the steps that may return
     * an error, such as flushing data to the filesystem or device, occurring only later in the close operation.
     * EINTR is a somewhat special case.  Regarding the EINTR error, POSIX.1-2008 says:
     *
     *        If close() is interrupted by a signal that is to be caught, it shall return -1 with errno set to EINTR and
     *        the state of fildes is unspecified.
     *
     * This permits the behavior that occurs on Linux and many other implementations, where, as with other errors that
     * may be reported by close(), the file descriptor is guaranteed to be closed.
     *
     * In any case we handle the possibility of EWOULDBLOCK/EAGAIN being returned as an exception to the rule. We try to
     * change the socket to blocking and close again. We never force SO_LINGER(0) here because it is not clear what
     * the user intentions are. There is no point in having a boolean argument for that either (e.g.: force) because the
     * user can always adjust SO_LINGER or use the shorcut option SO_DONTLINGER at any time before closing the socket.
     */
    if (close(sockfd) == SOCKET_ERROR)
    {
        int err = clarinet_get_sockapi_error();
        if (!clarinet_again(err))
            return clarinet_error_from_sockapi_error(err);

        /* At this point if close(2) is still failing with EWOULDBLOCK/EAGAIN we have no choice but to wait and try
         * again until the SO_LINGER timeout expires or the output buffer is flushed. We don't simply wait for the 
         * whole SO_LINGER duration because the output buffer could be flushed way before that. This is mostly a safety 
         * mechanism because none of this should normally happen. */
        setnonblock(sockfd, 1);
        while (
            (sockfd) == SOCKET_ERROR)
        {
            err = clarinet_get_sockapi_error();
            if (!clarinet_again(err))
                return clarinet_error_from_sockapi_error(err);

            sleep(1);
        }
    }

    clarinet_socket_init(socket);

    return CLARINET_ENONE;
}

int
clarinet_socket_bind(clarinet_socket* restrict socket,
                     const clarinet_endpoint* restrict local)
{
    if (!socket || socket->family == CLARINET_AF_UNSPEC || socket->proto == CLARINET_PROTO_NONE || !local)
        return CLARINET_EINVAL;

    struct sockaddr_storage ss;
    socklen_t sslen;
    const int errcode = clarinet_endpoint_to_sockaddr(&ss, &sslen, local);
    if (errcode != CLARINET_ENONE)
        return errcode;

    int sockfd = socket->u.fd;
    if (bind(sockfd, (struct sockaddr*)&ss, sslen) == SOCKET_ERROR)
        return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

    return CLARINET_ENONE;
}

int
clarinet_socket_local_endpoint(clarinet_socket* restrict socket,
                               clarinet_endpoint* restrict local)
{
    if (!socket || socket->family == CLARINET_AF_UNSPEC || socket->proto == CLARINET_PROTO_NONE || !local)
        return CLARINET_EINVAL;

    struct sockaddr_storage ss = { 0 };
    socklen_t length = sizeof(ss);

    int sockfd = socket->u.fd;
    if (getsockname(sockfd, (struct sockaddr*)&ss, &length) == SOCKET_ERROR)
        return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

    return clarinet_endpoint_from_sockaddr(local, &ss);
}

int
clarinet_socket_remote_endpoint(clarinet_socket* restrict socket,
                                clarinet_endpoint* restrict remote)
{
    if (!socket || socket->family == CLARINET_AF_UNSPEC || socket->proto == CLARINET_PROTO_NONE || !remote)
        return CLARINET_EINVAL;

    struct sockaddr_storage ss = { 0 };
    socklen_t length = sizeof(ss);

    int sockfd = socket->u.fd;
    if (getpeername(sockfd, (struct sockaddr*)&ss, &length) == SOCKET_ERROR)
        return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

    return clarinet_endpoint_from_sockaddr(remote, &ss);
}

int
clarinet_socket_send(clarinet_socket* restrict socket,
                     const void* restrict buf,
                     size_t buflen)
{
    if (!socket || socket->family == CLARINET_AF_UNSPEC || socket->proto == CLARINET_PROTO_NONE || (!buf && buflen > 0) || buflen > INT_MAX)
        return CLARINET_EINVAL;

    int sockfd = socket->u.fd;
    if (send(sockfd, buf, buflen, 0) == SOCKET_ERROR)
        return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

    return CLARINET_ENONE;
}

int
clarinet_socket_sendto(clarinet_socket* restrict socket,
                       const void* restrict buf,
                       size_t buflen,
                       const clarinet_endpoint* restrict remote)
{
    if (!socket || socket->family == CLARINET_AF_UNSPEC || socket->proto == CLARINET_PROTO_NONE || (!buf && buflen > 0) || buflen > INT_MAX || !remote)
        return CLARINET_EINVAL;

    struct sockaddr_storage ss;
    socklen_t sslen;
    const int errcode = clarinet_endpoint_to_sockaddr(&ss, &sslen, remote);
    if (errcode != CLARINET_ENONE)
        return errcode;

    int sockfd = socket->u.fd;
    const ssize_t n = sendto(sockfd, buf, buflen, 0, (struct sockaddr*)&ss, sslen);
    if (n < 0)
        return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

    return (int)n;
}

int
clarinet_socket_recv(clarinet_socket* restrict socket,
                     void* restrict buf,
                     size_t buflen)
{
    if (!socket || socket->family == CLARINET_AF_UNSPEC || socket->proto == CLARINET_PROTO_NONE || !buf || buflen == 0 || buflen > INT_MAX)
        return CLARINET_EINVAL;

    int sockfd = socket->u.fd;
    const ssize_t n = recv(sockfd, buf, buflen, 0);
    if (n < 0)
        return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

    return (int)n;
}

int
clarinet_socket_recvfrom(clarinet_socket* restrict socket,
                         void* restrict buf,
                         size_t buflen,
                         clarinet_endpoint* restrict remote)
{
    if (!socket || socket->family == CLARINET_AF_UNSPEC || socket->proto == CLARINET_PROTO_NONE || !buf || buflen == 0 || buflen > INT_MAX || !remote)
        return CLARINET_EINVAL;

    struct sockaddr_storage ss;
    const socklen_t slen = sizeof(ss);

    /* The description of recvfrom(2) at https://pubs.opengroup.org/onlinepubs/009696699/functions/recvfrom.html
     * does not document EMSGSIZE, and the Linux manual page for recvfrom(2) does not document it either. Linux
     * supports sending with the MSG_TRUNC flag, in which case recvfrom will return the real length of the datagram
     * even when it is longer than the passed buffer but this is not supported in BSD/Darwin and as of Sep, 2021 WSL
     * seems to ignore the MSG_TRUNC flag so the only real choice is recvmsg(2). Note that at least on Linux recvfrom(2)
     * is just a convenience because internally it is translated into a call to recvmsg anyway.
     */
    struct iovec iov;
    struct msghdr msg;

    msg.msg_control = NULL;
    msg.msg_controllen = 0;
    msg.msg_flags = 0;
    msg.msg_name = (struct sockaddr*)&ss;
    msg.msg_namelen = slen;

    iov.iov_base = buf;
    iov.iov_len = buflen;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    int sockfd = socket->u.fd;
    const ssize_t n = recvmsg(sockfd, &msg, 0);
    if (n < 0)
        return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

    if (msg.msg_flags & MSG_TRUNC)
        return CLARINET_EMSGSIZE;

    /* Sanity: for platforms that may return the datagram size instead of implmenting MSG_TRUNC properly  */
    if ((size_t)n > buflen)
        return CLARINET_EMSGSIZE;

    /* Sanity: improbable but possible if the system disagrees on the size of the sockaddr (then something is off)  */
    if (msg.msg_namelen > sizeof(ss))
        return CLARINET_EADDRNOTAVAIL;

    const int errcode = clarinet_endpoint_from_sockaddr(remote, &ss);
    if (errcode != CLARINET_ENONE)
        return CLARINET_EADDRNOTAVAIL;

    return (int)n;
}

int
clarinet_socket_setopt(clarinet_socket* restrict socket,
                       int optname,
                       const void* restrict optval,
                       size_t optlen)
{
    if (!socket || socket->family == CLARINET_AF_UNSPEC || socket->proto == CLARINET_PROTO_NONE || !optval || optlen > INT_MAX)
        return CLARINET_EINVAL;

    int sockfd = socket->u.fd;
    switch (optname)
    {
        case CLARINET_SO_NONBLOCK:
            if (optlen == sizeof(int32_t))
            {
                const int val = *(const int32_t*)optval ? 1 : 0;
                if (setnonblock(sockfd, val) == SOCKET_ERROR)
                    return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                return CLARINET_ENONE;
            }
            break;
        case CLARINET_SO_REUSEADDR:
            if (optlen == sizeof(int32_t))
            {
                const int val = *(const int32_t*)optval ? 1 : 0;
                if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) == SOCKET_ERROR)
                    return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                #if defined(SO_REUSEPORT_LB)
                /* Some BSD systems support a special reuse port mode that promotes load balancing between connections */
                if (trysockopt(sockfd, SOL_SOCKET, SO_REUSEPORT_LB, &val, sizeof(val)) == SOCKET_ERROR)
                    return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());
                #elif defined(SO_REUSEPORT)
                /* SO_REUSEPORT is only supported on Linux >= 3.9 but also promotes load balancing on UDP - may be present on some patched 2.6 kernels (e.g. REHL 2.6.32) */
                if (trysockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &val, sizeof(val)) == SOCKET_ERROR)
                    return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());
                #endif /* defined(SO_REUSEPORT) */

                #if defined(SO_EXCLBIND)
                /* Solaris supports SO_EXCLBIND similarly to SO_EXCLUSIVEADDRUSE on Windows */
                if (trysockopt(sockfd, SOL_SOCKET, SO_EXCLBIND, &val, sizeof(val)) == SOCKET_ERROR)
                    return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());
                #endif /* defined(SO_EXCLBIND) */

                return CLARINET_ENONE;
            }
            break;
        case CLARINET_SO_SNDBUF:
            if (optlen == sizeof(int32_t))
            {
                #if defined(__linux__)
                /* Linux doubles the buffer size passed to setsockopt, so we half it here to more closely match
                 * how bufffer sizes are set in other platforms. The effect is that odd numbers are rounded down
                 * to the closest even number. */
                const int val = (int)clamp(*(const int32_t*)optval, INT_MIN, INT_MAX) >> 1;
                #else
                const int val = (int)clamp(*(const int32_t*)optval, INT_MIN, INT_MAX);
                #endif /* defined(__linux__) */
                if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (const void*)&val, sizeof(val)) == SOCKET_ERROR)
                    return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                return CLARINET_ENONE;
            }
            break;
        case CLARINET_SO_RCVBUF:
            if (optlen == sizeof(int32_t))
            {
                /* Linux doubles the buffer size passed to setsockopt, so we half it here to more closely match
                 * how bufffer sizes are set in other platforms. The effect is that odd numbers are rounded down
                 * to the closest even number. */
                #if defined(__linux__)
                const int val = (int)clamp(*(const int32_t*)optval, INT_MIN, INT_MAX) >> 1;
                #else
                const int val = (int)clamp(*(const int32_t*)optval, INT_MIN, INT_MAX);
                #endif /* defined(__linux__) */
                if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (const void*)&val, sizeof(val)) == SOCKET_ERROR)
                    return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                return CLARINET_ENONE;
            }
            break;
        case CLARINET_SO_SNDTIMEO:
            if (optlen == sizeof(int32_t))
            {
                struct timeval val;
                ms2tv(&val, (int)clamp(*(const int32_t*)optval, INT_MIN, INT_MAX));
                if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (const void*)&val, sizeof(val)) == SOCKET_ERROR)
                    return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                return CLARINET_ENONE;
            }
            break;
        case CLARINET_SO_RCVTIMEO:
            if (optlen == sizeof(int32_t))
            {
                struct timeval val;
                ms2tv(&val, (int)clamp(*(const int32_t*)optval, INT_MIN, INT_MAX));
                if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const void*)&val, sizeof(val)) == SOCKET_ERROR)
                    return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                return CLARINET_ENONE;
            }
            break;
        case CLARINET_SO_KEEPALIVE:
            if (optlen == sizeof(int32_t))
            {
                const int val = *(const int32_t*)optval ? 1 : 0;
                if (setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val)) == SOCKET_ERROR)
                    return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                return CLARINET_ENONE;
            }
            break;
        case CLARINET_SO_LINGER:
            if (optlen == sizeof(clarinet_linger))
            {
                const clarinet_linger* clinger = (const clarinet_linger*)optval;
                struct linger val;
                val.l_onoff = clinger->enabled;
                val.l_linger = clinger->seconds;
                if (setsockopt(sockfd, SOL_SOCKET, SO_LINGER, (const void*)&val, sizeof(val)) == SOCKET_ERROR)
                    return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                return CLARINET_ENONE;
            }
            break;
        case CLARINET_SO_DONTLINGER:
            if (optlen == sizeof(int32_t))
            {
                struct linger val;
                socklen_t len = sizeof(val);
                if (getsockopt(sockfd, SOL_SOCKET, SO_LINGER, (void*)&val, &len) == SOCKET_ERROR)
                    return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                if (len != sizeof(val)) /* sanity check */
                    return CLARINET_ESYS;

                val.l_onoff = *(const int32_t*)optval ? 0 : 1;
                if (setsockopt(sockfd, SOL_SOCKET, SO_LINGER, (const void*)&val, sizeof(val)) == SOCKET_ERROR)
                    return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                return CLARINET_ENONE;
            }
            break;
        case CLARINET_IP_V6ONLY:
            #if CLARINET_ENABLE_IPV6
            if (optlen == sizeof(int32_t))
            {
                const int family = socket->family;
                if (family == CLARINET_AF_INET6)
                {
                    const int val = *(const int32_t*)optval ? 1 : 0;
                    if (setsockopt(sockfd, IPPROTO_IPV6, IPV6_V6ONLY, &val, sizeof(val)) == SOCKET_ERROR)
                        return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                    return CLARINET_ENONE;
                }
            }
            #endif /* CLARINET_ENABLE_IPV6 */
            break;
        case CLARINET_IP_TTL:
            if (optlen == sizeof(int32_t))
            {
                const int family = socket->family;
                if (family == CLARINET_AF_INET)
                {
                    const int val = *(const int32_t*)optval;
                    if (setsockopt(sockfd, IPPROTO_IP, IP_TTL, &val, sizeof(val)) == SOCKET_ERROR)
                        return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                    return CLARINET_ENONE;
                }

                #if CLARINET_ENABLE_IPV6
                if (family == CLARINET_AF_INET6)
                {
                    const int val = *(const int32_t*)optval;
                    if (setsockopt(sockfd, IPPROTO_IPV6, IPV6_UNICAST_HOPS, &val, sizeof(val)) == SOCKET_ERROR) /* curiously IPV6_HOPLIMIT corresponds to IP_RECV_TTL not IP_TTL */
                        return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                    return CLARINET_ENONE;
                }
                #endif /* CLARINET_ENABLE_IPV6 */
            }
        case CLARINET_IP_MTU_DISCOVER:
            if (optlen == sizeof(int32_t))
            {
                const int family = socket->family;
                if (family == CLARINET_AF_INET)
                {
                    int val;
                    switch (*(const int32_t*)optval)
                    {
                        case CLARINET_PMTUD_UNSPEC:
                            val = IP_PMTUDISC_WANT;
                            break;
                        case CLARINET_PMTUD_ON:
                            val = IP_PMTUDISC_DO;
                            break;
                        case CLARINET_PMTUD_OFF:
                            #if defined(__linux__) && defined(IP_PMTUDISC_OMIT)
                            val = IP_PMTUDISC_OMIT;
                            #elif defined(__linux__) && defined(IP_PMTUDISC_INTERFACE)
                            val = IP_PMTUDISC_INTERAFCE;
                            #else
                            val = IP_PMTUDISC_DONT;
                            #endif
                            break;
                        case CLARINET_PMTUD_PROBE:
                            val = IP_PMTUDISC_PROBE;
                            break;
                        default:
                            return CLARINET_EINVAL;
                    }

                    if (setsockopt(sockfd, IPPROTO_IP, IP_MTU_DISCOVER, &val, sizeof(val)) == SOCKET_ERROR)
                        return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                    return CLARINET_ENONE;
                }

                #if CLARINET_ENABLE_IPV6
                if (family == CLARINET_AF_INET6)
                {
                    int val;
                    switch (*(const int32_t*)optval)
                    {
                        case CLARINET_PMTUD_UNSPEC:
                            val = IPV6_PMTUDISC_WANT;
                            break;
                        case CLARINET_PMTUD_ON:
                            val = IPV6_PMTUDISC_DO;
                            break;
                        case CLARINET_PMTUD_OFF:
                            #if defined(__linux__) && defined(IPV6_PMTUDISC_OMIT)
                            val = IPV6_PMTUDISC_OMIT;
                            #else
                            val = IPV6_PMTUDISC_DONT;
                            #endif
                            break;
                        case CLARINET_PMTUD_PROBE:
                            val = IPV6_PMTUDISC_PROBE;
                            break;
                        default:
                            return CLARINET_EINVAL;
                    }

                    if (setsockopt(sockfd, IPPROTO_IPV6, IPV6_MTU_DISCOVER, &val, sizeof(val)) == SOCKET_ERROR)
                        return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                    return CLARINET_ENONE;
                }
                #endif /* CLARINET_ENABLE_IPV6 */
            }
        default:
            break;
    }

    return CLARINET_EINVAL;
}

int
clarinet_socket_getopt(clarinet_socket* restrict socket,
                       int optname,
                       void* restrict optval,
                       size_t* restrict optlen)
{
    if (!socket || socket->family == CLARINET_AF_UNSPEC || socket->proto == CLARINET_PROTO_NONE || !optval || !optlen || *optlen > INT_MAX)
        return CLARINET_EINVAL;

    int sockfd = socket->u.fd;
    switch (optname)
    {
        case CLARINET_SO_NONBLOCK:
            if (*optlen >= sizeof(int32_t))
            {
                int val;
                if (getnonblock(sockfd, &val) == SOCKET_ERROR)
                    return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                *(int32_t*)optval = (int32_t)val;
                *optlen = sizeof(int32_t);

                return CLARINET_ENONE;
            }
            break;
        case CLARINET_SO_REUSEADDR:
            if (*optlen >= sizeof(int32_t))
            {
                int val;
                socklen_t len = sizeof(val);
                if (getsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &val, &len) == SOCKET_ERROR)
                    return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                if (len != sizeof(val)) /* sanity check */
                    return CLARINET_ESYS;

                *(int32_t*)optval = (int32_t)val;
                *optlen = sizeof(int32_t);

                return CLARINET_ENONE;
            }
            break;
        case CLARINET_SO_SNDBUF:
            if (*optlen >= sizeof(int32_t))
            {
                int val;
                socklen_t len = sizeof(val);
                if (getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &val, &len) == SOCKET_ERROR)
                    return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                if (len != sizeof(val)) /* sanity check */
                    return CLARINET_ESYS;

                *(int32_t*)optval = (int32_t)val;
                *optlen = sizeof(int32_t);

                return CLARINET_ENONE;
            }
            break;
        case CLARINET_SO_RCVBUF:
            if (*optlen >= sizeof(int))
            {
                int val;
                socklen_t len = sizeof(val);
                if (getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &val, &len) == SOCKET_ERROR)
                    return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                if (len != sizeof(val)) /* sanity check */
                    return CLARINET_ESYS;

                *(int32_t*)optval = (int32_t)val;
                *optlen = sizeof(int32_t);

                return CLARINET_ENONE;
            }
            break;
        case CLARINET_SO_SNDTIMEO:
            if (*optlen >= sizeof(int32_t))
            {
                struct timeval val;
                socklen_t len = sizeof(val);
                if (getsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (void*)&val, &len) == SOCKET_ERROR)
                    return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                if (len != sizeof(val)) /* sanity check */
                    return CLARINET_ESYS;

                *(int32_t*)optval = (int32_t)min(tv2ms(&val), INT32_MAX);
                *optlen = sizeof(int32_t);

                return CLARINET_ENONE;
            }
            break;
        case CLARINET_SO_RCVTIMEO:
            if (*optlen >= sizeof(int32_t))
            {
                struct timeval val;
                socklen_t len = sizeof(val);
                if (getsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (void*)&val, &len) == SOCKET_ERROR)
                    return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                if (len != sizeof(val)) /* sanity check */
                    return CLARINET_ESYS;

                *(int32_t*)optval = (int32_t)min(tv2ms(&val), INT32_MAX);
                *optlen = sizeof(int32_t);

                return CLARINET_ENONE;
            }
            break;
        case CLARINET_SO_KEEPALIVE:
            if (*optlen >= sizeof(int32_t))
            {
                int val;
                socklen_t len = sizeof(val);
                if (getsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &val, &len) == SOCKET_ERROR)
                    return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                if (len != sizeof(val)) /* sanity check */
                    return CLARINET_ESYS;

                *(int32_t*)optval = (int32_t)val;
                *optlen = sizeof(int32_t);

                return CLARINET_ENONE;
            }
            break;
        case CLARINET_SO_LINGER:
            if (*optlen >= sizeof(clarinet_linger))
            {
                struct linger val;
                socklen_t len = sizeof(val);

                if (getsockopt(sockfd, SOL_SOCKET, SO_LINGER, (void*)&val, &len) == SOCKET_ERROR)
                    return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                if (len != sizeof(val)) /* sanity check */
                    return CLARINET_ESYS;

                clarinet_linger* clinger = (clarinet_linger*)optval;
                clinger->enabled = val.l_onoff ? 1 : 0;
                clinger->seconds = (uint16_t)clamp(val.l_linger, 0, UINT16_MAX);
                *optlen = sizeof(clarinet_linger);

                return CLARINET_ENONE;
            }
            break;
        case CLARINET_SO_DONTLINGER:
            if (*optlen >= sizeof(int32_t))
            {
                struct linger val;
                socklen_t len = sizeof(val);
                if (getsockopt(sockfd, SOL_SOCKET, SO_LINGER, (void*)&val, &len) == SOCKET_ERROR)
                    return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                if (len != sizeof(val)) /* sanity check */
                    return CLARINET_ESYS;

                *(int32_t*)optval = val.l_onoff ? 0 : 1;
                *optlen = sizeof(int32_t);
                return CLARINET_ENONE;
            }
            break;
        case CLARINET_IP_V6ONLY:
            #if CLARINET_ENABLE_IPV6
            if (*optlen >= sizeof(int32_t))
            {
                const int family = socket->family;
                if (family == CLARINET_AF_INET6)
                {
                    int val;
                    socklen_t len = sizeof(val);
                    if (getsockopt(sockfd, IPPROTO_IPV6, IPV6_V6ONLY, &val, &len) == SOCKET_ERROR)
                        return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                    if (len != sizeof(val)) /* sanity check */
                        return CLARINET_ESYS;

                    *(int32_t*)optval = (int32_t)val;
                    *optlen = sizeof(int32_t);

                    return CLARINET_ENONE;
                }
            }
            #endif /* CLARINET_ENABLE_IPV6 */
            break;
        case CLARINET_IP_TTL:
            if (*optlen >= sizeof(int32_t))
            {
                const int family = socket->family;
                if (family == CLARINET_AF_INET)
                {
                    int val;
                    socklen_t len = sizeof(val);
                    if (getsockopt(sockfd, IPPROTO_IP, IP_TTL, &val, &len) == SOCKET_ERROR)
                        return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                    if (len != sizeof(val)) /* sanity check */
                        return CLARINET_ESYS;

                    *(int32_t*)optval = (int32_t)val;
                    *optlen = sizeof(int32_t);

                    return CLARINET_ENONE;
                }

                #if CLARINET_ENABLE_IPV6
                if (family == CLARINET_AF_INET6)
                {
                    int val;
                    socklen_t len = sizeof(val);
                    if (getsockopt(sockfd, IPPROTO_IPV6, IPV6_UNICAST_HOPS, &val, &len) == SOCKET_ERROR) /* curiously IPV6_HOPLIMIT corresponds to IP_RECV_TTL not IP_TTL */
                        return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                    if (len != sizeof(val)) /* sanity check */
                        return CLARINET_ESYS;

                    *(int32_t*)optval = (int32_t)val;
                    *optlen = sizeof(int32_t);

                    return CLARINET_ENONE;
                }
                #endif /* CLARINET_ENABLE_IPV6 */
            }
            break;
        case CLARINET_IP_MTU:
            if (*optlen >= sizeof(int32_t))
            {
                const int family = socket->family;
                if (family == CLARINET_AF_INET)
                {
                    int val;
                    socklen_t len = sizeof(val);
                    if (getsockopt(sockfd, IPPROTO_IP, IP_MTU, &val, &len) == SOCKET_ERROR)
                        return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                    if (len != sizeof(val)) /* sanity check */
                        return CLARINET_ESYS;

                    *(int32_t*)optval = (int32_t)val;
                    *optlen = sizeof(int32_t);

                    return CLARINET_ENONE;
                }

                #if CLARINET_ENABLE_IPV6
                if (family == CLARINET_AF_INET6)
                {
                    int val;
                    socklen_t len = sizeof(val);
                    if (getsockopt(sockfd, IPPROTO_IPV6, IPV6_MTU, &val, &len) == SOCKET_ERROR)
                        return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                    *(int32_t*)optval = (int32_t)val;
                    *optlen = sizeof(int32_t);

                    return CLARINET_ENONE;
                }
                #endif /* CLARINET_ENABLE_IPV6 */
            }
            break;
        case CLARINET_IP_MTU_DISCOVER:
            if (*optlen >= sizeof(int32_t))
            {
                int32_t* optret = (int32_t*)optval;

                const int family = socket->family;
                if (family == CLARINET_AF_INET)
                {
                    int val;
                    socklen_t len = sizeof(val);
                    if (getsockopt(sockfd, IPPROTO_IP, IP_MTU_DISCOVER, &val, &len) == SOCKET_ERROR)
                        return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                    if (len != sizeof(val)) /* sanity check */
                        return CLARINET_ESYS;

                    switch (val)
                    {
                        case IP_PMTUDISC_WANT:
                            *optret = CLARINET_PMTUD_UNSPEC;
                            break;
                        case IP_PMTUDISC_DO:
                            *optret = CLARINET_PMTUD_ON;
                            break;
                        case IP_PMTUDISC_DONT:
                            #if defined(__linux__) && defined(IP_PMTUDISC_OMIT)
                        case IP_PMTUDISC_OMIT:
                            #endif
                            #if defined(__linux__) && defined(IP_PMTUDISC_INTERFACE)
                        case IP_PMTUDISC_INTERFACE:
                            #endif
                            *optret = CLARINET_PMTUD_OFF;
                            break;
                        case IP_PMTUDISC_PROBE:
                            *optret = CLARINET_PMTUD_PROBE;
                            break;
                        default:
                            return CLARINET_ESYS;
                    }

                    *optlen = sizeof(int32_t);
                    return CLARINET_ENONE;
                }

                #if CLARINET_ENABLE_IPV6
                if (family == CLARINET_AF_INET6)
                {
                    int val;
                    socklen_t len = sizeof(val);
                    if (getsockopt(sockfd, IPPROTO_IPV6, IPV6_MTU_DISCOVER, &val, &len) == SOCKET_ERROR)
                        return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                    switch (val)
                    {
                        case IPV6_PMTUDISC_WANT:
                            *optret = CLARINET_PMTUD_UNSPEC;
                            break;
                        case IPV6_PMTUDISC_DO:
                            *optret = CLARINET_PMTUD_ON;
                            break;
                        case IPV6_PMTUDISC_DONT:
                            #if defined(__linux__) && defined(IPV6_PMTUDISC_OMIT)
                        case IPV6_PMTUDISC_OMIT:
                            #endif
                            #if defined(__linux__) && defined(IPV6_PMTUDISC_INTERFACE)
                        case IPV6_PMTUDISC_INTERFACE:
                            #endif
                            *optret = CLARINET_PMTUD_OFF;
                            break;
                        case IPV6_PMTUDISC_PROBE:
                            *optret = CLARINET_PMTUD_PROBE;
                            break;
                        default:
                            return CLARINET_ESYS;
                    }

                    *optlen = sizeof(int32_t);
                    return CLARINET_ENONE;
                }
                #endif /* CLARINET_ENABLE_IPV6 */
            }
            break;
        default:
            break;
    }

    return CLARINET_EINVAL;
}

int
clarinet_socket_connect(clarinet_socket* restrict socket,
                        clarinet_endpoint* restrict remote)
{
    if (!socket || socket->family == CLARINET_AF_UNSPEC || socket->proto == CLARINET_PROTO_NONE || !remote)
        return CLARINET_EINVAL;

    /* For now, only CLARINET_PROTO_UDP and CLARINET_PROTO_TCP are supported.
     * We could rely on the syscall to return an error, but it's more efficient to check the condition here. */
    if (socket->proto != CLARINET_PROTO_UDP && socket->proto != CLARINET_PROTO_TCP)
        return CLARINET_EPROTONOSUPPORT;

    struct sockaddr_storage ss;
    socklen_t sslen;
    const int errcode = clarinet_endpoint_to_sockaddr(&ss, &sslen, remote);
    if (errcode != CLARINET_ENONE)
        return errcode;

    int sockfd = socket->u.fd;
    if (connect(sockfd, (struct sockaddr*)&ss, sslen) == SOCKET_ERROR)
        return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

    return CLARINET_ENONE;
}

int
clarinet_socket_listen(clarinet_socket* socket,
                       int backlog)
{
    if (!socket || socket->family == CLARINET_AF_UNSPEC || socket->proto == CLARINET_PROTO_NONE)
        return CLARINET_EINVAL;

    /* For now, only CLARINET_PROTO_TCP is supported.
     * We could rely on the syscall to return an error, but it's more efficient to check the condition here. */
    if (socket->proto != CLARINET_PROTO_TCP)
        return CLARINET_EPROTONOSUPPORT;

    int sockfd = socket->u.fd;
    if (listen(sockfd, backlog) == SOCKET_ERROR)
        return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

    return CLARINET_ENONE;
}

int
clarinet_socket_accept(clarinet_socket* restrict server,
                       clarinet_socket* restrict client,
                       clarinet_endpoint* restrict remote)
{
    if (!server || server->family == CLARINET_AF_UNSPEC || server->proto == CLARINET_PROTO_NONE
        || !client || client->family != CLARINET_AF_UNSPEC || client->proto != CLARINET_PROTO_NONE
        || !remote)
        return CLARINET_EINVAL;

    /* For now, only CLARINET_PROTO_TCP is supported.
     * We could rely on the syscall to return an error, but it's more efficient to check the condition here. */
    if (server->proto != CLARINET_PROTO_TCP)
        return CLARINET_EPROTONOSUPPORT;

    struct sockaddr_storage ss;
    socklen_t slen = sizeof(ss);

    int serverfd = server->u.fd;
    int clientfd = accept(serverfd, (struct sockaddr*)&ss, &slen);
    if (clientfd == INVALID_SOCKET)
        return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

    const int errcode = clarinet_endpoint_from_sockaddr(remote, &ss);
    /* If the remote address cannot be decoded leave it unspecified but don't abort the operation. Leave it up to the 
     * the user to decide. Perhaps the remote address is not important. */
    if (errcode != CLARINET_ENONE)
        memset(remote, 0, sizeof(clarinet_endpoint));

    return CLARINET_ENONE;
}

int
clarinet_socket_shutdown(clarinet_socket* restrict socket,
                         int flags)
{
    if (!socket || socket->family == CLARINET_AF_UNSPEC || socket->proto == CLARINET_PROTO_NONE)
        return CLARINET_EINVAL;

    /* For now, only CLARINET_PROTO_TCP and CLARINET_PROTO_UDP are supported.
     * We could rely on the syscall to return an error, but it's more efficient to check the condition here. */
    if (socket->proto != CLARINET_PROTO_TCP && socket->proto != CLARINET_PROTO_UDP)
        return CLARINET_EPROTONOSUPPORT;

    int sflags = 0;
    if (flags & CLARINET_SD_RECV)
    {
        flags &= ~CLARINET_SD_RECV;
        sflags |= SHUT_RD;
    }

    if (flags & CLARINET_SD_SEND)
    {
        flags &= ~CLARINET_SD_SEND;
        sflags |= SHUT_WR;
    }

    if (flags)
        return CLARINET_EINVAL;

    int sockfd = socket->u.fd;
    if (shutdown(sockfd, sflags) == SOCKET_ERROR)
        return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

    return CLARINET_ENONE;
}

/* endregion */
