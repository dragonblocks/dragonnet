#include <arpa/inet.h>
#include <string.h>

#include "addr.h"

DragonnetAddr dragonnet_addr_parse(char *addr)
{
	DragonnetAddr net_addr;

	size_t port_i = 0;
	for (size_t i = 0; i < strlen(addr); ++i) {
		if (!port_i) {
			if (addr[i] != ':')
				net_addr.ip[i] = addr[i];
			else
				port_i = i+1;
		} else
			net_addr.port[i-port_i] = addr[i];
	}

	return net_addr;
}
