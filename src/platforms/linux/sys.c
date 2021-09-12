#include "platforms/linux/sys.h"
#include "portable/addr.h"

#include <unistd.h>
#include <errno.h>

/* NOTES:
 *   - All references to errno are thread-safe. POSIX guarantees that errno is a thread-specifc global.
 *
 */

int 
clarinet_error_from_errno(const int err)
{
    /* EWOULDBLOCK, EAGAIN and EINPROGRESS should never be relayed to the end-user. All operations are non-blocking 
     * after all so this information is redundant. It cannot even be considered an error. */
    assert(err != EWOULDBLOCK); 
    assert(err != EAGAIN); 
    assert(err != EINPROGRESS); 
    
    /* EOPNOTSUPP should never be relayed to the end-user. It can only happen due to socket mishandling. For example by 
     * calling accept() on a udp socket or using incompatible send/recv flags. */
    assert(err != EOPNOTSUPP);
    
    switch(err)
    {
        case 0: 
            return CLARINET_ENONE;
        /* TODO */        
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
int
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
    return CLARINET_ENONE;
}


int
clarinet_socket_close(clarinet_socket** spp,
                      void(*destructor)(clarinet_socket* sp))
{
    if (!spp || !*spp)
        return CLARINET_EINVAL;

    const int sockfd = (*spp)->handle;
    if (close(sockfd) == SOCKET_ERROR)
    {
        int err = errno;
        if (err == EWOULDBLOCK || err == EAGAIN)
        {
            /* close(2) MAY only fail with  EBADF, EINTR and EIO. Both ENOSPC, EDQUOT only apply to actual files.           
             * In any case we handle the possibility of EWOULDBLOCK/EAGAIN being returned when the socket is non-blocking, 
             * there is data to flush and a non-zero linger timeout is active. In this case we try to set the socket to 
             * blocking and close again. We do not force SO_LINGER(0) here because it is not clear what the user 
             * intentions are. There is no point in having a boolean argument for that either (e.g.: force) because the 
             * user can always adjust SO_LINGER or use the shorcut option SO_DONTLINGER at any time before closing the 
             * socket. If the system blocks on close() regardless of the socket being non-blocking, much better. Note 
             * that result of setsocknonblock is irrelevant. If it fails there is nothing else to do but to try to close 
             * again anyway. */
            setsocknonblock(sockfd);
            /* At this point if close is still failing with EWOULDBLOCK/EAGAIN we have no choice but to wait some time
             * and try again until the SO_LINGER timeout expires or the output buffer is flushed. We don't simply wait
             * for the whole SO_LINGER duration because the output buffer could be flushed way before that. This a
             * safety mechanism because none of this should normally happen. */
            while (1)
            {
                if (close(sockfd) == SOCKET_ERROR)
                {
                    err = errno;
                    if (err != EWOULDBLOCK && err != EAGAIN)
                        break;

                    sleep(1);
                }
                else
                {
                    return clarinet_socket_free(spp, destructor);
                }
            }
        }

        assert(err != 0);
        assert(err != EWOULDBLOCK);
        assert(err != EAGAIN);
        switch (err)
        {
            /* EBADF means the socket is in an unrecoverable state so the only thing to do is free the allocated memory */
            case EBADF:
                break;
            /* Anything else should include EINTR and EIO which require the end-user to try again. ENOSPC and EDQUOT are not 
             * expected because they should only apply to actual files. Finally, if anything unexpected is returned the user
             * can at least be made aware and try to close again */
            default:
                return clarinet_error_from_errno(err);
        }
    }

    return clarinet_socket_free(spp, destructor);
}

int
clarinet_socket_get_endpoint(clarinet_socket* restrict sp,
                             clarinet_endpoint* restrict endpoint)
{
    if (!sp | !endpoint)
        return CLARINET_EINVAL;

    struct sockaddr_storage ss = {0};
    socklen_t length = sizeof(ss);

    if (getsockname(sp->handle, (struct sockaddr*)&ss, &length) == SOCKET_ERROR)
        return clarinet_error_from_errno(errno);

    return clarinet_endpoint_from_sockaddr(endpoint, &ss);
}
