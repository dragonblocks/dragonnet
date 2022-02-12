#include <assert.h>
#include <dragonnet/peer.h>
#include <dragonnet/recv.h>
#include <dragonnet/recv_thread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static bool dragonnet_peer_init(DragonnetPeer *p, char *addr)
{
	pthread_mutex_init(&p->mtx, NULL);

	p->sock = socket(AF_INET6, SOCK_STREAM, 0);
	p->raddr = dragonnet_addr_parse_str(addr);
	p->on_disconnect = NULL;
	p->on_recv = NULL;
	p->on_recv_type = calloc(sizeof *p->on_recv_type, dragonnet_num_types);

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
	return true;
}

DragonnetPeer *dragonnet_connect(char *addr)
{
	DragonnetPeer *p = malloc(sizeof *p);
	if (!dragonnet_peer_init(p, addr)) {
		pthread_mutex_destroy(&p->mtx);
		free(p);
		return NULL;
	}

	return p;
}

void dragonnet_peer_run(DragonnetPeer *p)
{
	pthread_create(&p->recv_thread, NULL, &dragonnet_peer_recv_thread, p);
}

void dragonnet_peer_shutdown(DragonnetPeer *p)
{
	shutdown(p->sock, SHUT_RDWR);
}
