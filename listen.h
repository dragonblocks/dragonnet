#ifndef _DRAGONNET_LISTEN_H_
#define _DRAGONNET_LISTEN_H_

#include <dragonnet/peer.h>
#include <stdbool.h>

typedef struct {
	int sock;
	char *address;

	pthread_t accept_thread;
	bool active;

	void (*on_connect)(DragonnetPeer *);
	void (*on_disconnect)(DragonnetPeer *);
	bool (*on_recv)(DragonnetPeer *, DragonnetTypeId, void *);
	void (**on_recv_type)(DragonnetPeer *, void *);
} DragonnetListener;

DragonnetListener *dragonnet_listener_new(char *addr);
void dragonnet_listener_run(DragonnetListener *l);
void dragonnet_listener_close(DragonnetListener *l);
void dragonnet_listener_delete(DragonnetListener *l);

#endif
