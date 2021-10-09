#ifndef _DRAGONNET_SEND_H_
#define _DRAGONNET_SEND_H_

#include <stdbool.h>

#include <dragonnet/peer.h>

void dragonnet_send_raw(DragonnetPeer *p, bool submit, const void *buf, size_t n);
void dragonnet_write_raw(u8 **buf, size_t *n, const void *data, size_t len);

#endif
