#ifndef _DRAGONNET_PEER_H_
#define _DRAGONNET_PEER_H_

#include <pthread.h>

#include "addr.h"

const extern struct timeval dragonnet_timeout;

typedef enum {
	DRAGONNET_PEER_CREATED,
	DRAGONNET_PEER_ACTIVE,
	DRAGONNET_PEER_CLOSED
} DragonnetPeerState;

typedef struct {
	int sock;
	DragonnetAddr laddr, raddr;
	DragonnetPeerState state;

	pthread_rwlock_t *mu;
} DragonnetPeer;

DragonnetPeer *dragonnet_connect(char *addr);
void dragonnet_peer_run(DragonnetPeer *p);
void dragonnet_peer_close(DragonnetPeer *p);
void dragonnet_peer_delete(DragonnetPeer *p);

#endif
