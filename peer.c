#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "peer.h"
#include "recv_thread.h"

static bool dragonnet_peer_init(DragonnetPeer *p, char *addr, void (*on_recv_type)(struct dragonnet_peer *, u16))
{
	pthread_rwlock_init(&p->mu, NULL);
	pthread_rwlock_wrlock(&p->mu);

	p->sock = socket(AF_INET6, SOCK_STREAM, 0);
	p->raddr = dragonnet_addr_parse_str(addr);
	p->on_recv_type = on_recv_type;

	struct sockaddr_in6 sock_addr = dragonnet_addr_sock(p->raddr);
	if (connect(p->sock, (const struct sockaddr *) &sock_addr,
			sizeof sock_addr) < 0) {
		perror("connect");
		return false;
	}

	struct sockaddr_in6 sock_name;
	socklen_t sock_namelen = sizeof sock_name;

	if (getsockname(p->sock, (struct sockaddr *) &sock_name, &sock_namelen) < 0) {
		perror("getsockname");
		return false;
	}

	p->laddr = dragonnet_addr_parse_sock(sock_name);

	pthread_rwlock_unlock(&p->mu);
	return true;
}

DragonnetPeer *dragonnet_connect(char *addr, void (*on_recv_type)(struct dragonnet_peer *, u16))
{
	DragonnetPeer *p = malloc(sizeof *p);
	if (!dragonnet_peer_init(p, addr, on_recv_type)) {
		dragonnet_peer_delete(p);
		return NULL;
	}

	return p;
}

void dragonnet_peer_run(DragonnetPeer *p)
{
	pthread_rwlock_wrlock(&p->mu);
	pthread_create(&p->recv_thread, NULL, &dragonnet_peer_recv_thread, p);
	pthread_rwlock_unlock(&p->mu);

	while (p->state < DRAGONNET_PEER_ACTIVE);
}

void dragonnet_peer_close(DragonnetPeer *p)
{
	pthread_rwlock_wrlock(&p->mu);

	pthread_t recv_thread = p->recv_thread;
	if (p->state == DRAGONNET_PEER_ACTIVE)
		shutdown(p->sock, SHUT_RDWR);

	pthread_rwlock_unlock(&p->mu);

	pthread_cancel(recv_thread);
	pthread_join(recv_thread, NULL);
}

void dragonnet_peer_delete(DragonnetPeer *p)
{
	pthread_rwlock_destroy(&p->mu);
	free(p);
}
