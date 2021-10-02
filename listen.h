#ifndef _DRAGONNET_LISTEN_H_
#define _DRAGONNET_LISTEN_H_

#include <stdbool.h>

#include "peer.h"

typedef enum {
	DRAGONNET_LISTENER_CREATED,
	DRAGONNET_LISTENER_ACTIVE,
	DRAGONNET_LISTENER_CLOSED
} DragonnetListenerState;

typedef struct {
	int sock;
	struct addrinfo *laddr;
	void (*on_connect)(DragonnetPeer *p);
	DragonnetListenerState state;

	pthread_rwlock_t *mu;
} DragonnetListener;

DragonnetListener *dragonnet_listener_new(char *addr, void (*on_connect)(DragonnetPeer *p));
void dragonnet_listener_run(DragonnetListener *l);
void dragonnet_listener_close(DragonnetListener *l);
void dragonnet_listener_delete(DragonnetListener *l);

#endif
