#include "compat/compat.h"
#include "clarinet/clarinet.h"

#include "compat/addr.h"
#include "compat/error.h"

#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <poll.h>

/* region Library Initialization */

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
again(int e)
{
    #if HAVE_EAGAIN_EQUAL_TO_EWOULDBLOCK
    return (e == EWOULDBLOCK);
    #else
    return (e == EWOULDBLOCK) || (e == EAGAIN);
    #endif /* HAVE_EAGAIN_EQUAL_TO_EWOULDBLOCK */
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

/* endregion */

/* region Socket */

/**
 * Returns true (non-zero) if the socket pointed to by @p s is valid. All negative descriptors are invalid and values
 * 0, 1 and 2 are reserved for stdin, stdout and stderr respectively so valid descriptors start at 3.
 */
#define clarinet_socket_handle_is_valid(s)      ((s)->handle > 2)

/** Returns a handle for the socket pointed to by @p s */
#define clarinet_socket_handle(s)               ((s)->handle)

/**
 * Check the socket is of the expected type. This is used to ensure an option can only be get/set with a certain socket
 * type. Some platforms allow certain options to be get/set even when not applicable (e.g. Linux) but other platforms
 * (e.g Windows) are more restrictive. Since we cannot modify a platform to be more tolerant, the only alternative is to
 * make the socket get/set functions equaly strict.
 */
#define CLARINET_SOCKET_CHECK_TYPE(S, V, L, T) do { \
    if (getsockopt((S), SOL_SOCKET, SO_TYPE, &(V), &(L)) == SOCKET_ERROR) \
        return CLARINET_ESYS; \
    if ((L) != sizeof(V)) /* sanity check */ \
        return CLARINET_ESYS; \
    if ((V) != (T)) \
        return CLARINET_EPROTONOSUPPORT; \
} while(0)

void
clarinet_socket_init(clarinet_socket* sp)
{
    memset(sp, 0, sizeof(clarinet_socket));
}

int
clarinet_socket_open(clarinet_socket* sp,
                     int family,
                     int proto)
{
    /* On Unix, file descriptors 0, 1 and 2 are reserved for stdin, stdout and stderr respectively and will never be
     * valid in a sp structure. We can safely assume that any non-zero descriptor indicates that the sp
     * structure is either already in use or uninitialized. */
    if (!sp || sp->family != CLARINET_AF_UNSPEC || clarinet_socket_handle(sp) != 0)
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
        if (sfamily == AF_INET
            && setsockopt(sockfd, SOL_SOCKET, SO_NO_CHECK, (const void*)&off, sizeof(off)) == SOCKET_ERROR
            && clarinet_get_sockapi_error() != ENOPROTOOPT)
        {
            close(sockfd); /* there's nothing we can do (or the user) if close fails here... */
            return CLARINET_ESYS;
        }
        #endif /* defined(SO_NO_CHECK) */

        #if defined(UDP_NOCKSUM)
        /* macOS has an undocumented flag to enable/disable UDP checksum on IPv4 but we want UDP checksums even if for
         * some reason it was disabled in the system as there is no portable option for the user to set it. */
        if (sfamily == AF_INET
            && setsockopt(sockfd, IPPROTO_UDP, UDP_NOCKSUM, (const void*)&off, sizeof(off)) == SOCKET_ERROR
            && clarinet_get_sockapi_error() != ENOPROTOOPT)
        {
            close(sockfd); /* there's nothing we can do (or the user) if close fails here... */
            return CLARINET_ESYS;
        }
        #endif /* defined(UDP_NOCKSUM) */
    }

    sp->family = (uint16_t)family;
    sp->handle = sockfd;

    return CLARINET_ENONE;
}

int
clarinet_socket_close(clarinet_socket* sp)
{
    if (!sp || sp->family == CLARINET_AF_UNSPEC)
        return CLARINET_EINVAL;

    if (!clarinet_socket_handle_is_valid(sp))
        return CLARINET_EINVAL;

    const int sockfd = clarinet_socket_handle(sp);

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
     * change the sp to blocking and close again. We never force SO_LINGER(0) here because it is not clear what
     * the user intentions are. There is no point in having a boolean argument for that either (e.g.: force) because the
     * user can always adjust SO_LINGER or use the shorcut option SO_DONTLINGER at any time before closing the sp.
     */
    if (close(sockfd) == SOCKET_ERROR)
    {
        int err = clarinet_get_sockapi_error();
        if (!again(err))
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
            if (!again(err))
                return clarinet_error_from_sockapi_error(err);

            sleep(1);
        }
    }

    clarinet_socket_init(sp);

    return CLARINET_ENONE;
}

