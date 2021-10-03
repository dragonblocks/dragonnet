#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "addr.h"

DragonnetAddr dragonnet_addr_parse_str(char *str)
{
	// Reverse string for easier splitting
	char buf[1+strlen(str)];
	memset(buf, 0, sizeof buf);

	for (size_t i = 0; i < strlen(str); ++i)
		buf[i] = str[strlen(str)-1-i];

	char *r_port = strtok(buf, ":");
	char r_ip_addr[2+INET6_ADDRSTRLEN];

	char *tok = NULL;
	while (tok != NULL) {
		tok = strtok(NULL, ":");
		strcat(r_ip_addr, tok);
	}

	// Reverse strings again
	char ip_addr[1+strlen(r_ip_addr)];
	memset(ip_addr, 0, sizeof ip_addr);

	for (size_t i = 0; i < strlen(r_ip_addr); ++i)
		ip_addr[i] = r_ip_addr[strlen(r_ip_addr)-1-i];

	char port[1+strlen(r_port)];
	memset(port, 0, sizeof port);

	for (size_t i = 0; i < strlen(r_port); ++i)
		port[i] = r_port[strlen(r_port)-1-i];

	DragonnetAddr addr = {0};
	strcpy(addr.ip, ip_addr);
	strcpy(addr.port, port);

	return addr;
}

void dragonnet_addr_str(char dst[7+INET6_ADDRSTRLEN], DragonnetAddr addr)
{
	memset(dst, 0, 7+INET6_ADDRSTRLEN);
	sprintf(dst, "[%s]:%s", addr.ip, addr.port);
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
