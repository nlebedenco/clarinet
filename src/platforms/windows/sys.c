#include "platforms/windows/sys.h"
#include "portable/addr.h"

#include <synchapi.h>
#include <assert.h>

/* NOTES:
 *   - All calls to WSAGetLastError are thread-safe. Winsock keeps separate error codes pre-thread.
 *   - Ideally we would like to handle our socket identifiers without the need for some complex mapping to the 
 *     underlying socket descriptor used by the platform. Under most Unix systems, sockets are part of the standard C 
 *     library, no additional dependencies are required and file descriptors (integers) are used as identifiers. Under 
 *     Windows we have to use WinSock to which socket descriptors are an opaque type (usually normal NT kernel object 
 *     handles). This means that instead of type int, sockets need to be of type SOCKET and the classic comparison of 
 *     < 0 to detect error conditions or a bad descriptor is no longer valid - one has to use the INVALID_SOCKET
 *     constant instead. We worked around all this by using an opaque pointer to represent the socket which also serves
 *     to keep a to internal state. It's worth noting though that while winsock sockets are supposed to be 
 *     interchangeable with instances of HANDLE in many windows APIs, Microsoft chose to use a different representation 
 *     so casting is necessary when sockets are used with normal Win32  function calls. Unfortunately, this dependency 
 *     on SOCKET translates to a dependency on <winsock2.h> which everyone actively tries to avoid. According to the 
 *     documentation at https://docs.microsoft.com/en-us/windows/win32/winsock/socket-data-type-2 no assumption can be 
 *     made about the range of SOCKET other than that it is a value in the rance [0, INVALID_SOCKET-1]. On the other 
 *     hand, Microsoft guarantees a stable ABI so one could take advantage of that and rely on the underlying type used 
 *     for handles (UINT_PTR) which matches the more standard uintptr_t from <stdint.h>. Furthermore according to this 
 *     article https://docs.microsoft.com/en-us/windows/win32/sysinfo/kernel-objects windows has a per-process limit on 
 *     kernel handles of 2^24 which makes it relatively safe to cast to intptr_t or even int under the normal assumption 
 *     that sizeof(int) >= sizeof(intptr_t). The fact that INVALID_SOCKET has always been defined as (~0) seems to 
 *     support the notion that casting from SOCKET to int and back should not cause loss of data. All this will make 
 *     purists whimper but looking at the big picture Microsoft could have simply used int from the beginning.
 */

