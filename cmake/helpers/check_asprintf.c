#define _GNU_SOURCE
#include <stdio.h>

/* Declare it GNU-style; that will cause an error if it's not defined or not GNU-style */
extern int asprintf(char **restrict strp, const char *restrict fmt, ...);

int
main(void)
{
    return 0;
}
