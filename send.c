#include <errno.h>
#include <stdio.h>

#include <dragonnet/send.h>

void send_raw(DragonnetPeer *p, bool submit, const void *buf, size_t n)
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
