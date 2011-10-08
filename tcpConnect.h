#include "crawler.h"

int tcpConnect(const char *host, const char *serv)
{
	int				sockfd, n;
	struct addrinfo	hints, *res, *ressave;
	char logStr[LOG_LENGTH];

	bzero(&hints, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ( (n = getaddrinfo(host, serv, &hints, &res)) != 0)
	{
		snprintf(logStr,sizeof(logStr),"tcp_connect error getaddrinfo for %s, %s: %s",
				 host, serv, gai_strerror(n));
		logger(logStr);
		return -1;
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
		snprintf(logStr,sizeof(logStr),"tcp_connect error get socket for %s, %s", host, serv);
		logger(logStr);
		return -1;
	}

	freeaddrinfo(ressave);
	snprintf(logStr,sizeof(logStr),"tcp connect success for %s, %s",host,serv);
	logger(logStr);

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
