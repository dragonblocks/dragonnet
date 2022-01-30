#ifndef _DRAGONNET_RECV_H_
#define _DRAGONNET_RECV_H_

#include <dragonnet/peer.h>

typedef struct {
	size_t siz;
	void (*deserialize)(DragonnetPeer *, void *);
} DragonnetType;

extern DragonnetTypeId dragonnet_num_types;
extern DragonnetType dragonnet_types[];

void dragonnet_recv_raw(DragonnetPeer *p, void *buf, size_t n);

#endif
