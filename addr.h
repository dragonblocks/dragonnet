#ifndef _DRAGONNET_ADDR_H_
#define _DRAGONNET_ADDR_H_

#include <arpa/inet.h>

typedef struct {
	char ip[INET6_ADDRSTRLEN];
	char port[5];
} DragonnetAddr;

DragonnetAddr dragonnet_addr_parse_str(char *addr);
void dragonnet_addr_str(char dst[7+INET6_ADDRSTRLEN], DragonnetAddr addr);

DragonnetAddr dragonnet_addr_parse_sock(struct sockaddr_in6 ai_addr);
struct sockaddr_in6 dragonnet_addr_sock(DragonnetAddr addr);

#endif