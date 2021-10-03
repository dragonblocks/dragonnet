#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "peer.h"
#include "recv_thread.h"

const struct timeval dragonnet_timeout = {
	.tv_sec = 30,
	.tv_usec = 0
};

static bool dragonnet_peer_init(DragonnetPeer *p, char *addr)
{
	p->mu = malloc(sizeof *p->mu);
	pthread_rwlock_init(p->mu, NULL);
	pthread_rwlock_wrlock(p->mu);

	p->sock = socket(AF_INET6, SOCK_STREAM, 0);
	p->raddr = dragonnet_addr_parse_str(addr);

	if (setsockopt(p->sock, SOL_SOCKET, SO_RCVTIMEO, &dragonnet_timeout,
			sizeof dragonnet_timeout) < 0) {
		perror("setsockopt");
		return false;
	}

	if (setsockopt(p->sock, SOL_SOCKET, SO_SNDTIMEO, &dragonnet_timeout,
			sizeof dragonnet_timeout) < 0) {
		perror("setsockopt");
		return false;
	}

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

	pthread_rwlock_unlock(p->mu);
	return true;
}

DragonnetPeer *dragonnet_connect(char *addr)
{
	DragonnetPeer *p = malloc(sizeof *p);
	if (!dragonnet_peer_init(p, addr)) {
		dragonnet_peer_delete(p);
		return NULL;
	}

	return p;
}

void dragonnet_peer_run(DragonnetPeer *p)
{
	pthread_t recv_thread;
	pthread_create(&recv_thread, NULL, &dragonnet_peer_recv_thread, p);
	pthread_join(recv_thread, NULL);
}

void dragonnet_peer_close(DragonnetPeer *p)
{
	pthread_rwlock_wrlock(p->mu);

	if (p->state == DRAGONNET_PEER_ACTIVE) {
		shutdown(p->sock, SHUT_RDWR);
		p->state++;
	}

	pthread_rwlock_unlock(p->mu);
}

void dragonnet_peer_delete(DragonnetPeer *p)
{
	pthread_rwlock_destroy(p->mu);
	free(p);
}
