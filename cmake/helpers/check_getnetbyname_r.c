#include <netdb.h>

int main(void) 
{
	struct netent netent_buf;
	char buf[1024];
	struct netent *resultp;
	int h_errnoval;
	return getnetbyname_r((const char *)0, &netent_buf, buf, sizeof buf, &resultp, &h_errnoval);
}

