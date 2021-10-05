#include <dragonnet/recv.h>

static void rcv(DragonnetPeer *p, const void *buf, size_t n)
{
	pthread_rwlock_rdlock(&p->mu);
	int sock = p->sock;
	pthread_rwlock_unlock(&p->mu);

	ssize_t len = recv(sock, buf, n, MSG_WAITALL);
	if (len < 0) {
		perror("recv");
		dragonnet_peer_delete(p);
		return;
	}

	// Connection closed
	if (len == 0) {
		pthread_rwlock_wrlock(&p->mu);

		close(p->sock);
		p->sock = -1;
		p->state++;

		pthread_rwlock_unlock(&p->mu);
	}
}

u8 recv_u8(DragonnetPeer *p)
{
	u8 v;
	rcv(p, &v, sizeof v);
	return v;
}

s8 recv_s8(DragonnetPeer *p)
{
	return (s8) recv_u8(p);
}

u16 recv_u16(DragonnetPeer *p)
{
	u16 be;
	rcv(p, &be, sizeof be);
	return be16toh(be);
}

s16 recv_s16(DragonnetPeer *p)
{
	return (s16) recv_u16(p);
}

u32 recv_u32(DragonnetPeer *p)
{
	u32 be;
	rcv(p, &be, sizeof be);
	return be32toh(be);
}

s32 recv_s32(DragonnetPeer *p)
{
	return (s32) recv_u32(p);
}

u64 recv_u64(DragonnetPeer *p)
{
	u64 be;
	rcv(p, &be, sizeof be);
	return be64toh(be);
}

s64 recv_s64(DragonnetPeer *p)
{
	return (s64) recv_u64(p);
}

f32 recv_f32(DragonnetPeer *p)
{
	return (f32) recv_u32(p);
}

f64 recv_f64(DragonnetPeer *p)
{
	return (f64) recv_u64(p);
}
