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
	void (**on_recv_type)(DragonnetPeer *, void *);

	pthread_rwlock_t mu;
} DragonnetListener;

DragonnetListener *dragonnet_listener_new(char *addr,
		void (*on_connect)(DragonnetPeer *p));
void dragonnet_listener_set_recv_hook(DragonnetListener *l, u16 type_id,
		void (*on_recv)(struct dragonnet_peer *, void *));
void dragonnet_listener_run(DragonnetListener *l);
void dragonnet_listener_close(DragonnetListener *l);
void dragonnet_listener_delete(DragonnetListener *l);

#endif
