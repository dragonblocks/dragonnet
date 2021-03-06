#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "error.h"
#include "send.h"
#include "sock.h"

bool dragonnet_send_raw(DragonnetPeer *p, bool submit, const void *buf, size_t n)
{
	ssize_t len = send(p->sock, buf, n, MSG_NOSIGNAL | (submit ? 0 : MSG_MORE));

	if (len < 0) {
		if (dragonnet_isconnerr()) {
			shutdown(p->sock, SHUT_RDWR);
			pthread_mutex_unlock(&p->mtx);
			return false;
		}

		dragonnet_perror("send");
		abort();
	}

	if (submit)
		pthread_mutex_unlock(&p->mtx);

	return true;
}
