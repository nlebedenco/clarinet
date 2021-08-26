#define _GNU_SOURCE
#include <stdio.h>

/* Declare it GNU-style; that will cause an error if it's not defined or not GNU-style */
extern int vasprintf(char **restrict strp, const char *restrict fmt, va_list ap);

int
main(void)
{
    return 0;
}
