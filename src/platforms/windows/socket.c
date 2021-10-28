#include "platforms/windows/sys.h"
#include "compat/addr.h"
#include "compat/error.h"

#include <synchapi.h>
#include <assert.h>

#if !defined(SIO_UDP_CONNRESET)
    #define SIO_UDP_CONNRESET (IOC_IN | IOC_VENDOR | 12)
#endif

// COMMENT for clarinet_socket_listen
/* Winsock defines the constat SOMAXCONN which can be explicitly used to allow the system to set the backlog to a
 * maximum reasonable value. Winsock 1.x defines SOMAXCONN = 5. Winsock 2.x defines SOMAXCONN = 0x7fffffff
 * (2147483647) and this limits the listen backlog to a large value, typically several hundred or more but less than
 * 1000. Alternatively, passing a negative value -N will set the backlog to N, adjusted to be within the range
 * (200, 65535). The macro SOMAXCONN_HINT(N) is just syntax sugar to negate N.
 */

/* region Initialization */

/**
 * Lockless mutual exclusion between cl_initialize() and cl_finalize() is acomplished with a mechanism
 * similar to a spin lock but using 3 states instead of 2 so the function can break early if another thread managed
 * to execute first.
 *
 * States are: FINALIZED(0), BUSY(1) and INITIALIZED(2)
 *
 * A call to cl_initialize() transitions from FINALIZED to INITIALIZED passing through BUSY while a call to
 * cl_finalize() transitions from INITIALIZED to FINALIZED also passing through BUSY. Either call will only
 * perform any work when actually triggering the transition to BUSY, otherwise it will just spin and wait for the state
 * owner to complete the current transition or break early.
 */
static LONG cl_static_init = 0;

#define CL_SPIN_LIMIT 4096

int
cl_initialize(void)
{
    int cycles = 0;

    repeat:
    int prev = InterlockedCompareExchange(&cl_static_init, 1, 0);
    if (prev == 0)
    {
        /* The simplest approach is to call WSAStartup every time.
         * According to https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-wsastartup all calls to
         * WSAStartup() are ref counted and the library will only be unloaded after the same number of WSACleanup calls are
         * performed. */
        /* Version 2.2. is the one for Windows Vista and later */
        const WORD wsa_version = MAKEWORD(2, 2);
        WSADATA wsa_data;
        /* WSAStartup returns an error directly, cl_get_sockapi_error() should not be used in this case. */
        int err = WSAStartup(wsa_version, &wsa_data);
        if (err != 0)
        {
            InterlockedDecrement(&cl_static_init);
            return cl_error_from_sockapi_error(err);
        }

        InterlockedIncrement(&cl_static_init);
    }
    else if (prev == 1)
    {
        cycles++;
        if (cycles == CL_SPIN_LIMIT)
        {
            cycles = 0;
            Sleep(0); /* yield */
        }

        goto repeat;
    }

    return CL_ENONE;
}

int
cl_finalize(void)
{
    int cycles = 0;

    repeat:
    int prev = InterlockedCompareExchange(&cl_static_init, 1, 2);
    if (prev == 2)
    {
        int err = WSACleanup();
        if (err != 0 && err != WSANOTINITIALISED)
        {
            InterlockedIncrement(&cl_static_init);
            return cl_error_from_sockapi_error(err);
        }

        InterlockedDecrement(&cl_static_init);
    }
    else if (prev == 1)
    {
        cycles++;
        if (cycles == CL_SPIN_LIMIT)
        {
            cycles = 0;
            Sleep(0); /* yield */
        }

        goto repeat;
    }

    return CL_ENONE;


}

/* endregion */

/* region Socket */

CL_ATTR_INLINE
void
cl_socket_free(cl_socket** spp,
               void(* destructor)(cl_socket*))
{
    if (destructor)
    {
        /* defensive: ensure the socket is not valid anymore to avoid mistakes in the destructor */
        (*spp)->handle = INVALID_SOCKET;
        destructor(*spp);
    }
    free(*spp);
    *spp = NULL;
}

