#define _GNU_SOURCE
#include <string.h>

/* Declare it GNU-style; that will cause an error if it's not defined or not GNU-style */
extern char *strerror_r(int, char *, size_t);

int
main(void)
{
	return 0;
}