int
clarinet_socket_bind(clarinet_socket* restrict sp,
                     const clarinet_endpoint* restrict local)
{
    if (!sp || sp->family == CLARINET_AF_UNSPEC || !local)
        return CLARINET_EINVAL;

    if (!clarinet_socket_handle_is_valid(sp))
        return CLARINET_EINVAL;

    const int sockfd = clarinet_socket_handle(sp);

    struct sockaddr_storage ss;
    socklen_t sslen;
    const int errcode = clarinet_endpoint_to_sockaddr(&ss, &sslen, local);
    if (errcode != CLARINET_ENONE)
        return errcode;

    if (bind(sockfd, (struct sockaddr*)&ss, sslen) == SOCKET_ERROR)
        return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

    return CLARINET_ENONE;
}

int
clarinet_socket_local_endpoint(clarinet_socket* restrict sp,
                               clarinet_endpoint* restrict local)
{
    if (!sp || sp->family == CLARINET_AF_UNSPEC || !local)
        return CLARINET_EINVAL;

    if (!clarinet_socket_handle_is_valid(sp))
        return CLARINET_EINVAL;

    const int sockfd = clarinet_socket_handle(sp);

    struct sockaddr_storage ss = { 0 };
    socklen_t length = sizeof(ss);

    if (getsockname(sockfd, (struct sockaddr*)&ss, &length) == SOCKET_ERROR)
        return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

    clarinet_endpoint endpoint = { { 0 } };
    int errcode = clarinet_endpoint_from_sockaddr(&endpoint, &ss);
    if (errcode == CLARINET_ENONE)
    {
        /* On Unix systems it is not an error to call getsockname() on a sp that has not been bound to a local
         * endpoint yet. The address returned in this case is the wildcard (0.0.0.0 or ::) with port=0. The address
         * alone is not really important because a sp can in fact be bound to the wildcard address but the local
         * port of a properly bound sp can never be 0. */
        if (endpoint.port == 0)
            return CLARINET_EINVAL;

        *local = endpoint;
    }

    return errcode;
}

int
clarinet_socket_remote_endpoint(clarinet_socket* restrict sp,
                                clarinet_endpoint* restrict remote)
{
    if (!sp || sp->family == CLARINET_AF_UNSPEC || !remote)
        return CLARINET_EINVAL;

    if (!clarinet_socket_handle_is_valid(sp))
        return CLARINET_EINVAL;

    const int sockfd = clarinet_socket_handle(sp);

    struct sockaddr_storage ss = { 0 };
    socklen_t length = sizeof(ss);

    if (getpeername(sockfd, (struct sockaddr*)&ss, &length) == SOCKET_ERROR)
        return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

    return clarinet_endpoint_from_sockaddr(remote, &ss);
}

int
clarinet_socket_send(clarinet_socket* restrict sp,
                     const void* restrict buf,
                     size_t buflen)
{
    if (!sp || sp->family == CLARINET_AF_UNSPEC || (!buf && buflen > 0) || buflen > INT_MAX)
        return CLARINET_EINVAL;

    if (!clarinet_socket_handle_is_valid(sp))
        return CLARINET_EINVAL;

    const int sockfd = clarinet_socket_handle(sp);

    #if defined(__linux__)
    /**
     * @c MSG_NOSIGNAL (since Linux 2.2) Requests not to send @c SIGPIPE on errors on stream oriented sockets when the
     * other end breaks the connection. The @c EPIPE error is still returned.
     */
    const int flags = MSG_NOSIGNAL;
    #else
    const int flags = 0;
    #endif
    if (send(sockfd, buf, buflen, flags) == SOCKET_ERROR)
        return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

    return CLARINET_ENONE;
}

int
clarinet_socket_sendto(clarinet_socket* restrict sp,
                       const void* restrict buf,
                       size_t buflen,
                       const clarinet_endpoint* restrict remote)
{
    if (!sp || sp->family == CLARINET_AF_UNSPEC || (!buf && buflen > 0) || buflen > INT_MAX || !remote)
        return CLARINET_EINVAL;

    if (!clarinet_socket_handle_is_valid(sp))
        return CLARINET_EINVAL;

    const int sockfd = clarinet_socket_handle(sp);

    struct sockaddr_storage ss;
    socklen_t sslen;
    const int errcode = clarinet_endpoint_to_sockaddr(&ss, &sslen, remote);
    if (errcode != CLARINET_ENONE)
        return errcode;

    #if defined(__linux__)
    /**
     * @c MSG_NOSIGNAL (since Linux 2.2) Requests not to send @c SIGPIPE on errors on stream oriented sockets when the
     * other end breaks the connection. The @c EPIPE error is still returned.
     */
    const int flags = MSG_NOSIGNAL;
    #else
    const int flags = 0;
    #endif

    const ssize_t n = sendto(sockfd, buf, buflen, flags, (struct sockaddr*)&ss, sslen);
    if (n < 0)
        return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

    return (int)n;
}

