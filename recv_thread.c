#include <assert.h>
#include <dragontype/number.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#include "peer.h"
#include "recv_thread.h"

void *dragonnet_peer_recv_thread(void *g_peer)
{
	DragonnetPeer *p = (DragonnetPeer *) g_peer;

	pthread_rwlock_wrlock(&p->mu);
	assert(p->state == DRAGONNET_PEER_CREATED);
	p->state++;
	pthread_rwlock_unlock(&p->mu);

	while (true) {
		u16 msg;

		// Copy socket fd so that shutdown doesn't block
		pthread_rwlock_rdlock(&p->mu);
		int sock = p->sock;
		pthread_rwlock_unlock(&p->mu);

		ssize_t len = recv(sock, &msg, sizeof msg, MSG_WAITALL);

		if (len < 0) {
			perror("recv");
			dragonnet_peer_delete(p);
			return NULL;
		}

		// Connection closed
		if (len == 0) {
			pthread_rwlock_wrlock(&p->mu);

			close(p->sock);
			p->sock = -1;
			p->state++;

			pthread_rwlock_unlock(&p->mu);
			return NULL;
		}

		// Deserialization
	}
}
