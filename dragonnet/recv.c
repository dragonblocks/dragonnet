#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "error.h"
#include "recv.h"
#include "sock.h"

ssize_t dragonnet_recv_wrapper(int sock, char *buf, size_t len)
{
#ifdef _WIN32
	size_t rem = len;

	while (rem) {
		ssize_t num = recv(sock, buf, rem, 0);

		if (num < 0)
			return num;

		if (num == 0)
			return len - rem;

		buf += num;
		rem -= num;
	}

	return len;
#else // _WIN32
	return recv(sock, buf, len, MSG_WAITALL);
#endif // _WIN32
}

bool dragonnet_recv_raw(DragonnetPeer *p, void *buf, size_t n)
{
	if (n == 0)
		return true;

	ssize_t len = dragonnet_recv_wrapper(p->sock, buf, n);
	if (len < 0) {
		dragonnet_perror("recv");
		abort();
	}

	return len == (ssize_t) n;
}
