#ifndef _DRAGONNET_PEER_H_
#define _DRAGONNET_PEER_H_

#include <dragonnet/addr.h>
#include <dragontype/number.h>
#include <pthread.h>
#include <stdbool.h>

typedef enum {
	DRAGONNET_PEER_CREATED,
	DRAGONNET_PEER_ACTIVE,
	DRAGONNET_PEER_CLOSED
} DragonnetPeerState;

typedef struct dragonnet_peer {
	int sock;
	DragonnetAddr laddr, raddr;
	DragonnetPeerState state;
	pthread_t recv_thread;

	bool (*on_recv)(struct dragonnet_peer *, u16, void *);
	void (**on_recv_type)(struct dragonnet_peer *, void *);

	pthread_rwlock_t mu;
} DragonnetPeer;

DragonnetPeer *dragonnet_connect(char *addr);
void dragonnet_peer_set_recv_hook(DragonnetPeer *p, u16 type_id,
		void (*on_recv)(struct dragonnet_peer *, void *));
void dragonnet_peer_run(DragonnetPeer *p);
void dragonnet_peer_close(DragonnetPeer *p);
void dragonnet_peer_delete(DragonnetPeer *p);

#endif
