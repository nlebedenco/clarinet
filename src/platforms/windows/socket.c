#include "compat/compat.h"
#include "clarinet/clarinet.h"

#include "compat/addr.h"
#include "compat/error.h"

#include <synchapi.h>
#include <assert.h>

#if !defined(SIO_UDP_CONNRESET)
#define SIO_UDP_CONNRESET (IOC_IN | IOC_VENDOR | 12)
#endif

/* region Library Initialization */

/**
 * Lockless mutual exclusion between clarinet_initialize() and clarinet_finalize() is acomplished with a mechanism
 * similar to a spin lock but using 3 states instead of 2 so the function can break early if another thread managed
 * to execute first.
 *
 * States are: FINALIZED(0), BUSY(1) and INITIALIZED(2)
 *
 * A call to clarinet_initialize() transitions from FINALIZED to INITIALIZED passing through BUSY while a call to
 * clarinet_finalize() transitions from INITIALIZED to FINALIZED also passing through BUSY. Either call will only
 * perform any work when actually triggering the transition to BUSY, otherwise it will just spin and wait for the state
 * owner to complete the current transition or break early.
 */
static LONG clarinet_static_init = 0;

#define CLARINET_SPIN_LIMIT 4096

int
clarinet_initialize(void)
{
    int cycles = 0;

    repeat:
    int prev = InterlockedCompareExchange(&clarinet_static_init, 1, 0);
    if (prev == 0)
    {
        /* Version 2.2. is the one for Windows Vista and later */
        const WORD wsa_version = MAKEWORD(2, 2);
        WSADATA wsa_data;
        /* WSAStartup returns an error directly, clarinet_get_sockapi_error() should not be used in this case. */
        int err = WSAStartup(wsa_version, &wsa_data);
        if (err != 0)
        {
            InterlockedDecrement(&clarinet_static_init);
            return clarinet_error_from_sockapi_error(err);
        }

        InterlockedIncrement(&clarinet_static_init);
    }
    else if (prev == 1)
    {
        cycles++;
        if (cycles == CLARINET_SPIN_LIMIT)
        {
            cycles = 0;
            Sleep(0); /* yield */
        }

        goto repeat;
    }

    return CLARINET_ENONE;
}

int
clarinet_finalize(void)
{
    int cycles = 0;

    repeat:
    int prev = InterlockedCompareExchange(&clarinet_static_init, 1, 2);
    if (prev == 2)
    {
        int err = WSACleanup();
        if (err != 0 && err != WSANOTINITIALISED)
        {
            InterlockedIncrement(&clarinet_static_init);
            return clarinet_error_from_sockapi_error(err);
        }

        InterlockedDecrement(&clarinet_static_init);
    }
    else if (prev == 1)
    {
        cycles++;
        if (cycles == CLARINET_SPIN_LIMIT)
        {
            cycles = 0;
            Sleep(0); /* yield */
        }

        goto repeat;
    }

    return CLARINET_ENONE;


}

/* endregion */

/* region Helpers */


/* Helper for setting the UDP socket to NOT reset when an ICMP Type3 Code3 (Port unreachable) packet is received */
CLARINET_STATIC_INLINE
int
setudpconnreset(SOCKET sock,
                DWORD value)
{
    return ioctlsocket(sock, SIO_UDP_CONNRESET, &value);
}

/** Helper for setting the socket blocking value.*/
CLARINET_STATIC_INLINE
int
setnonblock(SOCKET sockfd,
            DWORD value)
{
    return ioctlsocket(sockfd, FIONBIO, &value);
}

/* endregion */

/* region Socket */

/** Returns a handle for the socket pointed to by @p s */
#define clarinet_socket_handle(s)           ((SOCKET)((s)->handle))

/**
 * Returns true (non-zero) if the socket pointed to by @p s is valid, otherwise false (0). We only compare to 0 (NULL)
 * here because we never store an INVALID_SOCKET and Windows can already reject an eventual INVALID_SOCKET passed by
 * chance from a pointer to uninitialized memory.
 */
