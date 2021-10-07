#include <stdio.h>
#include <unistd.h>

#include <dragonnet/recv.h>

void dragonnet_recv_raw(DragonnetPeer *p, void *buf, size_t n)
{
	pthread_rwlock_rdlock(&p->mu);
	int sock = p->sock;
	pthread_rwlock_unlock(&p->mu);

	ssize_t len = recv(sock, buf, n, MSG_WAITALL);
	if (len < 0) {
		perror("recv");
		dragonnet_peer_delete(p);
		return;
	}

	// Connection closed
	if (len == 0) {
		pthread_rwlock_wrlock(&p->mu);

		close(p->sock);
		p->sock = -1;
		p->state++;

		pthread_rwlock_unlock(&p->mu);
	}
}
