#ifndef _DRAGONNET_PEER_H_
#define _DRAGONNET_PEER_H_

#include <pthread.h>

typedef enum {
	DRAGONNET_PEER_CREATED,
	DRAGONNET_PEER_ACTIVE,
	DRAGONNET_PEER_CLOSED
} DragonnetPeerState;

typedef struct {
	int sock;
	struct addrinfo *laddr;
	struct addrinfo *raddr;
	DragonnetPeerState state;

	pthread_rwlock_t *mu;
} DragonnetPeer;

DragonnetPeer *dragonnet_connect(char *addr);
void dragonnet_peer_close(DragonnetPeer *p);
void dragonnet_peer_delete(DragonnetPeer *p);

#endif
