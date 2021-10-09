#include <assert.h>
#include <dragonnet/listen.h>
#include <dragonnet/recv.h>
#include <netdb.h>
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
	pthread_rwlock_init(&p->mu, NULL);
	pthread_rwlock_wrlock(&p->mu);

	pthread_rwlock_rdlock(&l->mu);
	p->sock = sock;
	p->laddr = l->laddr;
	p->raddr = dragonnet_addr_parse_sock(addr);
	p->on_recv_type = l->on_recv_type;
	pthread_rwlock_unlock(&l->mu);

	pthread_rwlock_unlock(&p->mu);
	return true;
}

static DragonnetPeer *dragonnet_peer_accept(int sock, struct sockaddr_in6 addr,
		DragonnetListener *l)
{
	DragonnetPeer *p = malloc(sizeof *p);
	if (!dragonnet_peer_init_accepted(p, sock, addr, l)) {
		dragonnet_peer_delete(p);
		return NULL;
	}

	return p;
}

// --------
// Listener
// --------

DragonnetListener *dragonnet_listener_new(char *addr,
		void (*on_connect)(DragonnetPeer *p))
{
	DragonnetListener *l = malloc(sizeof *l);
	pthread_rwlock_init(&l->mu, NULL);
	pthread_rwlock_wrlock(&l->mu);

	l->sock = socket(AF_INET6, SOCK_STREAM, 0);
	l->on_connect = on_connect;
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

	pthread_rwlock_unlock(&l->mu);
	return l;
}

void dragonnet_listener_set_recv_hook(DragonnetListener *l, u16 type_id,
		void (*on_recv)(struct dragonnet_peer *, void *))
{
	pthread_rwlock_rdlock(&l->mu);
	DragonnetListenerState state = l->state;
	pthread_rwlock_unlock(&l->mu);

	if (state >= DRAGONNET_LISTENER_ACTIVE)
		return;

	pthread_rwlock_wrlock(&l->mu);
	l->on_recv_type[type_id] = on_recv;
	pthread_rwlock_unlock(&l->mu);
}

static void *listener_main(void *g_listener)
{
	DragonnetListener *l = (DragonnetListener *) g_listener;

	pthread_rwlock_wrlock(&l->mu);
	assert(l->state == DRAGONNET_LISTENER_CREATED);
	l->state++;
	pthread_rwlock_unlock(&l->mu);

	while (l->state == DRAGONNET_LISTENER_ACTIVE) {
		struct sockaddr_in6 clt_addr;
		socklen_t clt_addrlen = sizeof clt_addr;

		pthread_rwlock_rdlock(&l->mu);
		int sock = l->sock;
		pthread_rwlock_unlock(&l->mu);

		int clt_sock = accept(sock, (struct sockaddr *) &clt_addr, &clt_addrlen);
		if (clt_sock < 0) {
			perror("accept");
			continue;
		}

		DragonnetPeer *p = dragonnet_peer_accept(clt_sock, clt_addr, l);
		if (p == NULL)
			continue;

		dragonnet_peer_run(p);

		pthread_rwlock_rdlock(&l->mu);
		void (*on_connect)(DragonnetPeer *) = l->on_connect;
		pthread_rwlock_unlock(&l->mu);

		if (on_connect != NULL)
			on_connect(p);
	}

	return NULL;
}

void dragonnet_listener_run(DragonnetListener *l)
{
	pthread_create(&l->accept_thread, NULL, &listener_main, l);
}

void dragonnet_listener_close(DragonnetListener *l)
{
	pthread_rwlock_wrlock(&l->mu);

	pthread_t accept_thread = l->accept_thread;
	assert(l->state == DRAGONNET_LISTENER_ACTIVE);
	close(l->sock);
	l->sock = -1;
	l->state++;

	pthread_rwlock_unlock(&l->mu);

	pthread_cancel(accept_thread);
	pthread_join(accept_thread, NULL);
}

void dragonnet_listener_delete(DragonnetListener *l)
{
	pthread_rwlock_destroy(&l->mu);
	free(l);
}