int
clarinet_socket_recv(clarinet_socket* restrict sp,
                     void* restrict buf,
                     size_t buflen)
{
    if (!sp || sp->family == CLARINET_AF_UNSPEC || !buf || buflen == 0 || buflen > INT_MAX)
        return CLARINET_EINVAL;

    if (!clarinet_socket_handle_is_valid(sp))
        return CLARINET_EINVAL;

    const int sockfd = clarinet_socket_handle(sp);

    const ssize_t n = recv(sockfd, buf, buflen, 0);
    if (n < 0)
        return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

    return (int)n;
}

int
clarinet_socket_recvfrom(clarinet_socket* restrict sp,
                         void* restrict buf,
                         size_t buflen,
                         clarinet_endpoint* restrict remote)
{
    if (!sp || sp->family == CLARINET_AF_UNSPEC || !buf || buflen == 0 || buflen > INT_MAX || !remote)
        return CLARINET_EINVAL;

    if (!clarinet_socket_handle_is_valid(sp))
        return CLARINET_EINVAL;

    const int sockfd = clarinet_socket_handle(sp);

    struct sockaddr_storage ss;
    const socklen_t sslen = sizeof(ss);

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
    msg.msg_namelen = sslen;

    iov.iov_base = buf;
    iov.iov_len = buflen;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

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

    assert(n >= 0);
    return (int)n;
}