#define clarinet_socket_handle_is_valid(s)  (clarinet_socket_handle(s) != (SOCKET)0)

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
    /* On Windows, the SOCKET type is interchangeable with the HANDLE type which must have the same size and semantics
     * of void*. The documentation at https://docs.microsoft.com/en-us/windows/win32/winsock/socket-data-type-2 suggests
     * valid values for SOCKET include anything other than INVALID_SOCKET=(~0) but this is really an overstatement.
     * According to the article at https://docs.microsoft.com/en-us/windows/win32/sysinfo/kernel-objects, Windows has a
     * per-process limit of 2^24 for kernel handles which makes it safe to cast SOCKET to HANDLE, void*, intptr_t and
     * even int (assuming sizeof(int) >= sizeof(intptr_t)). The fact that INVALID_SOCKET has always been defined as ~0
     * seems to support the notion that casting from SOCKET to int and back should not cause loss of data. Furthermore,
     * since SOCKET and HANDLE must adhere to the same underlying semantics, NULL can also be considered an invalid
     * socket handle besides INVALID_SOCKET. The description of WSAPOLLFD found at
     * https://docs.microsoft.com/en-us/windows/win32/api/winsock2/ns-winsock2-wsapollfd also suggests SOCKET could
     * be cast to a signed integer and negative values would be consideed invalid. Raymond Chen from MS gives a good
     * overview on why Windows API treats handles so inconsistently in this article
     * https://devblogs.microsoft.com/oldnewthing/20040302-00/?p=40443. */
    if (!sp || sp->family != CLARINET_AF_UNSPEC || clarinet_socket_handle_is_valid(sp))
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

    SOCKET sockfd = socket(sfamily, sdomain, sproto);

    if (sockfd == (SOCKET)0) /* Sanity check: a valid socket should never be 0 because SOCKET and HANDLE should be interchangeable. */
    {
        closesocket(sockfd);
        return CLARINET_ESYS;
    }

    if (sockfd == INVALID_SOCKET)
        return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

    if (sproto == IPPROTO_UDP)
    {
        static const DWORD on = 1;
        static const DWORD off = 0;

        /* Set transparent socket options first. Operation must be aborted if anyone fails. */
        if (setsockopt(sockfd, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (const char*)&on, sizeof(on)) == SOCKET_ERROR
            || setudpconnreset(sockfd, off) == SOCKET_ERROR)
        {
            closesocket(sockfd); /* there's nothing we can do (or the user) if close fails here... */
            return CLARINET_ESYS;
        }

        /* UDP_CHECKSUM_COVERAGE is superficially described in
         * https://docs.microsoft.com/en-us/windows/win32/winsock/ipproto-udp-socket-options
         * but does not work on Windows 10 and probably neither on any other Windows version.
         * At first, one would expect it to set the checksum coverage for UDP-Lite support (RFC3828) with a value
         * in the range 0..65535 where 0 means the entire datagram is always covered and values from 1-7 (being
         * illegal per RFC 3828, Section 3.1) should be rounded up to the minimum coverage of 8. HOWEVER, in this case
         * the option should take an int and not a bool as documented... Damn you, Microsoft!
         */

        /* Ensure UDP checksum is not forced to zero in sent datagrams (does not affect ability to receive) */
        if (sfamily == AF_INET
            && setsockopt(sockfd, IPPROTO_UDP, UDP_NOCHECKSUM, (const char*)&off, sizeof(off)) == SOCKET_ERROR)
        {
            const int err = clarinet_get_sockapi_error();
            if (err != WSAENOPROTOOPT && err != WSAEINVAL)
            {
                closesocket(sockfd); /* there's nothing we can do (or the user) if close fails here... */
                return clarinet_error_from_sockapi_error(err);
            }
        }
    }

    sp->family = (uint16_t)family;
    sp->handle = (void*)sockfd;

    return CLARINET_ENONE;
}

