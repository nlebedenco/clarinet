#include <sys/socket.h> 
int main(void) 
{
    return SO_REUSEPORT ? 0 : 1; 
}
