#define _GNU_SOURCE
#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "addr.h"
#include "error.h"
#include "listen.h"
#include "recv.h"
#include "sock.h"

#define mymax(a, b) ((a) > (b) ? (a) : (b))

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
	l->address = NULL;
	l->on_connect = NULL;
	l->on_disconnect = NULL;
	l->on_recv = NULL;
	l->on_recv_type = calloc(dragonnet_num_types, sizeof *l->on_recv_type);

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

	struct sockaddr_storage bound_addr;
	socklen_t bound_addrlen = sizeof bound_addr;
	if (getsockname(l->sock, (struct sockaddr *) &bound_addr, &bound_addrlen) < -1) {
		dragonnet_perror("getsockname");
		dragonnet_listener_delete(l);
		return NULL;
	}

	l->address = dragonnet_addr2str((struct sockaddr *) &bound_addr, bound_addrlen);
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

		struct timeval *tv = NULL;

		fd_set set;
		FD_ZERO(&set);
		FD_SET(l->sock, &set);
		int nfds = l->sock;

#ifdef _WIN32
		// on windows, we can't listen on the pipe, set timeout instead
		tv = &(struct timeval) {1, 0};
#else // _WIN32
		FD_SET(l->intr[0], &set);

		if (nfds < l->intr[0])
			nfds = l->intr[0];
#endif // _WIN32

		if (select(nfds + 1, &set, NULL, NULL, tv) < 0) {
			dragonnet_perror("select");
			continue;
		}

		if (!FD_ISSET(l->sock, &set))
			continue;

		int clt_sock = accept(l->sock, (struct sockaddr *) &clt_addr, &clt_addrlen);
		if (clt_sock < 0) {
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
#ifndef _WIN32
	if (pipe(l->intr) < 0) {
		perror("pipe");
		abort();
	}
#endif // _WIN32
	pthread_create(&l->accept_thread, NULL, &listener_main, l);
}

void dragonnet_listener_close(DragonnetListener *l)
{
	l->active = false;

#ifndef _WIN32
	close(l->intr[1]);
#endif // _WIN32
	pthread_join(l->accept_thread, NULL);
#ifndef _WIN32
	close(l->intr[0]);
#endif // _WIN32
}

void dragonnet_listener_delete(DragonnetListener *l)
{
	free(l->on_recv_type);
	free(l->address);
	free(l);
}
