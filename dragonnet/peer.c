#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "addr.h"
#include "error.h"
#include "peer.h"
#include "recv.h"
#include "recv_thread.h"
#include "sock.h"

static bool dragonnet_peer_init(DragonnetPeer *p, char *addr)
{
	pthread_mutex_init(&p->mtx, NULL);

	struct addrinfo *info = dragonnet_str2addr(addr);
	if (!info)
		return false;

	if ((p->sock = socket(info->ai_family, info->ai_socktype, info->ai_protocol)) < 0) {
		dragonnet_perror("socket");
		freeaddrinfo(info);
		return false;
	}
	p->address = dragonnet_addr2str(info->ai_addr, info->ai_addrlen);

	if (connect(p->sock, info->ai_addr, info->ai_addrlen) < 0) {
		dragonnet_perror("connect");
		freeaddrinfo(info);
		return false;
	}

	freeaddrinfo(info);

	p->on_disconnect = NULL;
	p->on_recv = NULL;
	p->on_recv_type = calloc(dragonnet_num_types, sizeof *p->on_recv_type); // fixme: memory leak

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
