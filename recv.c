#include <dragonnet/recv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

bool dragonnet_recv_raw(DragonnetPeer *p, void *buf, size_t n)
{
	if (n == 0)
		return true;

	ssize_t len = recv(p->sock, buf, n, MSG_WAITALL);
	if (len < 0) {
		perror("recv");
		exit(EXIT_FAILURE);
	}

	return len != 0;
}
