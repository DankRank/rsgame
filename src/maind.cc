// SPDX-License-Identifier: Apache-2.0 OR MIT
#include "common.hh"
#include "level.hh"
#include "tile.hh"
#include "net.hh"
#include <stdio.h>
namespace rsgame {
std::vector<glm::ivec3> block_updates;
void server_set_dirty(int x, int y, int z)
{
	block_updates.emplace_back(x, y, z);
}
struct RenderLevel {
	void set_dirty(int x, int y, int z) {
		server_set_dirty(x, y, z);
	}
};
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
	RenderLevel rl;
	level.rl = &rl;
	for (;;) {
		int sock = accept(listenfd, NULL, NULL);
		{
			uint8_t buf[17];
			net_read(sock, buf, 2+5);
			PacketReader pr(buf);
			if (pr.read16() != 5 || pr.read8() != C_ClientIntroduction || pr.read32() != RSGAME_NETPROTO) {
				PacketWriter(buf)
					.write8(B_Disconnect)
					.write_str("Bad proto", 9)
					.send(sock);
				net_close(sock);
				continue;
			}
			PacketWriter(buf)
				.write8(S_ServerIntroduction)
				.write32(RSGAME_NETPROTO)
				.write32(level.xsize)
				.write32(level.zsize)
				.write32(level.zbits)
				.send(sock);
			net_write(sock, level.buf.data(), level.buf.size());
		}
		auto send_updates = [&]() {
			uint8_t pbuf[65536];
			auto it = block_updates.begin();
			while (it != block_updates.end()) {
				PacketWriter pw(pbuf);
				pw.write8(S_BlockUpdates);
				while (pw.pos < 65536 - 6 && it != block_updates.end()) {
					glm::ivec3 pos = *it++;
					pw.write32(level.pos_to_index(pos.x, pos.y, pos.z));
					pw.write8(level.get_tile_id(pos.x, pos.y, pos.z));
					pw.write8(level.get_tile_meta(pos.x, pos.y, pos.z));
				}
				pw.send(sock);
			}
			block_updates.clear();
		};
		for (;;) {
			uint16_t plen;
			int r = net_read(sock, &plen, 2);
			if (r == 0) {
				break;
			}
			plen = ntohs(plen);
			if (!plen)
				continue;
			uint8_t pbuf[65536];
			net_read(sock, pbuf, plen);
			PacketReader pr(pbuf);
			switch (pr.read8()) {
			case B_Disconnect: {
				fprintf(stderr, "Disconnected: %.*s\n", plen-1, &pbuf[1]);
				goto leave;
			}
			case C_ChangePosition: {
				if (plen < 17)
					break;
#if 0
				int x = pr.read32();
				int y = pr.read32();
				int z = pr.read32();
				short yaw = pr.read16();
				short pitch = pr.read16();
				printf("%f %f %f %f %f\n", x/32.f, y/32.f, z/32.f, yaw*2.f*glm::pi<float>()/65535, pitch*2.f*glm::pi<float>()/65535);
#endif
				level.on_tick();
				send_updates();
				break;
			}
			case C_ChangeBlock: {
				if (plen < 8)
					break;
				uint32_t index = pr.read32();
				uint8_t old_id = pr.read8();
				uint8_t old_data = pr.read8();
				uint8_t new_id = pr.read8();
				uint8_t new_data = pr.read8();

				glm::ivec3 pos = level.index_to_pos(index);
				if (old_id == level.get_tile_id(pos.x, pos.y, pos.z) &&
						old_data == level.get_tile_meta(pos.x, pos.y, pos.z)) {
					level.set_tile(pos.x, pos.y, pos.z, new_id, new_data);
					if (new_id != 0)
						level.on_block_add(pos.x, pos.y, pos.z, new_id);
					else
						level.on_block_remove(pos.x, pos.y, pos.z, old_id);
					send_updates();
				}
				break;
			}
			}
		}
	leave:
		net_close(sock);
	}
	return 0;
}
}
extern "C" int main(int argc, char** argv)
{
	return rsgame::main(argc, argv);
}
