/* include tcp_connect */
#include "crawler.h"

int tcpConnect(const char *host, const char *serv)
{
	int				sockfd, n;
	struct addrinfo	hints, *res, *ressave;

	bzero(&hints, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ( (n = getaddrinfo(host, serv, &hints, &res)) != 0)
	{
		printf("tcp_connect error for %s, %s: %s\n",
				 host, serv, gai_strerror(n));
		exit(1);
	}
	ressave = res;

	do {
		sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (sockfd < 0)
			continue;	/* ignore this one */

		if (connect(sockfd, res->ai_addr, res->ai_addrlen) == 0)
			break;		/* success */

		close(sockfd);	/* ignore this one */
	} while ( (res = res->ai_next) != NULL);

	if (res == NULL)	/* errno set from final connect() */
	{
		printf("tcp_connect error for %s, %s\n", host, serv);
		exit(1);
	}

	freeaddrinfo(ressave);
	printf("tcp connect success for %s, %s\n",host,serv);

	return(sockfd);
}
/* end tcp_connect */

/*
 * We place the wrapper function here, not in wraplib.c, because some
 * XTI programs need to include wraplib.c, and it also defines
 * a Tcp_connect() function.
 */

int Tcp_connect(const char *host, const char *serv)
{
	return(tcpConnect(host, serv));
}
