#include <errno.h>
#include <stdio.h>
#include "error.h"
#include "sock.h"

void dragonnet_perror(const char *str)
{
#ifdef _WIN32
	wchar_t *msg = NULL;
	FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, WSAGetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR) &msg, 0, NULL);
	fprintf(stderr, "%s: %S\n", str, msg);
	LocalFree(msg);
#else // _WIN32
	perror(str);
#endif // _WIN32
}

bool dragonnet_isconnerr()
{
#ifdef _WIN32
	int err = WSAGetLastError();
	return err == WSAECONNRESET || err == WSAETIMEDOUT || err == WSAEDISCON;
#else // _WIN32
	return errno == ECONNRESET || errno == EPIPE || errno == ETIMEDOUT;
#endif // _WIN32
}


bool dragonnet_isintrerr()
{
#ifdef _WIN32
	return WSAGetLastError() == WSAEINTR; 
#else // _WIN32
	return errno == EINTR;
#endif // _WIN32
}
