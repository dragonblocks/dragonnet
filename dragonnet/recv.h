#ifndef _DRAGONNET_RECV_H_
#define _DRAGONNET_RECV_H_

#include "peer.h"

#ifdef WIN32
#define WEAK
#else
#define WEAK __attribute__((weak))
#endif

typedef struct {
	size_t siz;
	bool (*deserialize)(DragonnetPeer *, void *);
	void (*free)(void *);
} DragonnetType;

extern DragonnetTypeId dragonnet_num_types WEAK;
extern DragonnetType dragonnet_types[] WEAK;

bool dragonnet_recv_raw(DragonnetPeer *p, void *buf, size_t n);

#undef WEAK

#endif
