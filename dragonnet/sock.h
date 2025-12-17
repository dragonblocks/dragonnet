#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <ws2def.h>
#include <windows.h>
#include <io.h>
#define SHUT_RDWR SD_BOTH
#define MSG_NOSIGNAL 0
#define MSG_MORE 0
#else
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#ifndef MSG_MORE
#define MSG_MORE 0
#endif
#endif
