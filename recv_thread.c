#include <assert.h>
#include <dragontype/number.h>
#include <errno.h>
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

		pthread_rwlock_rdlock(&p->mu);
		ssize_t len = recv(p->sock, &msg, sizeof msg, MSG_WAITALL);
		pthread_rwlock_unlock(&p->mu);

		if (len < 0 && errno != EWOULDBLOCK) {
			perror("recv");
			dragonnet_peer_delete(p);
			return NULL;
		}

		// connection closed
		if ((len >= 0 && len != sizeof msg) || errno == EWOULDBLOCK) {
			pthread_rwlock_wrlock(&p->mu);

			close(p->sock);
			p->sock = 0;
			p->state++;

			pthread_rwlock_unlock(&p->mu);
			return NULL;
		}

		// deserialization
	}
}
