#include <arpa/inet.h>
#include <assert.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include "addr.h"
#include "listen.h"

// ----
// Peer
// ----

static DragonnetPeer *dragonnet_peer_accept(int sock, struct sockaddr_in6 addr,
		DragonnetListener *l)
{
	DragonnetPeer *p = malloc(sizeof *p);
	p->mu = malloc(sizeof *p->mu);
	pthread_rwlock_init(p->mu, NULL);
	pthread_rwlock_wrlock(p->mu);

	p->sock = sock;
	p->laddr = l->laddr;

	char ip_addr[INET6_ADDRSTRLEN] = {0};
	inet_ntop(AF_INET6, &addr.sin6_addr, ip_addr, INET6_ADDRSTRLEN);

	char port[6] = {0};
	sprintf(port, "%d", ntohs(addr.sin6_port));

	int err = getaddrinfo(ip_addr, port, NULL, &p->raddr);
	if (err != 0) {
		fprintf(stderr, "invalid network address %s:%s\n", ip_addr, port);
		dragonnet_peer_delete(p);
		p = NULL;
	}

	if (p != NULL)
		pthread_rwlock_unlock(p->mu);

	return p;
}

// --------
// Listener
// --------

DragonnetListener *dragonnet_listener_new(char *addr, void (*on_connect)(DragonnetPeer *p))
{
	DragonnetListener *l = malloc(sizeof *l);
	l->mu = malloc(sizeof *l->mu);
	pthread_rwlock_init(l->mu, NULL);
	pthread_rwlock_wrlock(l->mu);

	l->sock = socket(AF_INET6, SOCK_STREAM, 0);
	l->on_connect = on_connect;

	int flag = 1;
	if (setsockopt(l->sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof flag) < 0) {
		perror("setsockopt");
		dragonnet_listener_delete(l);
		return NULL;
	}

	DragonnetAddr net_addr = dragonnet_addr_parse(addr);
	int err = getaddrinfo(net_addr.ip, net_addr.port, NULL, &l->laddr);
	if (err != 0) {
		fprintf(stderr, "invalid network address %s\n", addr);
		dragonnet_listener_delete(l);
		return NULL;
	}

	if (bind(l->sock, l->laddr->ai_addr, l->laddr->ai_addrlen) < 0) {
		perror("bind");
		dragonnet_listener_delete(l);
		return NULL;
	}

	if (listen(l->sock, 10) < 0) {
		perror("listen");
		dragonnet_listener_delete(l);
		return NULL;
	}

	pthread_rwlock_unlock(l->mu);
	return l;
}

void dragonnet_listener_run(DragonnetListener *l)
{
	pthread_rwlock_wrlock(l->mu);

	assert(l->state == DRAGONNET_LISTENER_CREATED);
	l->state++;

	pthread_rwlock_unlock(l->mu);

	while (l->state == DRAGONNET_LISTENER_ACTIVE) {
		struct sockaddr_in6 clt_addr;
		socklen_t clt_addrlen = sizeof clt_addr;

		int clt_sock = accept(l->sock, (struct sockaddr *) &clt_addr, &clt_addrlen);
		if (clt_sock < 0) {
			perror("accept");
			continue;
		}

		DragonnetPeer *p = dragonnet_peer_accept(clt_sock, clt_addr, l);
		if (p == NULL)
			continue;

		if (l->on_connect != NULL)
			l->on_connect(p);
	}
}

void dragonnet_listener_close(DragonnetListener *l)
{
	pthread_rwlock_wrlock(l->mu);

	assert(l->state == DRAGONNET_LISTENER_ACTIVE);
	close(l->sock);
	l->state++;

	pthread_rwlock_unlock(l->mu);
}

void dragonnet_listener_delete(DragonnetListener *l)
{
	pthread_rwlock_wrlock(l->mu);

	if (l->laddr != NULL)
		freeaddrinfo(l->laddr);

	pthread_rwlock_unlock(l->mu);
	pthread_rwlock_destroy(l->mu);
	free(l);
}
