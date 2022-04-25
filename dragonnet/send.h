#ifndef _DRAGONNET_SEND_H_
#define _DRAGONNET_SEND_H_

#include <stdbool.h>
#include "peer.h"

bool dragonnet_send_raw(DragonnetPeer *p, bool submit, const void *buf, size_t n);

#endif
