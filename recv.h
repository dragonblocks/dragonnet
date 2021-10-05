#ifndef _DRAGONNET_RECV_H_
#define _DRAGONNET_RECV_H_

#include <dragonnet/peer.h>

void recv_raw(DragonnetPeer *p, const void *buf, size_t n);

#endif
