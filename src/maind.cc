// SPDX-License-Identifier: Apache-2.0 OR MIT
#include "common.hh"
#include "level.hh"
#include "tile.hh"
#include "net.hh"
#include <stdio.h>
namespace rsgame {
int main(int argc, char** argv)
{
	if (net_startup() == -1) {
		fprintf(stderr, "WSAStartup failed\n");
		return 1;
	}
	tiles::init();

	const char *listen_host = "127.0.0.1";
	const char *listen_port = "21814";
	if (argc > 1)
		listen_host = argv[1];
	if (argc > 2)
		listen_port = argv[2];

	struct addrinfo hints, *res;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV;
	int err = getaddrinfo(listen_host, listen_port, &hints, &res);
	if (err) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
		return 1;
	}
	int listenfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (listenfd == -1) {
		net_perror("socket");
		return 1;
	}
	int opt = 1;
#ifndef WIN32
	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (void *)&opt, sizeof(int)) == -1) {
		net_perror("setsockopt(SO_REUSEADDR)");
		return 1;
	}
#endif
	if (res->ai_family == AF_INET6) {
		if (net_setsockopt(listenfd, IPPROTO_IPV6, IPV6_V6ONLY, (void *)&opt, sizeof(int)) == -1) {
			net_perror("setsockopt(IPV6_V6ONLY)");
			return 1;
		}
	}
	if (bind(listenfd, res->ai_addr, res->ai_addrlen) == -1) {
		net_perror("bind");
		return 1;
	}
	freeaddrinfo(res);
	if (listen(listenfd, 10) == -1) {
		net_perror("listen");
		return 1;
	}

	Level level;
	for (;;) {
		int sock = accept(listenfd, NULL, NULL);
		uint32_t x;
		x = htonl(level.xsize);
		net_write(sock, &x, 4);
		x = htonl(level.zsize);
		net_write(sock, &x, 4);
		x = htonl(level.zbits);
		net_write(sock, &x, 4);
		net_write(sock, level.buf.data(), level.buf.size());
		net_close(sock);
	}
	return 0;
}
}
extern "C" int main(int argc, char** argv)
{
	return rsgame::main(argc, argv);
}