int
clarinet_socket_close(clarinet_socket* sp)
{
    if (!sp || sp->family == CLARINET_AF_UNSPEC)
        return CLARINET_EINVAL;

    if (!clarinet_socket_handle_is_valid(sp))
        return CLARINET_EINVAL;

    const SOCKET sockfd = clarinet_socket_handle(sp);

    if (closesocket(sockfd) == SOCKET_ERROR)
    {
        int err = clarinet_get_sockapi_error();
        if (err == WSAEWOULDBLOCK)
        {
            /* closesocket(1) MAY fail with WSAEWOULDBLOCK when the sp is non-blocking, there is data to flush and
           * a non-zero linger timeout is active, so try to set the sp to blocking and close again. Do not force
           * SO_LINGER(0) here because it is not clear what the user intentions are. There is no point in having a
           * boolean argument for that either (e.g.: force) because the user can always adjust SO_LINGER or use the
           * shorcut option SO_DONTLINGER at any time before closing the sp. */
            if (setnonblock(sockfd, 1) == SOCKET_ERROR)
            {
                /* setsocknonblock() may fail if there's a pending WSAEventSelect so try canceling it first.
                 * Issuing a WSAEventSelect for a sp cancels any previous WSAAsyncSelect or WSAEventSelect for the
                 * same sp and clears the internal network event record. */
                err = clarinet_get_sockapi_error();
                if (err == WSAEFAULT)
                {
                    /* At this point, the error from WSAEventSelect or setsocknonblock is irrelevant. Just try to
                     * release any pending event select, set the sp to blocking and move on to repeatedly try to
                     * close until the sp stops asking for a retry. */
                    WSAEventSelect(sockfd, NULL, 0);
                    setnonblock(sockfd, 1);
                }
            }

            /* At this point if close is still failing with WSAEWOULDBLOCK we have no choice but to wait some time
             * and try again until the SO_LINGER timeout expires or the output buffer is flushed. We don't simply wait
             * for the whole SO_LINGER duration because the output buffer could be flushed way before that. This a
             * safety mechanism because none of this should normally happen. */
            while (1)
            {
                if (closesocket(sockfd) == SOCKET_ERROR)
                {
                    err = clarinet_get_sockapi_error();
                    if (err != WSAEWOULDBLOCK)
                        break;

                    Sleep(1000);
                }
                else
                {
                    clarinet_socket_init(sp);
                    return CLARINET_ENONE;
                }
            }
        }

        assert(err != 0);
        assert(err != WSAEWOULDBLOCK);

        return clarinet_error_from_sockapi_error(err);
    }

    clarinet_socket_init(sp);
    return CLARINET_ENONE;
}


int
clarinet_socket_bind(clarinet_socket* restrict socket,
                     const clarinet_endpoint* restrict local)
{
    if (!socket || socket->family == CLARINET_AF_UNSPEC || !local)
        return CLARINET_EINVAL;

    if (!clarinet_socket_handle_is_valid(socket))
        return CLARINET_EINVAL;

    const SOCKET sockfd = clarinet_socket_handle(socket);

    struct sockaddr_storage ss = { 0 };
    socklen_t sslen = 0;
    const int errcode = clarinet_endpoint_to_sockaddr(&ss, &sslen, local);
    if (errcode != CLARINET_ENONE)
        return errcode;

    if (bind(sockfd, (struct sockaddr*)&ss, sslen) == SOCKET_ERROR)
    {
        int err = clarinet_get_sockapi_error();
        /* WSAEACCESS could only have been returned by bind() if the address is in effect in use by another socket
         * with SO_EXCLUSIVEADDRUSE or the address is a broadcast address but SO_BROADCAST was not enabled. In the
         * first case we would like to return CLARINET_EADDRINUSE instead of CLARINET_EACCESS because this is what
         * Unix platforms return where CLARINET_SO_REUSEADDR is defined as semantically equivalent to
         * [SO_REUSEADDR|SO_REUSEPORT] */
        if (err == WSAEACCES && !clarinet_addr_is_broadcast_ip(&local->addr))
            err = WSAEADDRINUSE;

        return clarinet_error_from_sockapi_error(err);
    }

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

    const SOCKET sockfd = clarinet_socket_handle(sp);

    struct sockaddr_storage ss = { 0 };
    int length = sizeof(ss);

    /* "The getsockname function does not always return information about the host address when the sp has been
     * bound to an unspecified address, unless the sp has been connected with connect or accept (for example, using
     * ADDR_ANY). A Windows Sockets application must not assume that the address will be specified unless the sp is
     * connected. The address that will be used for the sp is unknown unless the sp is connected when used in a
     * multihomed host. If the sp is using a connectionless protocol, the address may not be available until I/O
     * occurs on the sp." (https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-getsockname)
     *
     * HOWEVER, tests demonstrate this is not entirely true and in fact the only case when getsockname() returns
     * WSAEINVAL is when the sp is completely unbound, that is neither bind() nor connect() have been called prior
     * to the call to getsockname(). */
    if (getsockname(sockfd, (struct sockaddr*)&ss, &length) == SOCKET_ERROR)
        return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

    return clarinet_endpoint_from_sockaddr(local, &ss);
}

