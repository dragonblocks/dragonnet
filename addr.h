#ifndef _DRAGONNET_ADDR_H_
#define _DRAGONNET_ADDR_H_

typedef struct {
	char ip[INET6_ADDRSTRLEN];
	char port[5];
} DragonnetAddr;

DragonnetAddr dragonnet_addr_parse(char *addr);

#endif
