#ifndef _DRAGONNET_PEER_H_
#define _DRAGONNET_PEER_H_

#include <dragonnet/addr.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

typedef uint16_t DragonnetTypeId;

typedef struct dragonnet_peer {
	int sock;
	DragonnetAddr laddr, raddr;
	pthread_t recv_thread;
	pthread_mutex_t mtx;

	void (*on_disconnect)(struct dragonnet_peer *);
	bool (*on_recv)(struct dragonnet_peer *, DragonnetTypeId, void *);
	void (**on_recv_type)(struct dragonnet_peer *, void *);

	void *extra;
} DragonnetPeer;

DragonnetPeer *dragonnet_connect(char *addr);
void dragonnet_peer_run(DragonnetPeer *p);
void dragonnet_peer_shutdown(DragonnetPeer *p);

#endif