int 
clarinet_error_from_wsaerr(const int wsaerr)
{
    /* WSAEOPNOTSUPP (10045) "Operation not supported" should never be relayed to the end-user. It can only happen due 
     * to socket mishandling. For example by calling accept() on a udp socket or using incompatible send/recv flags. */
    assert(wsaerr != WSAEOPNOTSUPP);

    /* WSAEINPROGRESS (10036) "Operation now in progress" is not equivalent to EINPROGRESS. 
     *
     * TL;DR: On Unix EINPROGRESS is only indicated by connect(2) and is semantically equivalent to EWOULDBLOCK (and by 
     * extension to WSAEWOULDBLOCK). On Windows (winsock 2.x) WSAEINPROGRESS should only (if ever) be indicated by 
     * connect() and is semantically equivalent to WSAEALREADY (and by extension to EALREADY).
     *
     *
     * The long story:
     * On BSD sockets EINPROGRESS is indicated when an operation that takes a long time to complete (such as a 
     * connect(2)) was attempted on a non-blocking socket (see ioctl(2)). On windows this is indicated by 
     * WSAEWOULDBLOCK because WSAEINPROGRESS only applies to blocking calls!. When winsock was introduced (v1.x), 
     * Microsoft operating systems (MS-DOS/Win3.x/Win95) did not support preemptive scheduling so blocking operations 
     * that could not complete immediately had to be implemented using a pseudo-blocking mechanism that had to blend 
     * in the application main loop whlie handling windows messages so the UI would not hang. This is more or less 
     * described in this MSN article: 
     * https://docs.microsoft.com/en-us/windows/win32/winsock/windows-sockets-1-1-blocking-routines-and-einprogress-2.
     * In this context winsock only allowed a single blocking operation to be outstanding per task (or thread), and if 
     * the user made any other blocking call (whether or not it referenced the same socket) the function would fail with 
     * WSAEINPROGRESS. Since WinSock 2.x, all Windows versions featured a preemptive 32-bit kernel so this error should
     * not be observable anymore. Nevertheless, anecdotal evidence suggests winsock might still return it after an 
     * application calls connect() a second time on a non-blocking socket while the first connection attempt is pending 
     * (i.e. after the first returned with WSAEWOULDBLOCK) although the documentation states WSAEALREADY should be 
     * expected instead.
     *
     * In any case, since our socket API is exclusively non-blocking, EINPROGRESS and EWOULDBLOCK are redundant and 
     * should never reach the user.
     */
     
    /* WSAENETRESET (10052) "Network dropped connection on reset" has a special meaning for TCP and UDP sockets.
     * For a connection-oriented socket, this error indicates that the connection has been broken due to keep-alive 
     * activity that detected a failure while the operation was in progress. This is semantically equivalent to 
     * ENETRESET. For a datagram socket, this error indicates that the time to live has expired which is detectable
     * because routers can send back an ICMP packet indicating that a packet was dropped because TTL reached zero. Note
     * that ICMP packets may be dropped by other routers or blocked by firewalls though. For now this error should
     * be ignored for udp sockets on both setsockopt(), send() and recv(). 
     */
   
    /* WSAEWOULDBLOCK (10035) "Resource temporarily unavailable" may be relayed to the end-user from send() or recv() */
        
    switch(wsaerr)
    {
        case 0: 
            return CLARINET_ENONE;
        case WSA_INVALID_HANDLE:            /* An application attempted to use an event object, but the specified handle is not valid. */
        case WSAEBADF:                      /* The file handle supplied is not valid */            
        case WSAEFAULT:                     /* The system detected an invalid pointer address in attempting to use a pointer argument of a call.  */                    
        case WSAEINVAL:                     /* Invalid argument */            
        case WSAEAFNOSUPPORT:               /* Address/Protocol family not supported. */
        case WSAEPFNOSUPPORT:               /* Equivalent to WSAEAFNOSUPPORT */
            return CLARINET_EINVAL; 
        case WSA_NOT_ENOUGH_MEMORY:         /* Not enough memory. Not indicated by any specific function supposedly could be reported by any one. */ 
            return CLARINET_ENOMEM;           
        case WSAEINTR:                      /* A blocking operation was interrupted. This error should not normally be relayed to the end-user. */        
            return CLARINET_EINTR;                   
        case WSAESOCKTNOSUPPORT:            /* Socket type not suported. This error should not normally be relayed to the end-user. */        
        case WSAEPROTOTYPE:                 /* The specified protocol is the wrong type for this socket. This error should not normally be relayed to the end-user. */
        case WSAEPROTONOSUPPORT:            /* Protocol not supported. This error should not normally be relayed to the end-user. */
            return CLARINET_ENOTSUPP;               
        case WSAEACCES:                     /* Permission denied */
            return CLARINET_EACCES;                        
        case WSAEMFILE:                     /* Too many open sockets */
            return CLARINET_EMFILE;         
        case WSAEWOULDBLOCK:                /* Operations on a nonblocking socket could not be completed immediately */
            return CLARINET_EWOULDBLOCK;
        case WSAEINPROGRESS:                /* A blocking operation is currently executing. Winsock only allows a single blocking operation—per-task/thread */
        case WSAEALREADY:                   /* In other cases when an operation is already in progress a function may fail with WSAEALREADY. */        
            return CLARINET_EALREADY;       
        case WSAENOTSOCK:                   /* An operation was attempted on something that is not a socket. */  
            return CLARINET_ENOTSOCK;        
        case WSAEMSGSIZE:                   /* Datagram is larger than the internal message buffer */
            return CLARINET_EMSGSIZE;        
        case WSAENOPROTOOPT:                /* Bad protocol option. */    
            return CLARINET_ENOPROTOOPT;        
        case WSAEADDRINUSE:                 /* Address already in use. */
            return CLARINET_EADDRINUSE;        
        case WSAEADDRNOTAVAIL:              /* This normally results from an attempt to bind to an address that is not valid for the local computer. */
            return CLARINET_EADDRNOTAVAIL;        
        case WSAENETDOWN:                   /* Network is down */
            return CLARINET_ENETDOWN;        
        case WSAENETUNREACH:                /* Network is unreachable */
             return CLARINET_ENETUNREACH;        
        case WSAENETRESET:                  /* The connection has been broken due to keep-alive activity detecting a failure while the operation was in progress. */
            return CLARINET_ENETRESET;
        case WSAECONNABORTED:               /* An established connection was aborted by the software in your host computer (local)*/
            return CLARINET_ECONNABORTED;        
        case WSAECONNRESET:                 /* Connection reset by peer. An existing connection was forcibly closed by the remote host. */
            return CLARINET_ECONNRESET;        
        case WSAENOBUFS:                    /* No buffer space available or queue is full */
            return CLARINET_ENOBUFS;        
        case WSAEISCONN:                    /* Socket is already connected */
            return CLARINET_EISCONN;        
        case WSAENOTCONN:                   /* Socket is not connected */
            return CLARINET_ENOTCONN;        
        case WSAESHUTDOWN:                  /* Cannot send after socket is shutdown */               
            return CLARINET_ECONNSHUTDOWN;        
        case WSAETIMEDOUT:                  /* Connection timed out */
            return CLARINET_ECONNTIMEOUT;        
        case WSAECONNREFUSED:               /* Connection refused */
            return CLARINET_ECONNREFUSED;            
        case WSAEHOSTDOWN:                  /* A socket operation failed because the destination host is down. Currently no documented winsock function returns this error. */
            return CLARINET_EHOSTDOWN;        
        case WSAEHOSTUNREACH:               /* No route to host. */
            return CLARINET_EHOSTUNREACH;        
        case WSAEPROCLIM:                   /* A limit on the number of tasks supported by the Windows Sockets implementation has been reached. */            
            return CLARINET_EPROCLIM;                   
        case WSASYSNOTREADY:                /* The underlying network subsystem is not ready for network communication. */ 
            return CLARINET_ENOTREADY;        
        case WSAVERNOTSUPPORTED:            /* The version of Windows Sockets support requested is not provided by this particular Windows Sockets implementation. */                 
        case WSANOTINITIALISED:             /* Successful WSAStartup not yet performed. This error should not normally be relayed to the end-user. */
            return CLARINET_ELIBACC;        
        case WSAEINVALIDPROVIDER:           /* The service provider returned a version other than 2.2. */
        case WSAEINVALIDPROCTABLE:          /* The service provider returned an invalid or incomplete procedure table to the WSPStartup. */
        case WSAEPROVIDERFAILEDINIT:        /* The service provider failed to initialize. */        
            return CLARINET_ELIBBAD;            
        default:
            return CLARINET_EDEFAULT;
    }
}

