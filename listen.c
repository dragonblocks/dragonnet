#include <assert.h>
#include <dragonnet/listen.h>
#include <dragonnet/recv.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

// ----
// Peer
// ----

static bool dragonnet_peer_init_accepted(DragonnetPeer *p, int sock,
		struct sockaddr_in6 addr, DragonnetListener *l)
{
	pthread_mutex_init(&p->mtx, NULL);

	p->sock = sock;
	p->laddr = l->laddr;
	p->raddr = dragonnet_addr_parse_sock(addr);
	p->on_disconnect = l->on_disconnect;
	p->on_recv = l->on_recv;
	p->on_recv_type = l->on_recv_type;

	return true;
}

static DragonnetPeer *dragonnet_peer_accept(int sock, struct sockaddr_in6 addr,
		DragonnetListener *l)
{
	DragonnetPeer *p = malloc(sizeof *p);
	if (!dragonnet_peer_init_accepted(p, sock, addr, l)) {
		pthread_mutex_destroy(&p->mtx);
		free(p);
		return NULL;
	}

	return p;
}

// --------
// Listener
// --------

DragonnetListener *dragonnet_listener_new(char *addr)
{
	DragonnetListener *l = malloc(sizeof *l);

	l->active = true;
	l->sock = socket(AF_INET6, SOCK_STREAM, 0);
	l->on_connect = NULL;
	l->on_disconnect = NULL;
	l->on_recv = NULL;
	l->on_recv_type = calloc(sizeof *l->on_recv_type, dragonnet_num_types);

	int so_reuseaddr = 1;
	if (setsockopt(l->sock, SOL_SOCKET, SO_REUSEADDR, &so_reuseaddr,
			sizeof so_reuseaddr) < 0) {
		perror("setsockopt");
		dragonnet_listener_delete(l);
		return NULL;
	}

	l->laddr = dragonnet_addr_parse_str(addr);
	struct sockaddr_in6 ai_addr = dragonnet_addr_sock(l->laddr);

	if (bind(l->sock, (const struct sockaddr *) &ai_addr, sizeof ai_addr) < 0) {
		perror("bind");
		dragonnet_listener_delete(l);
		return NULL;
	}

	if (listen(l->sock, 10) < 0) {
		perror("listen");
		dragonnet_listener_delete(l);
		return NULL;
	}

	return l;
}

static void *listener_main(void *g_listener)
{
	DragonnetListener *l = (DragonnetListener *) g_listener;

	while (l->active) {
		struct sockaddr_in6 clt_addr;
		socklen_t clt_addrlen = sizeof clt_addr;

		int clt_sock = accept(l->sock, (struct sockaddr *) &clt_addr, &clt_addrlen);
		if (clt_sock < 0) {
			if (errno != EINTR)
				perror("accept");
			continue;
		}

		DragonnetPeer *p = dragonnet_peer_accept(clt_sock, clt_addr, l);
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
	free(l);
}