int
clarinet_socket_setopt(clarinet_socket* restrict sp,
                       int optname,
                       const void* restrict optval,
                       size_t optlen)
{
    if (!sp || sp->family == CLARINET_AF_UNSPEC || !optval || optlen > INT_MAX)
        return CLARINET_EINVAL;

    if (!clarinet_socket_handle_is_valid(sp))
        return CLARINET_EINVAL;

    const int sockfd = clarinet_socket_handle(sp);

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
                if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT_LB, &val, sizeof(val)) == SOCKET_ERROR)
                    return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());
                #elif defined(SO_REUSEPORT)
                /* SO_REUSEPORT is only supported on Linux >= 3.9 but also promotes load balancing on UDP - may be
                 * present on some patched 2.6 kernels (e.g. REHL 2.6.32) */
                if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &val, sizeof(val)) == SOCKET_ERROR)
                    return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());
                #endif /* defined(SO_REUSEPORT) */

                #if defined(SO_EXCLBIND)
                /* Solaris supports SO_EXCLBIND similarly to SO_EXCLUSIVEADDRUSE on Windows */
                if (setsockopt(sockfd, SOL_SOCKET, SO_EXCLBIND, &val, sizeof(val)) == SOCKET_ERROR)
                    return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());
                #endif /* defined(SO_EXCLBIND) */

                return CLARINET_ENONE;
            }
            break;
        case CLARINET_SO_SNDBUF:
            if (optlen == sizeof(int32_t))
            {
                #if defined(__linux__)
                /* Linux doubles the buffer size passed to setsockopt(2), so we half it here to more closely match
                 * how bufffer sizes are set in other platforms. The effect is that odd numbers are rounded down
                 * to the closest even number. */
                const int val = (int)clamp(*(const int32_t*)optval, 0, INT_MAX) >> 1;
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
                const int val = (int)clamp(*(const int32_t*)optval, 0, INT_MAX) >> 1;
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
                ms2tv(&val, (int)clamp(*(const int32_t*)optval, 0, INT_MAX));
                if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (const void*)&val, sizeof(val)) == SOCKET_ERROR)
                    return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                return CLARINET_ENONE;
            }
            break;
        case CLARINET_SO_RCVTIMEO:
            if (optlen == sizeof(int32_t))
            {
                struct timeval val;
                ms2tv(&val, (int)clamp(*(const int32_t*)optval, 0, INT_MAX));
                if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const void*)&val, sizeof(val)) == SOCKET_ERROR)
                    return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                return CLARINET_ENONE;
            }
            break;
        case CLARINET_SO_KEEPALIVE:
            if (optlen == sizeof(int32_t))
            {
                int val = 0;
                socklen_t len = sizeof(val);

                CLARINET_SOCKET_CHECK_TYPE(sockfd, val, len, SOCK_STREAM);

                val = *(const int32_t*)optval ? 1 : 0;
                if (setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val)) == SOCKET_ERROR)
                    return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                return CLARINET_ENONE;
            }
            break;
        case CLARINET_SO_LINGER:
            if (optlen == sizeof(clarinet_linger))
            {
                int val = 0;
                socklen_t len = sizeof(val);

                CLARINET_SOCKET_CHECK_TYPE(sockfd, val, len, SOCK_STREAM);

                const clarinet_linger* optret = (const clarinet_linger*)optval;
                struct linger linger;
                linger.l_onoff = optret->enabled;
                linger.l_linger = optret->seconds;
                if (setsockopt(sockfd, SOL_SOCKET, SO_LINGER, (const void*)&linger, sizeof(linger)) == SOCKET_ERROR)
                    return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                return CLARINET_ENONE;
            }
            break;
        case CLARINET_SO_DONTLINGER:
            if (optlen == sizeof(int32_t))
            {
                int val = 0;
                socklen_t len = sizeof(val);

                CLARINET_SOCKET_CHECK_TYPE(sockfd, val, len, SOCK_STREAM);

                struct linger linger;
                len = sizeof(linger);
                if (getsockopt(sockfd, SOL_SOCKET, SO_LINGER, (void*)&linger, &len) == SOCKET_ERROR)
                    return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                if (len != sizeof(linger)) /* sanity check */
                    return CLARINET_ESYS;

                linger.l_onoff = *(const int32_t*)optval ? 0 : 1;
                if (setsockopt(sockfd, SOL_SOCKET, SO_LINGER, (const void*)&linger, sizeof(linger)) == SOCKET_ERROR)
                    return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                return CLARINET_ENONE;
            }
            break;
        case CLARINET_IP_V6ONLY:
            #if CLARINET_ENABLE_IPV6
            if (optlen == sizeof(int32_t))
            {
                const int family = sp->family;
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
                const int family = sp->family;
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
            break;
        case CLARINET_IP_MTU_DISCOVER:
            if (optlen == sizeof(int32_t))
            {
                const int family = sp->family;
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
            break;
        case CLARINET_IP_BROADCAST:
            if (optlen == sizeof(int32_t))
            {
                int val = 0;
                socklen_t len = sizeof(val);

                CLARINET_SOCKET_CHECK_TYPE(sockfd, val, len, SOCK_DGRAM);

                val = *(const int32_t*)optval ? 1 : 0;
                if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &val, sizeof(val)) == SOCKET_ERROR)
                    return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                return CLARINET_ENONE;
            }
            break;
        default:
            break;
    }

    return CLARINET_EINVAL;
}

