#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dragonnet/send.h>

void dragonnet_send_raw(DragonnetPeer *p, bool submit, const void *buf, size_t n)
{
	pthread_rwlock_rdlock(&p->mu);
	int sock = p->sock;
	pthread_rwlock_unlock(&p->mu);

	ssize_t len = send(sock, buf, n, MSG_NOSIGNAL | (submit ? 0 : MSG_MORE));
	if (len < 0) {
		if (errno == EPIPE) {
			dragonnet_peer_close(p);
			return;
		}

		perror("send");
		dragonnet_peer_delete(p);
	}
}

void dragonnet_write_raw(u8 **buf, size_t *n, const void *data, size_t len)
{
	*buf = realloc(*buf, len + *n);
	memcpy(&((*buf)[*n]), data, len);
	*n += len;
}
