#define _GNU_SOURCE
#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include "addr.h"
#include "error.h"
#include "listen.h"
#include "recv.h"
#include "sock.h"

// ----
// Peer
// ----

static bool dragonnet_peer_init_accepted(DragonnetPeer *p, int sock,
		char *addr, DragonnetListener *l)
{
	pthread_mutex_init(&p->mtx, NULL);

	p->sock = sock;
	p->address = addr;
	p->on_disconnect = l->on_disconnect;
	p->on_recv = l->on_recv;
	p->on_recv_type = l->on_recv_type;

	return true;
}

static DragonnetPeer *dragonnet_peer_accept(int sock, char *addr,
		DragonnetListener *l)
{
	DragonnetPeer *p = malloc(sizeof *p);
	if (!dragonnet_peer_init_accepted(p, sock, addr, l)) {
		pthread_mutex_destroy(&p->mtx);
		free(p);
		free(addr);
		return NULL;
	}

	return p;
}

// --------
// Listener
// --------

DragonnetListener *dragonnet_listener_new(char *addr)
{
	struct addrinfo *info = dragonnet_str2addr(addr);
	if (!info)
		return NULL;

	DragonnetListener *l = malloc(sizeof *l);

	if ((l->sock = socket(info->ai_family, info->ai_socktype, info->ai_protocol)) < 0) {
		dragonnet_perror("socket");
		freeaddrinfo(info);
		free(l);
		return NULL;
	}

	l->active = true;
	l->address = dragonnet_addr2str(info->ai_addr, info->ai_addrlen);
	l->on_connect = NULL;
	l->on_disconnect = NULL;
	l->on_recv = NULL;
	l->on_recv_type = calloc(sizeof *l->on_recv_type, dragonnet_num_types);

	int so_reuseaddr = 1;
	if (setsockopt(l->sock, SOL_SOCKET, SO_REUSEADDR, (void *) &so_reuseaddr,
			sizeof so_reuseaddr) < 0) {
		dragonnet_perror("setsockopt");
		freeaddrinfo(info);
		dragonnet_listener_delete(l);
		return NULL;
	}

	if (bind(l->sock, info->ai_addr, info->ai_addrlen) < 0) {
		dragonnet_perror("bind");
		freeaddrinfo(info);
		dragonnet_listener_delete(l);
		return NULL;
	}

	freeaddrinfo(info);

	if (listen(l->sock, 10) < 0) {
		dragonnet_perror("listen");
		dragonnet_listener_delete(l);
		return NULL;
	}

	return l;
}

static void *listener_main(void *g_listener)
{
#ifdef __GLIBC__
	pthread_setname_np(pthread_self(), "listen");
#endif

	DragonnetListener *l = (DragonnetListener *) g_listener;

	while (l->active) {
		struct sockaddr_storage clt_addr;
		socklen_t clt_addrlen = sizeof clt_addr;

		int clt_sock = accept(l->sock, (struct sockaddr *) &clt_addr, &clt_addrlen);
		if (clt_sock < 0) {
			if (!dragonnet_isintrerr())
				dragonnet_perror("accept");
			continue;
		}

		char *clt_addstr =  dragonnet_addr2str((struct sockaddr *) &clt_addr, clt_addrlen);
		DragonnetPeer *p = dragonnet_peer_accept(clt_sock, clt_addstr, l);
		if (p == NULL)
			continue;

		void (*on_connect)(DragonnetPeer *) = l->on_connect;

		if (on_connect != NULL)
			on_connect(p);

		dragonnet_peer_run(p);
	}

	return NULL;
}

void dragonnet_listener_run(DragonnetListener *l)
{
	pthread_create(&l->accept_thread, NULL, &listener_main, l);
}

void dragonnet_listener_close(DragonnetListener *l)
{
	l->active = false;

	pthread_kill(l->accept_thread, SIGINT);
	pthread_join(l->accept_thread, NULL);
}

void dragonnet_listener_delete(DragonnetListener *l)
{
	free(l->on_recv_type);
	free(l->address);
	free(l);
}
