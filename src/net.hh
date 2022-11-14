// SPDX-License-Identifier: Apache-2.0 OR MIT
#ifndef RSGAME_NET
#define RSGAME_NET
#ifndef WIN32
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#define net_read read
#define net_write write
#define net_close close
#define net_setsockopt setsockopt
#define net_startup() 0
#define net_perror perror
#else
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#undef near
#undef far
#define net_close closesocket
#define net_read(a,b,c) recv((a), (char*)(void*)(b), (c), 0)
#define net_write(a,b,c) send((a), (char*)(void*)(b), (c), 0)
#define net_setsockopt(a,b,c,d,e) setsockopt((a), (b), (c), (char*)(void*)(d), (e))
inline int net_startup() {
	WSADATA wsadata;
	return WSAStartup(MAKEWORD(2, 2), &wsadata) ? -1 : 0;
}
#include <stdio.h>
inline void net_perror(const char *s) {
	char buf[256];
	int error = WSAGetLastError();
	if (FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, error, 0, buf, 256, 0) == 0)
		sprintf(buf, "WSA Error %d", error);
	if (s && *s)
		fprintf(stderr, "%s: %s\n", s, buf);
	else
		fprintf(stderr, "%s\n", buf);
}
#endif
#endif
