#include <assert.h>
#include <dragonnet/addr.h>
#include <dragonnet/peer.h>
#include <dragonnet/recv.h>
#include <dragonnet/recv_thread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "sock.h"

static bool dragonnet_peer_init(DragonnetPeer *p, char *addr)
{
	pthread_mutex_init(&p->mtx, NULL);

	struct addrinfo *info = dragonnet_str2addr(addr);
	if (!info)
		return false;

	p->sock = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
	p->address = dragonnet_addr2str(info->ai_addr, info->ai_addrlen);

	if (connect(p->sock, info->ai_addr, info->ai_addrlen) < 0) {
		freeaddrinfo(info);
		perror("connect");
		return false;
	}

	freeaddrinfo(info);

	p->on_disconnect = NULL;
	p->on_recv = NULL;
	p->on_recv_type = calloc(sizeof *p->on_recv_type, dragonnet_num_types); // fixme: memory leak

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
