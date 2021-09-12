#include "portability.h"

#include <string.h>
#include <errno.h>

#if !HAVE_STRERROR_S && HAVE_STRERROR_R && !HAVE_POSIX_STRERROR_R && HAVE_GNU_STRERROR_R

/**
 * An strerror_s implementation based on GNU strerror_r.
 */ 
int 
strerror_s(char* buf, size_t buflen, int errnum)
{
    char s[256] = {0};
	const char* r = strerror_r(errnum, s, sizeof(s));
	const size_t n = strlen(r);
    int errcode;
    size_t size;     
    if (buflen > n)
    {
        errcode = 0;
        size = n + 1;
    }
    else
    {
        errcode = ERANGE;
        size = buflen;
    }
    strlcpy(buf, r, size);
    return errcode;
}
#endif