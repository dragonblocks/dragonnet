#include <dragonport/asprintf.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "addr.h"

DragonnetAddr dragonnet_addr_parse_str(char *str)
{
	DragonnetAddr addr = {0};

	size_t colon_i = 0;
	for (ssize_t i = strlen(str)-1; i >= 0; --i) {
		if (str[i] == ':') {
			colon_i = i;
			break;
		}
	}

	for (size_t i = 0; i < strlen(str); ++i) {
		if (i < colon_i)
			addr.ip[i] = str[i];
		else if (i > colon_i)
			addr.port[i-colon_i-1] = str[i];
	}

	return addr;
}

char *dragonnet_addr_str(DragonnetAddr addr)
{
	char *ptr;
	asprintf(&ptr, "[%s]:%s", addr.ip, addr.port);
	return ptr;
}

DragonnetAddr dragonnet_addr_parse_sock(struct sockaddr_in6 ai_addr)
{
	DragonnetAddr addr = {0};
	sprintf(addr.port, "%d", ntohs(ai_addr.sin6_port));
	inet_ntop(AF_INET6, &ai_addr.sin6_addr, addr.ip, INET6_ADDRSTRLEN);

	return addr;
}

struct sockaddr_in6 dragonnet_addr_sock(DragonnetAddr addr)
{
	struct sockaddr_in6 ai_addr = {0};
	ai_addr.sin6_family = AF_INET6;
	ai_addr.sin6_flowinfo = 0;
	ai_addr.sin6_port = htons(atoi(addr.port));
	inet_pton(AF_INET6, addr.ip, &ai_addr.sin6_addr);

	return ai_addr;
}