int
clarinet_socket_getopt(clarinet_socket* restrict sp,
                       int optname,
                       void* restrict optval,
                       size_t* restrict optlen)
{
    if (!sp || sp->family == CLARINET_AF_UNSPEC || !optval || !optlen || *optlen > INT_MAX)
        return CLARINET_EINVAL;

    if (!clarinet_socket_handle_is_valid(sp))
        return CLARINET_EINVAL;

    const int sockfd = clarinet_socket_handle(sp);

    switch (optname)
    {
        case CLARINET_SO_REUSEADDR:
            if (*optlen >= sizeof(int32_t))
            {
                int val = 0;
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
                int val = 0;
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
                int val = 0;
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
                struct timeval val = { 0 };
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
                struct timeval val = { 0 };
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
                int val = 0;
                socklen_t len = sizeof(val);

                CLARINET_SOCKET_CHECK_TYPE(sockfd, val, len, SOCK_STREAM);

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
                int val = 0;
                socklen_t len = sizeof(val);

                CLARINET_SOCKET_CHECK_TYPE(sockfd, val, len, SOCK_STREAM);

                struct linger linger = { 0 };
                len = sizeof(linger);
                if (getsockopt(sockfd, SOL_SOCKET, SO_LINGER, (void*)&linger, &len) == SOCKET_ERROR)
                    return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                if (len != sizeof(linger)) /* sanity check */
                    return CLARINET_ESYS;

                clarinet_linger* optret = (clarinet_linger*)optval;
                optret->enabled = linger.l_onoff ? 1 : 0;
                optret->seconds = (uint16_t)clamp(linger.l_linger, 0, UINT16_MAX);
                *optlen = sizeof(clarinet_linger);

                return CLARINET_ENONE;
            }
            break;
        case CLARINET_SO_DONTLINGER:
            if (*optlen >= sizeof(int32_t))
            {
                int val = 0;
                socklen_t len = sizeof(val);

                CLARINET_SOCKET_CHECK_TYPE(sockfd, val, len, SOCK_STREAM);

                struct linger linger = { 0 };
                len = sizeof(linger);
                if (getsockopt(sockfd, SOL_SOCKET, SO_LINGER, (void*)&linger, &len) == SOCKET_ERROR)
                    return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                if (len != sizeof(linger)) /* sanity check */
                    return CLARINET_ESYS;

                *(int32_t*)optval = linger.l_onoff ? 0 : 1;
                *optlen = sizeof(int32_t);
                return CLARINET_ENONE;
            }
            break;
        case CLARINET_SO_ERROR:
            if (*optlen >= sizeof(int32_t))
            {
                int val = 0;
                socklen_t len = sizeof(val);
                if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &val, &len) == SOCKET_ERROR)
                    return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                if (len != sizeof(val)) /* sanity check */
                    return CLARINET_ESYS;

                *(int32_t*)optval = (int32_t)clarinet_error_from_sockapi_error(val);
                *optlen = sizeof(int32_t);

                return CLARINET_ENONE;
            }
            break;
        case CLARINET_IP_V6ONLY:
            #if CLARINET_ENABLE_IPV6
            if (*optlen >= sizeof(int32_t))
            {
                const int family = sp->family;
                if (family == CLARINET_AF_INET6)
                {
                    int val = 0;
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
                int val = 0;
                socklen_t len = sizeof(val);

                const int family = sp->family;
                if (family == CLARINET_AF_INET)
                {
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
                int val = 0;
                socklen_t len = sizeof(val);

                const int family = sp->family;
                if (family == CLARINET_AF_INET)
                {
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
                    if (getsockopt(sockfd, IPPROTO_IPV6, IPV6_MTU, &val, &len) == SOCKET_ERROR)
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
        case CLARINET_IP_MTU_DISCOVER:
            if (*optlen >= sizeof(int32_t))
            {
                int32_t* optret = (int32_t*)optval;
                int val = 0;
                socklen_t len = sizeof(val);

                const int family = sp->family;
                if (family == CLARINET_AF_INET)
                {
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
                    if (getsockopt(sockfd, IPPROTO_IPV6, IPV6_MTU_DISCOVER, &val, &len) == SOCKET_ERROR)
                        return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                    if (len != sizeof(val)) /* sanity check */
                        return CLARINET_ESYS;

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
        case CLARINET_IP_BROADCAST:
            if (*optlen >= sizeof(int32_t))
            {
                int val = 0;
                socklen_t len = sizeof(val);

                CLARINET_SOCKET_CHECK_TYPE(sockfd, val, len, SOCK_DGRAM);

                if (getsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &val, &len) == SOCKET_ERROR)
                    return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                if (len != sizeof(val)) /* sanity check */
                    return CLARINET_ESYS;

                *(int32_t*)optval = (int32_t)val;
                *optlen = sizeof(int32_t);

                return CLARINET_ENONE;
            }
            break;
        default:
            break;
    }

    return CLARINET_EINVAL;
}

int
clarinet_socket_connect(clarinet_socket* restrict sp,
                        const clarinet_endpoint* restrict remote)
{
    if (!sp || sp->family == CLARINET_AF_UNSPEC || !remote)
        return CLARINET_EINVAL;

    if (!clarinet_socket_handle_is_valid(sp))
        return CLARINET_EINVAL;

    const int sockfd = clarinet_socket_handle(sp);

    struct sockaddr_storage ss;
    socklen_t sslen;
    const int errcode = clarinet_endpoint_to_sockaddr(&ss, &sslen, remote);
    if (errcode != CLARINET_ENONE)
        return errcode;

    if (connect(sockfd, (struct sockaddr*)&ss, sslen) == SOCKET_ERROR)
        return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

    return CLARINET_ENONE;
}

int
clarinet_socket_listen(clarinet_socket* sp,
                       int backlog)
{
    if (!sp || sp->family == CLARINET_AF_UNSPEC || backlog < 0)
        return CLARINET_EINVAL;

    if (!clarinet_socket_handle_is_valid(sp))
        return CLARINET_EINVAL;

    const int sockfd = clarinet_socket_handle(sp);

    if (listen(sockfd, backlog) == SOCKET_ERROR)
    {
        const int err = clarinet_get_sockapi_error();
        /* If the sp type/proto is not compatible the system returns EOPNOTSUPP but since we automatically derive
         * type from proto (STREAM/TCP, STREAM/SCTP, DGRAM/UDP) we want to return CLARINET_EPROTONOSUPPORT instead
         * of CLARINET_ENOTSUP here. */
        return (err == EOPNOTSUPP) ? CLARINET_EPROTONOSUPPORT : clarinet_error_from_sockapi_error(err);
    }

    return CLARINET_ENONE;
}

int
clarinet_socket_accept(clarinet_socket* restrict ssp,
                       clarinet_socket* restrict csp,
                       clarinet_endpoint* restrict remote)
{
    if (!ssp || ssp->family == CLARINET_AF_UNSPEC || !csp || csp->family != CLARINET_AF_UNSPEC || !remote)
        return CLARINET_EINVAL;

    if (!clarinet_socket_handle_is_valid(ssp))
        return CLARINET_EINVAL;

    const int serverfd = clarinet_socket_handle(ssp);

    struct sockaddr_storage ss;
    socklen_t sslen = sizeof(ss);

    int clientfd = accept(serverfd, (struct sockaddr*)&ss, &sslen);
    if (clientfd == INVALID_SOCKET)
    {
        const int err = clarinet_get_sockapi_error();
        /* If the socket type/proto is not compatible the system returns EOPNOTSUPP but since we automatically derive
         * type from proto (STREAM/TCP, STREAM/SCTP, DGRAM/UDP) we want to return CLARINET_EPROTONOSUPPORT instead
         * of CLARINET_ENOTSUP here. */
        return (err == EOPNOTSUPP) ? CLARINET_EPROTONOSUPPORT : clarinet_error_from_sockapi_error(err);
    }

    csp->family = ssp->family;
    csp->handle = clientfd;

    const int errcode = clarinet_endpoint_from_sockaddr(remote, &ss);
    /* If the remote address cannot be decoded leave it unspecified but don't abort the operation. Leave it up to the 
     * user to decide. Perhaps the remote address is not important. */
    if (errcode != CLARINET_ENONE)
    {
        memset(remote, 0, sizeof(clarinet_endpoint));
        return CLARINET_EADDRNOTAVAIL;
    }

    return CLARINET_ENONE;
}

int
clarinet_socket_shutdown(clarinet_socket* restrict sp,
                         int flags)
{
    if (!sp || sp->family == CLARINET_AF_UNSPEC)
        return CLARINET_EINVAL;

    if (!clarinet_socket_handle_is_valid(sp))
        return CLARINET_EINVAL;

    const int sockfd = clarinet_socket_handle(sp);

    int sflags = 0;
    if (flags & CLARINET_SHUTDOWN_RECV)
    {
        flags &= ~CLARINET_SHUTDOWN_RECV;
        sflags |= SHUT_RD;
    }

    if (flags & CLARINET_SHUTDOWN_SEND)
    {
        flags &= ~CLARINET_SHUTDOWN_SEND;
        sflags |= SHUT_WR;
    }

    if (flags)
        return CLARINET_EINVAL;

    if (shutdown(sockfd, sflags) == SOCKET_ERROR)
        return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

    return CLARINET_ENONE;
}

int
clarinet_socket_poll_context_calcsize(size_t count)
{
    if (count > (INT_MAX / sizeof(struct pollfd)))
        return CLARINET_EINVAL;

    return (int)(count * sizeof(struct pollfd));
}

int
clarinet_socket_poll_context_getstatus(const void* restrict context,
                                       size_t index,
                                       uint16_t* restrict status)
{
    if (!context || index >= INT_MAX || !status)
        return CLARINET_EINVAL;

    const struct pollfd* pfd = (struct pollfd*)context;
    *status = (uint16_t)pfd[index].revents;

    return CLARINET_ENONE;
}

int
clarinet_socket_poll(void* restrict context,
                     const clarinet_socket_poll_target* restrict targets,
                     size_t count,
                     int timeout)
{
    if (!context || !targets || count == 0 || count > INT_MAX)
        return CLARINET_EINVAL;

    if (poll((struct pollfd*)context, (nfds_t)count, timeout) == SOCKET_ERROR)
        return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

    return CLARINET_ENONE;
}

/* endregion */
