#ifndef _DRAGONNET_SEND_H_
#define _DRAGONNET_SEND_H_

#include <dragonnet/peer.h>
#include <stdbool.h>

bool dragonnet_send_raw(DragonnetPeer *p, bool submit, const void *buf, size_t n);

#endif
