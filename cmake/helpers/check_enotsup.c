#include <errno.h>

int main(void)
{
    return ENOTSUP == EOPNOTSUPP ? 0 : 1;
}
