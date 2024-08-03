#ifndef _DRAGONNET_RECV_H_
#define _DRAGONNET_RECV_H_

#include "peer.h"

typedef struct {
	size_t siz;
	bool (*deserialize)(DragonnetPeer *, void *);
	void (*free)(void *);
} DragonnetType;

extern DragonnetTypeId dragonnet_num_types __attribute__((weak));
extern DragonnetType dragonnet_types[] __attribute__((weak));

bool dragonnet_recv_raw(DragonnetPeer *p, void *buf, size_t n);

#endif