int 
clarinet_socket_option_to_sockopt(int* restrict optlevel,
                                  int* restrict optname,
                                  const int proto, 
                                  const int option)
{
    CLARINET_IGNORE(optlevel);
    CLARINET_IGNORE(optname);
    CLARINET_IGNORE(proto);
    CLARINET_IGNORE(option);
    
    return CLARINET_ENOSYS;
}

CLARINET_INLINE
void
clarinet_socket_free(clarinet_socket** spp,
                     void(*destructor)(clarinet_socket* sp))
{
    if (destructor)
    {
        /* defensive:  ensure the socket is not valid anymore to avoid mistakes in the destructor */
        (*spp)->handle = INVALID_SOCKET;
        destructor(*spp);
    }
    clarinet_free(*spp);
    *spp = NULL;
}


CLARINET_INLINE
void
clarinet_socket_free_and_cleanup(clarinet_socket** spp,
                                 void(*destructor)(clarinet_socket* sp))
{
    if (destructor)
    {
        /* defensive: ensure the socket is not valid anymore to avoid mistakes in the destructor */
        (*spp)->handle = INVALID_SOCKET;
        destructor(*spp);
    }
    clarinet_free(*spp);
    *spp = NULL;
    /* At this point any error produced by WSACleanup is irrelevant because there is nothing the end user can do */
    WSACleanup();
}

