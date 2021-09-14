#include <errno.h>

int main(void)
{
    return EAGAIN == EWOULDBLOCK ? 0 : 1;
}