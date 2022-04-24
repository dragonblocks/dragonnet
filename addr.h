#ifndef _DRAGONNET_ADDR_H_
#define _DRAGONNET_ADDR_H_

#include "sock.h"

struct addrinfo *dragonnet_str2addr(const char *str);
char *dragonnet_addr2str(struct sockaddr *addr, socklen_t addr_len);

#endif
