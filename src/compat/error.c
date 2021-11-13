#include "compat/error.h"

#if defined(_WIN32)

/* All calls to WSAGetLastError are thread-safe. Winsock keeps separate error codes pre-thread.
 * Error codes returned by getaddrinfo() have a specific mapping. Although they are known to have the same values
 * as WSA errors according to the documentation the interpretation to the end user is different in some cases.
 */

int
clarinet_get_sockapi_error()
{
    return WSAGetLastError();
}

void
clarinet_set_sockapi_error(int err)
{
    WSASetLastError(err);
}


int
clarinet_error_from_sockapi_error(int err)
{
    /* WSAEINPROGRESS (10036) "Operation now in progress" is not equivalent to EINPROGRESS.
     *
     * TL;DR: On Unix EINPROGRESS is only indicated by connect(2) and is semantically equivalent to EWOULDBLOCK (and by
     * extension to WSAEWOULDBLOCK). On Windows (winsock 2.x) WSAEINPROGRESS should only (if ever) be indicated by
     * connect() and is semantically equivalent to WSAEALREADY (and by extension to EALREADY).
     *
     * Long story:
     *
     * On BSD sockets EINPROGRESS is indicated when an operation that takes a long time to complete (such as a
     * connect(2)) was attempted on a non-blocking socket (see ioctl(2)). On windows this is indicated by
     * WSAEWOULDBLOCK because WSAEINPROGRESS only applies to blocking calls!. When winsock was introduced (v1.x),
     * Microsoft operating systems (MS-DOS/Win3.x/Win95) did not support preemptive scheduling so blocking operations
     * that could not complete immediately had to be implemented using a pseudo-blocking mechanism that had to blend
     * in the application main loop whlie handling windows messages so the UI would not hang. This is more or less
     * described in this MSN article:
     * https://docs.microsoft.com/en-us/windows/win32/winsock/windows-sockets-1-1-blocking-routines-and-einprogress-2.
     *
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
     * that ICMP packets may be dropped by other routers or blocked by firewalls though. This error should be ignored
     * for udp sockets in calrinet_socket_setopt(), calrinet_socket_send() and calrinet_socket_recv().
     */

    switch (err)
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
        case WSASYSCALLFAILURE:             /* Syscall failed */
            return CLARINET_ESYS;
        case WSA_NOT_ENOUGH_MEMORY:         /* Not enough memory. Not indicated by any specific function supposedly could be reported by anyone. */
            return CLARINET_ENOMEM;
        case WSAEINTR:                      /* A blocking operation was interrupted.*/
            return CLARINET_EINTR;
        case WSAEOPNOTSUPP:                 /* Internal socket mishandling. For example: trying to set/get an unsupported socket opt, calling accept() on a UDP socket or using incompatible send/recv flags. */
        case WSAESOCKTNOSUPPORT:            /* Socket type not suported. This error should not normally be relayed to the end-user. */
            return CLARINET_ENOTSUP;
        case WSAEPROTOTYPE:                 /* The specified protocol is the wrong type for this socket. May be returned by connect(2). */
        case WSAEPROTONOSUPPORT:            /* Protocol not supported. */
        case WSAENOPROTOOPT:                /* Option is not supported by protocol. May be returned by setsockopt(2). */
            return CLARINET_EPROTONOSUPPORT;
        case WSAEACCES:                     /* Permission denied */
            return CLARINET_EACCES;
        case WSAEMFILE:                     /* Too many open sockets */
            return CLARINET_EMFILE;
        case WSAEWOULDBLOCK:                /* Operations on a nonblocking socket could not be completed immediately */
            return CLARINET_EAGAIN;
        case WSAEINPROGRESS:                /* A blocking operation is currently executing. Winsock only allows a single blocking operation—per-task/thread */
        case WSAEALREADY:                   /* In other cases when an operation is already in progress a function may fail with WSAEALREADY. */
            return CLARINET_EALREADY;
        case WSAENOTSOCK:                   /* An operation was attempted on something that is not a socket. */
            return CLARINET_ENOTSOCK;
        case WSAEMSGSIZE:                   /* Datagram is larger than the internal message buffer */
            return CLARINET_EMSGSIZE;
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

#elif defined(__unix__)

/* All references to errno are thread-safe. POSIX guarantees that errno is a thread-specifc global.
 * Error codes returned by getaddrinfo() require specific mapping
 */

int
clarinet_get_sockapi_error(void)
{
    return errno;
}

void
clarinet_set_sockapi_error(int err)
{
    errno = err;
}

int
clarinet_error_from_sockapi_error(const int err)
{
    /* EINPROGRESS can only be generated by connect() in which case it has the same meaning as EWOULDBLOCK. */

    /* EWOULDBLOCK and EAGAIN still have to be relayed to the user from send() and recv(). Both have the same value on
     * LINUX but not necessarily on other unix systems, although they should normally be interchangeable. */

    /* @formatter:off */
    switch (err)
    {
        case 0:
            return CLARINET_ENONE;
        case EBADF:                         /* Bad file descriptor */
        case EFAULT:                        /* Bad address (normally a null ptr)*/
        case ENAMETOOLONG:                  /* Buffer is too small for name */
        case EINVAL:                        /* Invalid argument */
        case EAFNOSUPPORT:                  /* Address family not supported by protocol */
        case EPFNOSUPPORT:                  /* Protocol family not supported  */
            return CLARINET_EINVAL;
        case ENOSPC:                        /* No space left on device */
        case EDQUOT:                        /* Disc quota exceeded */
        case EIO:                           /* An I/O e rror occurred. */
            return CLARINET_EIO;
        case ENOMEM:                        /* Out of memory */
            return CLARINET_ENOMEM;
        case EINTR:                         /* Call was interrupted by a signal */
            return CLARINET_EINTR;
        case ENOTSUP:                       /* Operation not supported */
        #if !HAVE_ENOTSUP_EQUAL_TO_EOPNOTSUPP
        case EOPNOTSUPP:                    /* Internal socket mishandling. For example: trying to set/get an unsupported socket opt, calling accept() on a udp socket or using incompatible send/recv flags. */
        #endif
        case ESOCKTNOSUPPORT:               /* Socket type not suported. This error should not normally be relayed to the end-user. */
            return CLARINET_ENOTSUP;
        case EPROTOTYPE:                    /* The specified protocol is the wrong type for this socket. May be returned by connect(2). */
        case EPROTONOSUPPORT:               /* Protocol not supported. This error should not normally be relayed to the end-user. */
        case ENOPROTOOPT:                   /* Option is not supported by protocol. May be returned by setsockopt(2). */
            return CLARINET_EPROTONOSUPPORT;
        case EACCES:                        /* Permission denied */
            return CLARINET_EACCES;
        case EMFILE:                        /* Too many open sockets */
            return CLARINET_EMFILE;
        case EINPROGRESS:                   /* Connection could not be completed immediately. */
        #if !HAVE_EAGAIN_EQUAL_TO_EWOULDBLOCK
        case EAGAIN:                        /* Operation could not be executed this time, try again. */
        #endif
        case EWOULDBLOCK:                   /* Operation could not be completed immediately. */
            return CLARINET_EAGAIN;
        case EALREADY:                      /* Operation is already in progress. */
            return CLARINET_EALREADY;
        case ENOTSOCK:                      /* An operation was attempted on something that is not a socket. */
            return CLARINET_ENOTSOCK;
        case EMSGSIZE:                      /* Datagram is larger than the internal message buffer */
            return CLARINET_EMSGSIZE;
        case EPROTO:                        /* Protocol error */
            return CLARINET_EPROTO;
        case EADDRINUSE:                    /* Address already in use. */
            return CLARINET_EADDRINUSE;
        case EADDRNOTAVAIL:                 /* This normally results from an attempt to bind to an address that is not valid for the local computer. */
            return CLARINET_EADDRNOTAVAIL;
        case ENETDOWN:                      /* Network is down */
            return CLARINET_ENETDOWN;
        case ENETUNREACH:                   /* Network is unreachable */
            return CLARINET_ENETUNREACH;
        case ENETRESET:                     /* The connection has been broken due to keep-alive activity detecting a failure while the operation was in progress. */
            return CLARINET_ENETRESET;
        case ECONNABORTED:                  /* An established connection was aborted by the software in your host computer (local)*/
            return CLARINET_ECONNABORTED;
        case ECONNRESET:                    /* Connection reset by peer. An existing connection was forcibly closed by the remote host. */
            return CLARINET_ECONNRESET;
        case ENOBUFS:                       /* No buffer space available or queue is full */
            return CLARINET_ENOBUFS;
        case EISCONN:                       /* Socket is already connected */
            return CLARINET_EISCONN;
        case EPIPE:                         /* The local end has been shut down on a connection oriented socket. May be returned by send(2) */
        case ENOTCONN:                      /* Socket is not connected */
            return CLARINET_ENOTCONN;
        case ESHUTDOWN:                     /* Cannot send after socket is shutdown */
            return CLARINET_ECONNSHUTDOWN;
        case ETIMEDOUT:                     /* Connection timed out */
            return CLARINET_ECONNTIMEOUT;
        case ECONNREFUSED:                  /* Connection refused */
            return CLARINET_ECONNREFUSED;
        case EHOSTDOWN:                     /* A socket operation failed because the destination host is down. Currently, there is no documented winsock function that returns this error. */
            return CLARINET_EHOSTDOWN;
        case EHOSTUNREACH:                  /* No route to host. */
            return CLARINET_EHOSTUNREACH;
        default:
            return CLARINET_EDEFAULT;
    }
    /* @formatter:on */
}

#else
#error "Portable error functions must be defined for this platform."
#endif