int
cl_socket_close(cl_socket** spp,
                void(* destructor)(cl_socket*))
{
    if (!spp || !*spp)
        return CL_EINVAL;

    const SOCKET sockfd = (*spp)->handle;
    if (closesocket(sockfd) == SOCKET_ERROR)
    {
        int err = cl_get_sockapi_error();
        if (err == WSAEWOULDBLOCK)
        {
            /* closesocket(1) MAY fail with WSAEWOULDBLOCK when the socket is non-blocking, there is data to flush and
             * a non-zero linger timeout is active, so try to set the socket to blocking and close again. Do not force
             * SO_LINGER(0) here because it is not clear what the user intentions are. There is no point in having a
             * boolean argument for that either (e.g.: force) because the user can always adjust SO_LINGER or use the
             * shorcut option SO_DONTLINGER at any time before closing the socket. */
            if (setnonblock(sockfd) == SOCKET_ERROR)
            {
                /* setsocknonblock() may fail if there's a pending WSAEventSelect so try canceling it first.
                 * Issuing a WSAEventSelect for a socket cancels any previous WSAAsyncSelect or WSAEventSelect for the
                 * same socket and clears the internal network event record. */
                err = cl_get_sockapi_error();
                if (err == WSAEFAULT)
                {
                    /* At this point, the error from WSAEventSelect or setsocknonblock is irrelevant. Just try to
                     * release any pending event select, set the socket to blocking and move on to repeatedly try to
                     * close until the socket stops asking for a retry. */
                    WSAEventSelect(sockfd, NULL, 0);
                    setnonblock(sockfd);
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
                    err = cl_get_sockapi_error();
                    if (err != WSAEWOULDBLOCK)
                        break;

                    Sleep(1000);
                }
                else
                {
                    cl_socket_free(spp, destructor);
                    return CL_ENONE;
                }
            }
        }

        assert(err != 0);
        assert(err != WSAEWOULDBLOCK);
        switch (err)
        {
            /* It's not clear in the documentation when/why/how closesocket() would return WSAENETDOWN and what to do in
             * that case. Could the network recover and the socket still be valid? Would resources still be
             * acquired? For now assuming the socket is in an unrecoverable state. */
            case WSAENETDOWN:
                /* WSANOTINITIALISED and WSAENOTSOCK means the socket is in an unrecoverable state so the only thing to do
                 * is free the allocated memory */
            case WSANOTINITIALISED:
            case WSAENOTSOCK:
                cl_socket_free(spp, destructor);
                return CL_ENONE;
                /* Anything else should include only WSAEINPROGRESS and WSAEINTR which are not expected because the
                 * pseudo-blocking facilities of WinSock 1 are not used and the socket can only be in non-blocking mode
                 * but if anything unexpected is returned the user can at least be made aware and try to close again */
            default:
                return cl_error_from_sockapi_error(err);
        }
    }

    cl_socket_free(spp, destructor);
    return CL_ENONE;
}

int
cl_socket_send(cl_socket* restrict sp,
               const void* restrict buf,
               size_t buflen,
               const cl_endpoint* restrict dst)
{
    if (!sp || (!buf && buflen > 0) || !dst || buflen > INT_MAX)
        return CL_EINVAL;

    struct sockaddr_storage ss;
    socklen_t sslen;
    const int errcode = cl_endpoint_to_sockaddr(&ss, &sslen, dst);
    if (errcode != CL_ENONE)
        return errcode;

    const int n = sendto(sp->handle, buf, (int)buflen, 0, (struct sockaddr*)&ss, sslen);
    if (n < 0)
        return cl_error_from_sockapi_error(cl_get_sockapi_error());

    return (int)n;
}

int
cl_socket_recv(cl_socket* restrict sp,
               void* restrict buf,
               size_t buflen,
               cl_endpoint* restrict src)
{
    if (!sp || !buf || buflen == 0 || buflen > INT_MAX || !src)
        return CL_EINVAL;

    struct sockaddr_storage ss;
    int slen = sizeof(ss);
    const int n = recvfrom(sp->handle, buf, (int)buflen, 0, (struct sockaddr*)&ss, &slen);
    if (n < 0)
        return cl_error_from_sockapi_error(cl_get_sockapi_error());

    /* Sanity: improbable but possible */
    if (slen > sizeof(ss))
        return CL_EADDRNOTAVAIL;

    const int errcode = cl_endpoint_from_sockaddr(src, &ss);
    if (errcode != CL_ENONE)
        return CL_EADDRNOTAVAIL;

    return (int)n;
}

int
cl_socket_get_endpoint(cl_socket* restrict sp,
                       cl_endpoint* restrict endpoint)
{
    if (!sp | !endpoint)
        return CL_EINVAL;

    struct sockaddr_storage ss = { 0 };
    int length = sizeof(ss);

    if (getsockname(sp->handle, (struct sockaddr*)&ss, &length) == SOCKET_ERROR)
        return cl_error_from_sockapi_error(cl_get_sockapi_error());

    return cl_endpoint_from_sockaddr(endpoint, &ss);
}

/* endregion */

/* region Helpers */

int
setnonblock(SOCKET sock)
{
    ULONG mode = 1;
    return ioctlsocket(sock, FIONBIO, &mode);
}

int
setnonreset(SOCKET sock)
{
    ULONG value = 0;
    return ioctlsocket(sock, SIO_UDP_CONNRESET, &value);
}

int
initsockopt(SOCKET sock,
            int level,
            int optname,
            const char* optval,
            int optlen)
{
    if (setsockopt(sock, level, optname, optval, optlen) == SOCKET_ERROR)
    {
        if (cl_get_sockapi_error() == WSAEINVAL)
            cl_set_sockapi_error(WSAEOPNOTSUPP);

        return SOCKET_ERROR;
    }

    return 0;
}

int
trysockopt(SOCKET sock,
           int level,
           int optname,
           const char* optval,
           int optlen)
{
    if (setsockopt(sock, level, optname, optval, optlen) == SOCKET_ERROR)
    {
        const int err = cl_get_sockapi_error();
        return (err == WSAENOPROTOOPT || err == WSAEINVAL) ? 0 : SOCKET_ERROR;
    }

    return 0;
}

/* endregion */
