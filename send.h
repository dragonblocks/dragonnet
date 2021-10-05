#ifndef _DRAGONNET_SEND_H_
#define _DRAGONNET_SEND_H_

#include <stdbool.h>

#include <dragonnet/peer.h>

void send_raw(DragonnetPeer *p, bool submit, const void *buf, size_t n);

#endif
