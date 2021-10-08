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
	DragonnetAddr laddr;
	DragonnetListenerState state;
	pthread_t accept_thread;

	void (*on_connect)(DragonnetPeer *);
	void (*on_recv_type)(struct dragonnet_peer *, u16);

	pthread_rwlock_t mu;
} DragonnetListener;

DragonnetListener *dragonnet_listener_new(char *addr,
		void (*on_connect)(DragonnetPeer *p),
		void (*on_recv_type)(struct dragonnet_peer *, u16));
void dragonnet_listener_run(DragonnetListener *l);
void dragonnet_listener_close(DragonnetListener *l);
void dragonnet_listener_delete(DragonnetListener *l);

#endif
