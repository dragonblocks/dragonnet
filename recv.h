#ifndef _DRAGONNET_RECV_H_
#define _DRAGONNET_RECV_H_

#include <dragonnet/peer.h>

void dragonnet_recv_raw(DragonnetPeer *p, void *buf, size_t n);
void dragonnet_read_raw(u8 **buf, size_t *n, void *data, size_t len);

#endif
