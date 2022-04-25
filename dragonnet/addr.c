#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "addr.h"

struct addrinfo *dragonnet_str2addr(const char *str)
{
	const char *port = str + strlen(str) - 1;
	while (port >= str && *port != ':')
		port--;
	port++;

	const char *host_begin = str;
	if (*host_begin == '[')
		host_begin++;

	const char *host_end = port - 2;
	if (host_end >= str && *host_end == ']')
		host_end--;

	ssize_t host_len = host_end - host_begin + 1;
	if (host_len < 0)
		host_len = 0;

	char host[host_len + 1];
	host[host_len] = '\0';
	memcpy(host, host_begin, host_len);

	struct addrinfo *result, hints = {0};
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	int err;
	if ((err = getaddrinfo(host, port, &hints, &result))) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
		return NULL;
	}

	return result;
}

char *dragonnet_addr2str(struct sockaddr *addr, socklen_t addr_len)
{
	char host[NI_MAXHOST], port[NI_MAXSERV];

	int err;
	if ((err = getnameinfo(addr, addr_len, host, NI_MAXHOST, port, NI_MAXSERV, NI_NUMERICHOST | NI_NUMERICSERV))) {
		fprintf(stderr, "getnameinfo: %s\n", gai_strerror(err));
		return NULL;
	}

	char str[1 + strlen(host) + 1 + 1 + strlen(port) + 1];
	sprintf(str, "[%s]:%s", host, port);
	return strdup(str);
}
