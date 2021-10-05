#include <errno.h>
#include <stdio.h>

#include <dragonnet/send.h>

static void snd(DragonnetPeer *p, bool confirm, const void *buf, size_t n)
{
	pthread_rwlock_rdlock(&p->mu);
	int sock = p->sock;
	pthread_rwlock_unlock(&p->mu);

	ssize_t len = send(sock, buf, n, MSG_NOSIGNAL | (confirm ? 0 : MSG_MORE));
	if (len < 0) {
		if (errno == EPIPE) {
			dragonnet_peer_close(p);
			return;
		}

		perror("send");
		dragonnet_peer_delete(p);
	}
}

void send_u8(DragonnetPeer *p, bool confirm, u8 v)
{
	snd(p, confirm, &v, sizeof v);
}

void send_s8(DragonnetPeer *p, bool confirm, s8 v)
{
	send_u8(p, confirm, (u8) v);
}

void send_u16(DragonnetPeer *p, bool confirm, u16 v)
{
	u16 be = htobe16(v);
	snd(p, confirm, &be, sizeof be);
}

void send_s16(DragonnetPeer *p, bool confirm, s16 v)
{
	send_u16(p, confirm, (u16) v);
}

void send_u32(DragonnetPeer *p, bool confirm, u32 v)
{
	u32 be = htobe32(v);
	snd(p, confirm, &be, sizeof be);
}

void send_s32(DragonnetPeer *p, bool confirm, s32 v)
{
	send_u32(p, confirm, (u32) v);
}

void send_u64(DragonnetPeer *p, bool confirm, u64 v)
{
	u64 be = htobe64(v);
	snd(p, confirm, &be, sizeof be);
}

void send_s64(DragonnetPeer *p, bool confirm, s64 v)
{
	send_u64(p, confirm, (u64) v);
}

void send_f32(DragonnetPeer *p, bool confirm, f32 v)
{
	send_u32(p, confirm, (u32) v);
}

void send_f64(DragonnetPeer *p, bool confirm, f64 v)
{
	send_u64(p, confirm, (u64) v);
}
