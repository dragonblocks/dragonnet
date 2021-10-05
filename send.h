#ifndef _DRAGONNET_SEND_H_
#define _DRAGONNET_SEND_H_

#include <stdbool.h>

#include <dragontype/number.h>
#include <dragonnet/peer.h>

void send_u8(DragonnetPeer *p, bool confirm, u8 v);
void send_s8(DragonnetPeer *p, bool confirm, s8 v);
void send_u16(DragonnetPeer *p, bool confirm, u16 v);
void send_s16(DragonnetPeer *p, bool confirm, s16 v);
void send_u32(DragonnetPeer *p, bool confirm, u32 v);
void send_s32(DragonnetPeer *p, bool confirm, s32 v);
void send_u64(DragonnetPeer *p, bool confirm, u64 v);
void send_s64(DragonnetPeer *p, bool confirm, s64 v);
void send_f32(DragonnetPeer *p, bool confirm, f32 v);
void send_f64(DragonnetPeer *p, bool confirm, f64 v);

#endif
