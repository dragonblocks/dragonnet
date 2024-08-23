#include <stdio.h>
#include <stdlib.h>
#include "init.h"
#include "sock.h"

void dragonnet_init()
{
#ifdef _WIN32
	fprintf(stderr, "[info] initializing winsock\n");

	WSADATA wsa_data;
	if (WSAStartup(0x202, &wsa_data) != 0) {
		fprintf(stderr, "[error] failed to initialize winsock\n");
		abort();
	}
#endif
}

void dragonnet_deinit()
{
#ifdef _WIN32
	WSACleanup();
#endif
}
