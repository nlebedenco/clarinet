#include "clarinet/clarinet.h"
#include "config.h"

#include <stdio.h>

uint32_t 
clarinet_getlibnver(void)
{
    return (CONFIG_VERSION_MAJOR << 24) 
         | (CONFIG_VERSION_MINOR << 16) 
         | (CONFIG_VERSION_PATCH << 8);
}

const char* 
clarinet_getlibsver(void)
{
    return (CLARINET_XSTR(CONFIG_VERSION_MAJOR)"."CLARINET_XSTR(CONFIG_VERSION_MINOR)"."CLARINET_XSTR(CONFIG_VERSION_PATCH));
}

const char* 
clarinet_getlibname(void)
{
    return CONFIG_NAME;
}

const char* 
clarinet_getlibdesc(void)
{
    return CONFIG_DESCRIPTION;
}

int 
clarinet_initialize(void)
{
    return CLARINET_ENONE;
}

int 
clarinet_finalize(void)
{
    return CLARINET_ENONE;
}