int
clarinet_socket_close(clarinet_socket** spp,
                      void(*destructor)(clarinet_socket* sp))
{
    if (!spp || !*spp)
        return CLARINET_EINVAL;

    const SOCKET sockfd = (*spp)->handle;
    if (closesocket(sockfd) == SOCKET_ERROR)
    {
        int wsaerr = WSAGetLastError();
        if (wsaerr == WSAEWOULDBLOCK)
        {
            /* closesocket(1) MAY fail with WSAEWOULDBLOCK when the socket is non-blocking, there is data to flush and
             * a non-zero linger timeout is active, so try to set the socket to blocking and close again. Do not force
             * SO_LINGER(0) here because it is not clear what the user intentions are. There is no point in having a
             * boolean argument for that either (e.g.: force) because the user can always adjust SO_LINGER or use the
             * shorcut option SO_DONTLINGER at any time before closing the socket. */
            if (setsocknonblock(sockfd) == SOCKET_ERROR)
            {
                /* setsocknonblock() may fail if there's a pending WSAEventSelect so try canceling it first.
                 * Issuing a WSAEventSelect for a socket cancels any previous WSAAsyncSelect or WSAEventSelect for the
                 * same socket and clears the internal network event record. */
                wsaerr = WSAGetLastError();
                if (wsaerr == WSAEFAULT)
                {
                    /* At this point the wsaerr from WSAEventSelect or setsocknonblock is irrelevant. Just try to release
                     * any pending event select, set the socket to blocking and move on to repeatedly try to close until the
                     * socket stops asking for a retry. */
                    WSAEventSelect(sockfd, NULL, 0);
                    setsocknonblock(sockfd);
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
                    wsaerr = WSAGetLastError();
                    if (wsaerr != WSAEWOULDBLOCK)
                        break;

                    Sleep(1000);
                }
                else
                {
                    clarinet_socket_free_and_cleanup(spp, destructor);
                    return CLARINET_ENONE;
                }
            }
        }

        assert(wsaerr != 0);
        assert(wsaerr != WSAEWOULDBLOCK);
        switch (wsaerr)
        {
            /* It's not clear in the documentation when/why/how closesocket() would return WSAENETDOWN and what to do in
             * that case. Could the network recover and the socket still be valid? Would resources still be
             * acquired? For now assuming the socket is in an unrecoverable state. */
            case WSAENETDOWN:
            /* WSANOTINITIALISED and WSAENOTSOCK means the socket is in an unrecoverable state so the only thing to do 
             * is free the allocated memory */
            case WSANOTINITIALISED:
            case WSAENOTSOCK:
                clarinet_socket_free(spp, destructor);
                return CLARINET_ENONE;
            /* Anything else should include only WSAEINPROGRESS and WSAEINTR which are not expected because the
             * pseudo-blocking facilities of WinSock 1 are not used and the socket can only be in non-blocking mode
             * but if anything unexpected is returned the user can at least be made aware and try to close again */
            default:
                return clarinet_error_from_wsaerr(wsaerr);
        }
    }

    clarinet_socket_free_and_cleanup(spp, destructor);
    return CLARINET_ENONE;
}

int
clarinet_socket_get_endpoint(clarinet_socket* restrict sp,
                             clarinet_endpoint* restrict endpoint)
{
    if (!sp | !endpoint)
        return CLARINET_EINVAL;

    struct sockaddr_storage ss = {0};
    int length = sizeof(ss);

    if (getsockname(sp->handle, (struct sockaddr*)&ss, &length) == SOCKET_ERROR)
        return clarinet_error_from_wsaerr(WSAGetLastError());

    return clarinet_endpoint_from_sockaddr(endpoint, &ss);
}

int
clarinet_socket_send(clarinet_socket* restrict sp,
                     const void* restrict buf,
                     size_t len,
                     const clarinet_endpoint* restrict dst)
{
    if (!sp || !buf || !dst || len > INT_MAX)
        return CLARINET_EINVAL;
    
    if (len == 0)
        return CLARINET_ENONE;
       
    struct sockaddr_storage ss;
    const int errcode = clarinet_endpoint_to_sockaddr(&ss, dst);
    if (errcode != CLARINET_ENONE)
        return errcode;
    
    const int n = sendto(sp->handle, buf, (int)len, 0, (struct sockaddr*)&ss, sizeof(ss));
    if (n < 0)
        return clarinet_error_from_wsaerr(WSAGetLastError());
    
    return (int)n;
}

int
clarinet_socket_recv(clarinet_socket* restrict sp,
                     void* restrict buf,
                     size_t len,
                     clarinet_endpoint* restrict src)
{
     if (!sp || !buf || len == 0 || len > INT_MAX || !src)
        return CLARINET_EINVAL;
          
    struct sockaddr_storage ss;   
    int slen = sizeof(ss);
    const int n = recvfrom(sp->handle, buf, (int)len, 0, (struct sockaddr*)&ss, &slen);
    if (n < 0)
        return clarinet_error_from_wsaerr(WSAGetLastError());
    
    /* Sanity: improbable but possible */
    if (slen > sizeof(ss))
        return CLARINET_EADDRNOTAVAIL;
    
    const int errcode = clarinet_endpoint_from_sockaddr(src, &ss);
    if (errcode != CLARINET_ENONE)
        return CLARINET_EADDRNOTAVAIL;

    return (int)n;
}
