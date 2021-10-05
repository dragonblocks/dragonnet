#ifndef _DRAGONNET_RECV_H_
#define _DRAGONNET_RECV_H_

#include <dragontype/number.h>
#include <dragonnet/peer.h>

u8 recv_u8(DragonnetPeer *p);
s8 recv_s8(DragonnetPeer *p);
u16 recv_u16(DragonnetPeer *p);
s16 recv_s16(DragonnetPeer *p);
u32 recv_u32(DragonnetPeer *p);
s32 recv_s32(DragonnetPeer *p);
u64 recv_u64(DragonnetPeer *p);
s64 recv_s64(DragonnetPeer *p);
f32 recv_f32(DragonnetPeer *p);
f64 recv_f64(DragonnetPeer *p);

#endif
