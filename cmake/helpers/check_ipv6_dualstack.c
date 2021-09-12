#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

int main(void) 
{
    int exitcode = EXIT_SUCCESS;
    
    struct sockaddr_in6 addr;
    
    int sockfd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd < 0)
        return EXIT_FAILURE;

    do
    {  
        int flag = 0;
        if (setsockopt(sockfd, IPPROTO_IPV6, IPV6_V6ONLY, &flag, sizeof(flag)) < 0)
        {
            exitcode = EXIT_FAILURE;
            break;
        }
        
        flag = -1;
        socklen_t len = sizeof(flag);
        if (getsockopt(sockfd, IPPROTO_IPV6, IPV6_V6ONLY, &flag, &len) < 0)
        {
            exitcode = EXIT_FAILURE;
            break;
        }
        
        if (flag != 0)
        {
            exitcode = EXIT_FAILURE;
            break;
        }
        
        memset((char *) &addr, 0, sizeof(addr));	
        addr.sin6_family = AF_INET6;   
        if(bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
        {
            exitcode = EXIT_FAILURE;
            break;
        }
    }
    while(0);
    
    close(sockfd);
    
    return exitcode;
 }

