#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "peer.h"

DragonnetPeer *dragonnet_connect(char *addr)
{
	DragonnetPeer *p = malloc(sizeof *p);
	p->mu = malloc(sizeof *p->mu);
	pthread_rwlock_init(p->mu, NULL);
	pthread_rwlock_wrlock(p->mu);

	p->sock = socket(AF_INET6, SOCK_STREAM, 0);
	p->raddr = dragonnet_addr_parse_str(addr);

	struct sockaddr_in6 sock_addr = dragonnet_addr_sock(p->raddr);
	if (connect(p->sock, (const struct sockaddr *) &sock_addr, sizeof sock_addr) < 0) {
		perror("connect");
		dragonnet_peer_delete(p);
		return NULL;
	}

	struct sockaddr_in6 sock_name;
	socklen_t sock_namelen = sizeof sock_name;

	if (getsockname(p->sock, (struct sockaddr *) &sock_name, &sock_namelen) < 0) {
		perror("getsockname");
		dragonnet_peer_delete(p);
		return NULL;
	}

	p->laddr = dragonnet_addr_parse_sock(sock_name);

	pthread_rwlock_unlock(p->mu);
	return p;
}

void dragonnet_peer_close(DragonnetPeer *p)
{
	pthread_rwlock_wrlock(p->mu);

	assert(p->state == DRAGONNET_PEER_ACTIVE);
	shutdown(p->sock, SHUT_RDWR);
	p->state++;

	pthread_rwlock_unlock(p->mu);
}

void dragonnet_peer_delete(DragonnetPeer *p)
{
	pthread_rwlock_destroy(p->mu);
	free(p);
}
