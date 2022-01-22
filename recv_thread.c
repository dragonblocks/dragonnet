#include <assert.h>
#include <dragonnet/peer.h>
#include <dragonnet/recv.h>
#include <dragonnet/recv_thread.h>
#include <dragontype/number.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

void *dragonnet_peer_recv_thread(void *g_peer)
{
	DragonnetPeer *p = (DragonnetPeer *) g_peer;

	pthread_rwlock_wrlock(&p->mu);
	assert(p->state == DRAGONNET_PEER_CREATED);
	p->state++;
	pthread_rwlock_unlock(&p->mu);

	while (true) {
		u16 type_id;

		// Copy socket fd so that shutdown doesn't block
		pthread_rwlock_rdlock(&p->mu);
		int sock = p->sock;
		pthread_rwlock_unlock(&p->mu);

		ssize_t len = recv(sock, &type_id, sizeof type_id, MSG_WAITALL);
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

		type_id = be16toh(type_id);
		DragonnetType type = dragonnet_types[type_id];
		u8 buf[type.siz];
		type.deserialize(p, buf);

		pthread_rwlock_rdlock(&p->mu);
		bool (*on_recv)(struct dragonnet_peer *, u16, void *) = p->on_recv;
		void (*on_recv_type)(DragonnetPeer *, void *) = p->on_recv_type[type_id];
		pthread_rwlock_unlock(&p->mu);

		if (on_recv != NULL && !on_recv(p, type_id, buf))
			on_recv_type = NULL;

		if (on_recv_type != NULL)
			on_recv_type(p, buf);
	}
}