int
clarinet_socket_remote_endpoint(clarinet_socket* restrict sp,
                                clarinet_endpoint* restrict remote)
{
    if (!sp || sp->family == CLARINET_AF_UNSPEC || !remote)
        return CLARINET_EINVAL;

    if (!clarinet_socket_handle_is_valid(sp))
        return CLARINET_EINVAL;

    const SOCKET sockfd = clarinet_socket_handle(sp);

    struct sockaddr_storage ss = { 0 };
    int length = sizeof(ss);

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

    const SOCKET sockfd = clarinet_socket_handle(sp);

    if (send(sockfd, buf, (int)buflen, 0) == SOCKET_ERROR)
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

    const SOCKET sockfd = clarinet_socket_handle(sp);

    struct sockaddr_storage ss = { 0 };
    socklen_t sslen = 0;
    const int errcode = clarinet_endpoint_to_sockaddr(&ss, &sslen, remote);
    if (errcode != CLARINET_ENONE)
        return errcode;

    const int n = sendto(sockfd, buf, (int)buflen, 0, (struct sockaddr*)&ss, sslen);
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

    const SOCKET sockfd = clarinet_socket_handle(sp);

    const int n = recv(sockfd, buf, (int)buflen, 0);
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

    const SOCKET sockfd = clarinet_socket_handle(sp);

    struct sockaddr_storage ss;
    int sslen = sizeof(ss);
    const int n = recvfrom(sockfd, buf, (int)buflen, 0, (struct sockaddr*)&ss, &sslen);
    if (n < 0)
        return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

    /* Sanity: improbable but possible */
    if (sslen > sizeof(ss))
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

    const SOCKET sockfd = clarinet_socket_handle(sp);

    switch (optname)
    {
        case CLARINET_SO_NONBLOCK:
            if (optlen == sizeof(int32_t))
            {
                const DWORD val = *(const int32_t*)optval ? 1 : 0;
                if (setnonblock(sockfd, val) == SOCKET_ERROR)
                    return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                return CLARINET_ENONE;
            }
            break;
        case CLARINET_SO_REUSEADDR:
            if (optlen == sizeof(int32_t))
            {
                const DWORD val = (DWORD)((*(const int32_t*)optval) ? 1 : 0);
                const DWORD notval = (~val) & 1;
                /* The order we call setsockopt() here is important becase SO_EXCLUSIVEADDRUSE must be turned off before
                 * SO_REUSEADDR can be turned on SO_REUSEADDR. But SO_REUSEADDR must be turned off before
                 * SO_EXCLUSIVEADDRUSE can be turned on. If the right order is not respected we get WSAEINVAL. */
                if (val)
                {
                    if (setsockopt(sockfd, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (const char*)&notval, sizeof(notval)) == SOCKET_ERROR
                        || setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&val, sizeof(val)) == SOCKET_ERROR)
                    {
                        return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());
                    }
                }
                else
                {
                    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&val, sizeof(val)) == SOCKET_ERROR
                        || setsockopt(sockfd, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (const char*)&notval, sizeof(notval)) == SOCKET_ERROR)
                    {
                        return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());
                    }
                }

                return CLARINET_ENONE;
            }
            break;
        case CLARINET_SO_SNDBUF:
            if (optlen == sizeof(int32_t))
            {
                const DWORD val = (DWORD)clamp(*(const int32_t*)optval, 0, INT_MAX);
                if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (const char*)&val, sizeof(val)) == SOCKET_ERROR)
                    return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                return CLARINET_ENONE;
            }
            break;
        case CLARINET_SO_RCVBUF:
            if (optlen == sizeof(int32_t))
            {
                const DWORD val = (DWORD)clamp(*(const int32_t*)optval, 0, INT_MAX);
                if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (const char*)&val, sizeof(val)) == SOCKET_ERROR)
                    return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                return CLARINET_ENONE;
            }
            break;
        case CLARINET_SO_SNDTIMEO:
            if (optlen == sizeof(int32_t))
            {
                const DWORD val = (DWORD)clamp(*(const int32_t*)optval, 0, INT_MAX);
                if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&val, sizeof(val)) == SOCKET_ERROR)
                    return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                return CLARINET_ENONE;
            }
            break;
        case CLARINET_SO_RCVTIMEO:
            if (optlen == sizeof(int32_t))
            {
                const DWORD val = (DWORD)clamp(*(const int32_t*)optval, 0, INT_MAX);
                if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&val, sizeof(val)) == SOCKET_ERROR)
                    return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                return CLARINET_ENONE;
            }
            break;
        case CLARINET_SO_KEEPALIVE:
            if (optlen == sizeof(int32_t))
            {
                const DWORD val = *(const int32_t*)optval ? 1 : 0;
                if (setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (const char*)&val, sizeof(val)) == SOCKET_ERROR)
                    return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                return CLARINET_ENONE;
            }
            break;
        case CLARINET_SO_LINGER:
            if (optlen == sizeof(clarinet_linger))
            {
                const clarinet_linger* optret = (const clarinet_linger*)optval;
                struct linger linger;
                linger.l_onoff = optret->enabled;
                linger.l_linger = optret->seconds;
                if (setsockopt(sockfd, SOL_SOCKET, SO_LINGER, (const char*)&linger, sizeof(linger)) == SOCKET_ERROR)
                    return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                return CLARINET_ENONE;
            }
            break;
        case CLARINET_SO_DONTLINGER:
            if (optlen == sizeof(int32_t))
            {
                const DWORD val = *(const int32_t*)optval ? 1 : 0;
                if (setsockopt(sockfd, SOL_SOCKET, SO_DONTLINGER, (const char*)&val, sizeof(val)) == SOCKET_ERROR)
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
                    const DWORD val = *(const int32_t*)optval ? 1 : 0;
                    if (setsockopt(sockfd, IPPROTO_IPV6, IPV6_V6ONLY, (const char*)&val, sizeof(val)) == SOCKET_ERROR)
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
                    const DWORD val = *(const int32_t*)optval;
                    if (setsockopt(sockfd, IPPROTO_IP, IP_TTL, (const char*)&val, sizeof(val)) == SOCKET_ERROR)
                        return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                    return CLARINET_ENONE;
                }

                #if CLARINET_ENABLE_IPV6
                if (family == CLARINET_AF_INET6)
                {
                    const DWORD val = *(const int32_t*)optval;
                    if (setsockopt(sockfd, IPPROTO_IPV6, IPV6_UNICAST_HOPS, (const char*)&val, sizeof(val)) == SOCKET_ERROR) /* curiously IPV6_HOPLIMIT corresponds to IP_RECV_TTL not IP_TTL */
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
                    DWORD val;
                    switch (*(const int32_t*)optval)
                    {
                        case CLARINET_PMTUD_UNSPEC:
                            val = IP_PMTUDISC_NOT_SET;
                            break;
                        case CLARINET_PMTUD_ON:
                            val = IP_PMTUDISC_DO;
                            break;
                        case CLARINET_PMTUD_OFF:
                            val = IP_PMTUDISC_DONT;
                            break;
                        case CLARINET_PMTUD_PROBE:
                            val = IP_PMTUDISC_PROBE;
                            break;
                        default:
                            return CLARINET_EINVAL;
                    }

                    if (setsockopt(sockfd, IPPROTO_IP, IP_MTU_DISCOVER, (const char*)&val, sizeof(val)) == SOCKET_ERROR)
                        return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                    return CLARINET_ENONE;
                }

                #if CLARINET_ENABLE_IPV6
                if (family == CLARINET_AF_INET6)
                {
                    DWORD val;
                    switch (*(const int32_t*)optval)
                    {
                        case CLARINET_PMTUD_UNSPEC:
                            val = IP_PMTUDISC_NOT_SET;
                            break;
                        case CLARINET_PMTUD_ON:
                            val = IP_PMTUDISC_DO;
                            break;
                        case CLARINET_PMTUD_OFF:
                            val = IP_PMTUDISC_DONT;
                            break;
                        case CLARINET_PMTUD_PROBE:
                            val = IP_PMTUDISC_PROBE;
                            break;
                        default:
                            return CLARINET_EINVAL;
                    }

                    if (setsockopt(sockfd, IPPROTO_IPV6, IPV6_MTU_DISCOVER, (const char*)&val, sizeof(val)) == SOCKET_ERROR)
                        return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

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
clarinet_socket_getopt(clarinet_socket* restrict sp,
                       int optname,
                       void* restrict optval,
                       size_t* restrict optlen)
{
    /* Windows does not document the actual size of optlen returned for boolean sp options only the minimum that
     * must be provided so there are discrepancies. For example, SO_REUSEADDR is a boolean option that expects 4 bytes
     * and returns len = 4. On the other hand, SO_KEEPALIVE is documented as requiring 4 bytes and yet is happy enough
     * returning len = 1 on Windows 10. Since boolean options are false when 0 and true when non-zero the platform
     * endianess is irrelevant as long as the whole memory buffer provided has been set to all zeros before the call
     * to getsocktop() */

    if (!sp || sp->family == CLARINET_AF_UNSPEC || !optval || !optlen || *optlen > INT_MAX)
        return CLARINET_EINVAL;

    if (!clarinet_socket_handle_is_valid(sp))
        return CLARINET_EINVAL;

    const SOCKET sockfd = clarinet_socket_handle(sp);

    switch (optname)
    {
        case CLARINET_SO_REUSEADDR:
            if (*optlen >= sizeof(int32_t))
            {
                DWORD val = 0;
                int len = sizeof(val);
                if (getsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&val, &len) == SOCKET_ERROR)
                    return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                if (len < 0 || len > sizeof(val)) /* sanity check */
                    return CLARINET_ESYS;

                *(int32_t*)optval = (int32_t)val;
                *optlen = sizeof(int32_t);

                return CLARINET_ENONE;
            }
            break;
        case CLARINET_SO_SNDBUF:
            if (*optlen >= sizeof(int32_t))
            {
                DWORD val = 0;
                int len = sizeof(val);
                if (getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (char*)&val, &len) == SOCKET_ERROR)
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
                DWORD val = 0;
                int len = sizeof(val);
                if (getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (char*)&val, &len) == SOCKET_ERROR)
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
                DWORD val = 0;
                int len = sizeof(val);
                if (getsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char*)&val, &len) == SOCKET_ERROR)
                    return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                if (len != sizeof(val)) /* sanity check */
                    return CLARINET_ESYS;

                *(int32_t*)optval = (int32_t)val;
                *optlen = sizeof(int32_t);

                return CLARINET_ENONE;
            }
            break;
        case CLARINET_SO_RCVTIMEO:
            if (*optlen >= sizeof(int32_t))
            {
                DWORD val = 0;
                int len = sizeof(val);
                if (getsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&val, &len) == SOCKET_ERROR)
                    return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                if (len != sizeof(val)) /* sanity check */
                    return CLARINET_ESYS;

                *(int32_t*)optval = (int32_t)val;
                *optlen = sizeof(int32_t);

                return CLARINET_ENONE;
            }
            break;
        case CLARINET_SO_KEEPALIVE:
            if (*optlen >= sizeof(int32_t))
            {
                DWORD val = 0;
                int len = sizeof(val);
                if (getsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (char*)&val, &len) == SOCKET_ERROR)
                    return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                if (len < 0 || len > sizeof(val)) /* sanity check */
                    return CLARINET_ESYS;

                *(int32_t*)optval = (int32_t)val;
                *optlen = sizeof(int32_t);

                return CLARINET_ENONE;
            }
            break;
        case CLARINET_SO_LINGER:
            if (*optlen >= sizeof(clarinet_linger))
            {
                struct linger linger = {0};
                int len = sizeof(linger);

                if (getsockopt(sockfd, SOL_SOCKET, SO_LINGER, (char*)&linger, &len) == SOCKET_ERROR)
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
                DWORD val = 0;
                int len = sizeof(val);
                if (getsockopt(sockfd, SOL_SOCKET, SO_DONTLINGER, (char*)&val, &len) == SOCKET_ERROR)
                    return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                if (len < 0 || len > sizeof(val)) /* sanity check */
                    return CLARINET_ESYS;

                *(int32_t*)optval = (int32_t)val;
                *optlen = sizeof(int32_t);

                return CLARINET_ENONE;
            }
            break;
        case CLARINET_SO_ERROR:
            if (*optlen >= sizeof(int32_t))
            {
                DWORD val = 0;
                int len = sizeof(val);
                if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (char*)&val, &len) == SOCKET_ERROR)
                    return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                if (len != sizeof(val)) /* sanity check */
                    return CLARINET_ESYS;

                *(int32_t*)optval = (int32_t)clarinet_error_from_sockapi_error((int)val);
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
                    DWORD val = 0;
                    int len = sizeof(val);
                    if (getsockopt(sockfd, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&val, &len) == SOCKET_ERROR)
                        return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                    if (len < 0 || len > sizeof(val)) /* sanity check */
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
                const int family = sp->family;
                if (family == CLARINET_AF_INET)
                {
                    int val = 0;
                    int len = sizeof(val);
                    if (getsockopt(sockfd, IPPROTO_IP, IP_TTL, (char*)&val, &len) == SOCKET_ERROR)
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
                    DWORD val = 0;
                    int len = sizeof(val);
                    if (getsockopt(sockfd, IPPROTO_IPV6, IPV6_UNICAST_HOPS, (char*)&val, &len) == SOCKET_ERROR) /* curiously IPV6_HOPLIMIT corresponds to IP_RECV_TTL not IP_TTL */
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
                const int family = sp->family;
                if (family == CLARINET_AF_INET)
                {
                    int val;
                    int len = sizeof(val);
                    if (getsockopt(sockfd, IPPROTO_IP, IP_MTU, (char*)&val, &len) == SOCKET_ERROR)
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
                    DWORD val = 0;
                    int len = sizeof(val);
                    if (getsockopt(sockfd, IPPROTO_IPV6, IPV6_MTU, (char*)&val, &len) == SOCKET_ERROR)
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
                DWORD val = 0;
                int len = sizeof(val);

                const int family = sp->family;
                if (family == CLARINET_AF_INET)
                {
                    if (getsockopt(sockfd, IPPROTO_IP, IP_MTU_DISCOVER, (char*)&val, &len) == SOCKET_ERROR)
                        return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                    if (len != sizeof(val)) /* sanity check */
                        return CLARINET_ESYS;

                    switch (val)
                    {
                        case IP_PMTUDISC_NOT_SET:
                            *optret = CLARINET_PMTUD_UNSPEC;
                            break;
                        case IP_PMTUDISC_DO:
                            *optret = CLARINET_PMTUD_ON;
                            break;
                        case IP_PMTUDISC_DONT:
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
                    if (getsockopt(sockfd, IPPROTO_IPV6, IPV6_MTU_DISCOVER, (char*)&val, &len) == SOCKET_ERROR)
                        return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

                    if (len != sizeof(val)) /* sanity check */
                        return CLARINET_ESYS;

                    switch (val)
                    {
                        case IP_PMTUDISC_NOT_SET:
                            *optret = CLARINET_PMTUD_UNSPEC;
                            break;
                        case IP_PMTUDISC_DO:
                            *optret = CLARINET_PMTUD_ON;
                            break;
                        case IP_PMTUDISC_DONT:
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
                #endif /* CLARINET_ENABLE_IPV6 */
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

    const SOCKET sockfd = clarinet_socket_handle(sp);

    struct sockaddr_storage ss = { 0 };
    socklen_t sslen = 0;
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

    const SOCKET sockfd = clarinet_socket_handle(sp);

    /* Winsock defines the constat SOMAXCONN which can be explicitly used to allow the system to set the backlog to a
     * maximum reasonable value. Winsock 1.x defines SOMAXCONN = 5. Winsock 2.x defines SOMAXCONN = 0x7fffffff
     * (2147483647) and this limits the listen backlog to a large value, typically several hundred or more but less than
     * 1000. Alternatively, passing a negative value -N will set the backlog to N, adjusted to be within the range
     * (200, 65535). The macro SOMAXCONN_HINT(N) is just syntax sugar to negate N.
     */

    if (listen(sockfd, SOMAXCONN_HINT(backlog)) == SOCKET_ERROR)
    {
        const int err = clarinet_get_sockapi_error();
        /* If the sp type/proto is not compatible the system returns EOPNOTSUPP but since we automatically derive
         * type from proto (STREAM/TCP, STREAM/SCTP, DGRAM/UDP) we want to return CLARINET_EPROTONOSUPPORT instead
         * of CLARINET_ENOTSUP here. */
        return (err == WSAEOPNOTSUPP) ? CLARINET_EPROTONOSUPPORT : clarinet_error_from_sockapi_error(err);
    }

    return CLARINET_ENONE;
}

int
clarinet_socket_accept(clarinet_socket* restrict ssp,
                       clarinet_socket* restrict client,
                       clarinet_endpoint* restrict remote)
{
    if (!ssp || ssp->family == CLARINET_AF_UNSPEC || !client || client->family != CLARINET_AF_UNSPEC || !remote)
        return CLARINET_EINVAL;

    if (!clarinet_socket_handle_is_valid(ssp))
        return CLARINET_EINVAL;

    const SOCKET serverfd = clarinet_socket_handle(ssp);

    struct sockaddr_storage ss;
    int sslen = sizeof(ss);

    SOCKET clientfd = accept(serverfd, (struct sockaddr*)&ss, &sslen);
    if (clientfd == INVALID_SOCKET)
    {
        const int err = clarinet_get_sockapi_error();
        /* If the socket type/proto is not compatible the system returns EOPNOTSUPP but since we automatically derive
         * type from proto (STREAM/TCP, STREAM/SCTP, DGRAM/UDP) we want to return CLARINET_EPROTONOSUPPORT instead
         * of CLARINET_ENOTSUP here. */
        return (err == WSAEOPNOTSUPP) ? CLARINET_EPROTONOSUPPORT : clarinet_error_from_sockapi_error(err);
    }

    client->family = ssp->family;
    client->handle = (void*)clientfd;

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

    const SOCKET sockfd = clarinet_socket_handle(sp);

    int sflags = 0;
    if (flags & CLARINET_SHUTDOWN_RECV)
    {
        flags &= ~CLARINET_SHUTDOWN_RECV;
        sflags |= SD_RECEIVE;
    }

    if (flags & CLARINET_SHUTDOWN_SEND)
    {
        flags &= ~CLARINET_SHUTDOWN_SEND;
        sflags |= SD_SEND;
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
clarinet_socket_poll_context_getstatus(const void* context,
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

    if (WSAPoll((struct pollfd*)context, (ULONG)count, timeout) == SOCKET_ERROR)
        return clarinet_error_from_sockapi_error(clarinet_get_sockapi_error());

    return CLARINET_ENONE;
}



/* endregion */
