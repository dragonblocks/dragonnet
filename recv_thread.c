#define _GNU_SOURCE
#include <assert.h>
#include <dragonnet/peer.h>
#include <dragonnet/recv.h>
#include <dragonnet/recv_thread.h>
#include <endian.h/endian.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "sock.h"

void *dragonnet_peer_recv_thread(void *g_peer)
{
#ifdef __GLIBC__
	pthread_setname_np(pthread_self(), "recv");
#endif

	DragonnetPeer *p = (DragonnetPeer *) g_peer;

	while (true) {
		DragonnetTypeId type_id;

		bool reset = false;

		ssize_t len = recv(p->sock, (void *) &type_id, sizeof type_id, MSG_WAITALL);
		if (len < 0) {
			if (errno == ECONNRESET || errno == EPIPE || errno == ETIMEDOUT) {
				reset = true;
			} else {
				perror("recv");
				abort();
			}
		}

		// Connection closed
		if (len == 0 || reset) {
			if (p->on_disconnect)
				p->on_disconnect(p);

			close(p->sock);
			free(p->address);

			pthread_mutex_destroy(&p->mtx);
			free(p);
			return NULL;
		}

		type_id = be16toh(type_id);

		if (type_id >= dragonnet_num_types) {
			fprintf(stderr, "[warning] received invalid type id %d\n", type_id);
			continue;
		}

		DragonnetType type = dragonnet_types[type_id];

		unsigned char buf[type.siz];
		memset(buf, 0, type.siz);

		if (!type.deserialize(p, buf)) {
			if (type.free != NULL)
				type.free(buf);

			fprintf(stderr, "[warning] failed to deserialize package of type %d\n", type_id);

			continue;
		}

		bool (*on_recv)(struct dragonnet_peer *, DragonnetTypeId, void *) = p->on_recv;
		void (*on_recv_type)(DragonnetPeer *, void *) = p->on_recv_type[type_id];

		if (on_recv != NULL && !on_recv(p, type_id, buf))
			on_recv_type = NULL;

		if (on_recv_type != NULL)
			on_recv_type(p, buf);

		if (type.free != NULL)
			type.free(buf);
	}
}
