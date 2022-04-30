#ifndef _DRAGONNET_LISTEN_H_
#define _DRAGONNET_LISTEN_H_

#include <stdbool.h>
#include "peer.h"

typedef struct {
	int sock;
	char *address;

	pthread_t accept_thread;
	bool active;
#ifndef _WIN32
	int intr[2];
#endif // _WIN32

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
