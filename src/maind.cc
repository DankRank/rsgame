// SPDX-License-Identifier: Apache-2.0 OR MIT
#include "common.hh"
#include "level.hh"
#include "tile.hh"
#include "net.hh"
#include <stdio.h>
#include <time.h>
namespace rsgame {
#ifndef WIN32
timespec time0;
void time_init() {
	// checking for errors during init only seems to be sufficient (SDL does it like that)
	if (clock_gettime(CLOCK_MONOTONIC, &time0) == -1) {
		perror("clock_gettime");
		exit(1);
	}
}
uint64_t time_ticks() {
	timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	return (uint64_t)((int64_t)(now.tv_sec - time0.tv_sec)*1000 + (int64_t)(now.tv_nsec - time0.tv_nsec)/1000000);
}
#else
LARGE_INTEGER time0, timef;
void time_init() {
	QueryPerformanceFrequency(&timef);
	QueryPerformanceCounter(&time0);
}
uint64_t time_ticks() {
	LARGE_INTEGER now;
	QueryPerformanceCounter(&now);
	return (uint64_t)((now.QuadPart - time0.QuadPart)*1000 / timef.QuadPart);
}
#endif
int next_eid = 0;
struct Connection {
	Connection(int sock) :sock(sock), eid(next_eid++) {}
	int sock;
	bool dead = false;
	bool logged_in = false;
	int x = 0, y = 0, z = 0;
	short yaw = 0, pitch = 0;
	int eid = 0;
	std::vector<uint8_t> writebuf;
	uint8_t readbuf[2+65536];
	int readlen;
private:
	int readpos = 0;
public:
	bool read() {
		if (dead)
			return false;
		for (;;) {
			if (readpos < 2) {
				int r = net_read(sock, readbuf + readpos, 2 - readpos);
				if (r == -1) {
					if (net_again()) {
						return false;
					} else {
						net_perror("read");
						dead = true;
						return false;
					}
				} else if (r == 0) {
					fprintf(stderr, "read: returned 0\n");
					dead = true;
					return false;
				} else {
					readpos += r;
				}
			} else {
				int plen = readbuf[0] << 8 | readbuf[1];
				int r = net_read(sock, readbuf + readpos, 2 + plen - readpos);
				if (r == -1) {
					if (net_again()) {
						return false;
					} else {
						net_perror("read");
						dead = true;
						return false;
					}
				} else if (r == 0) {
					fprintf(stderr, "read: returned 0\n");
					dead = true;
					return false;
				} else {
					readpos += r;
					if (readpos == 2 + plen) {
						readlen = 2 + plen;
						readpos = 0;
						return true;
					}
				}
			}
		}
	}
	void write(const uint8_t *buf, int len) {
		if (dead)
			return;
		for (;;) {
			if (!writebuf.size()) {
				if (len) {
					int r = net_write(sock, buf, len);
					if (r == -1) {
						if (net_again()) {
							writebuf.resize(len);
							memcpy(writebuf.data(), buf, len);
						} else {
							net_perror("write");
							dead = true;
						}
						return;
					} else {
						assert(r);
						if (r == len)
							return;
						buf += r;
						len -= r;
					}
				} else {
					return;
				}
			} else {
				int r = net_write(sock, writebuf.data(), writebuf.size());
				if (r == -1) {
					if (net_again()) {
						writebuf.resize(writebuf.size() + len);
						memcpy(writebuf.data() + writebuf.size(), buf, len);
					} else {
						net_perror("write");
						dead = true;
					}
					return;
				} else {
					assert(r);
					if (r != (int)writebuf.size()) {
						memmove(writebuf.data(), writebuf.data()+r, writebuf.size() - r);
						writebuf.resize(writebuf.size() - r);
					} else {
						writebuf.clear();
					}
				}
			}
		}
	}
	void send(const PacketWriter &pw) {
		pw.buf[0] = (pw.pos-2)>>16;
		pw.buf[1] = (pw.pos-2);
		write(pw.buf, pw.pos);
	}
};
std::vector<Connection*> conns;
/* Each poll cycle looks like so:
 * - poll
 * - read and process packets
 * - flush write buffers
 * - accept new connections
 * - close connections marked as dead
 */
#ifndef WIN32
struct Poll {
	struct pollfd pollfds[256];
	int listenfd;
	bool needs_accept = false;
	bool can_accept() {
		return 1+conns.size() < 256;
	}
	void poll() {
		pollfds[0].fd = listenfd;
		pollfds[0].events = POLLIN;
		for (size_t i = 0; i < conns.size(); i++) {
			pollfds[i+1].fd = conns[i]->sock;
			pollfds[i+1].events = conns[i]->writebuf.size() ? POLLIN | POLLOUT : POLLIN;
		}
		::poll(pollfds, conns.size() + 1, 1);
		needs_accept = pollfds[0].revents & POLLIN;
		next_index = 1;
	}
	size_t next_index;
	Connection *next_to_read() {
		while (next_index++ < conns.size() + 1) {
			if (pollfds[next_index-1].revents & POLLIN) {
				return conns[next_index-2];
			}
		}
		return nullptr;
	}
	void process_writes() {
		for (size_t i = 1; i < conns.size() + 1; i++) {
			if (pollfds[i].revents & POLLOUT) {
				conns[i-1]->write(0, 0);
			}
		}
	}
	void add_conn(Connection *conn) {
		(void)conn;
	}
	void del_conn(Connection *conn) {
		(void)conn;
	}
};
#else
struct Poll {
	std::unordered_map<int, Connection*> sock_to_conn;
	fd_set readfds, writefds;
	int listenfd;
	bool needs_accept = false;
	bool can_accept() {
		return 1+conns.size() < FD_SETSIZE;
	}
	void poll() {
		readfds.fd_count = 0;
		writefds.fd_count = 0;
		readfds.fd_array[readfds.fd_count++] = listenfd;
		for (size_t i = 0; i < conns.size(); i++) {
			readfds.fd_array[readfds.fd_count++] = conns[i]->sock;
			if (conns[i]->writebuf.size())
				writefds.fd_array[writefds.fd_count++] = conns[i]->sock;
		}
		timeval tv;
		tv.tv_sec = 2;
		tv.tv_usec = 0;
		select(0, &readfds, &writefds, 0, &tv);
		needs_accept = false;
		next_index = 0;
	}
	size_t next_index;
	Connection *next_to_read() {
		while (next_index++ < readfds.fd_count) {
			if (readfds.fd_array[next_index-1] == (SOCKET)listenfd) {
				needs_accept = true;
			} else {
				auto it = sock_to_conn.find(readfds.fd_array[next_index-1]);
				assert(it != sock_to_conn.end());
				if (it != sock_to_conn.end())
					return it->second;
			}
		}
		return nullptr;
	}
	void process_writes() {
		for (size_t i = 0; i < writefds.fd_count; i++) {
			auto it = sock_to_conn.find(writefds.fd_array[i]);
			assert(it != sock_to_conn.end());
			if (it != sock_to_conn.end())
				it->second->write(0, 0);
		}
	}
	void add_conn(Connection *conn) {
		sock_to_conn.emplace(conn->sock, conn);
	}
	void del_conn(Connection *conn) {
		sock_to_conn.erase(conn->sock);
	}
};
#endif
Poll poller;
void accept_connections() {
	if (poller.needs_accept) {
		for (;;) {
			int sock = accept(poller.listenfd, NULL, NULL);
			if (sock == -1) {
				if (net_again()) {
					break;
				} else {
					net_perror("accept");
					exit(1);
				}
			}
			if (net_nonblock(sock) == -1) {
				net_perror("net_nonblock");
				net_close(sock);
				continue;
			}
			if (!poller.can_accept()) {
				net_write(sock, "\x00\x0C\x02Server Busy", 14);
				net_close(sock);
			} else {
				Connection *conn = new Connection(sock);
				poller.add_conn(conn);
				conns.push_back(conn);
			}
		}
	}
}
void close_dead_connections() {
	for (size_t i = 0; i < conns.size(); i++) {
		Connection *conn = conns[i];
		if (conn->dead) {
			if (conn->logged_in) {
				uint8_t pbuf[2+5];
				PacketWriter pw(pbuf);
				pw.write8(S_EntityLeave);
				pw.write32(conn->eid);
				for (Connection *conn : conns)
					if (conn->logged_in)
						conn->send(pw);
			}
			poller.del_conn(conn);
			net_close(conn->sock);
			delete conn;
			conns.erase(conns.begin()+i);
			i--;
		}
	}
}
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
	time_init();
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
	if (net_nonblock(listenfd) == -1) {
		net_perror("net_nonblock");
		return 1;
	}
	poller.listenfd = listenfd;
	fprintf(stderr, "Listening...\n");

	Level level;
	RenderLevel rl;
	level.rl = &rl;

	uint64_t unprocessed_ms = 0;
	uint32_t last_frame = time_ticks();
	for (;;) {
		poller.poll();
		Connection *conn;
		while ((conn = poller.next_to_read())) {
			while (conn->read()) {
				int plen = conn->readlen - 2;
				PacketReader pr(conn->readbuf + 2);
				if (!conn->logged_in) {
					uint8_t pbuf[2+21];
					if (plen != 5 || pr.read8() != C_ClientIntroduction || pr.read32() != RSGAME_NETPROTO) {
						conn->send(PacketWriter(pbuf)
							.write8(B_Disconnect)
							.write_str("Bad proto", 9));
						conn->dead = true;
					leave:
						break;
					}
					conn->send(PacketWriter(pbuf)
						.write8(S_ServerIntroduction)
						.write32(RSGAME_NETPROTO)
						.write32(conn->eid)
						.write32(level.xsize)
						.write32(level.zsize)
						.write32(level.zbits));
					conn->write(level.buf.data(), level.buf.size());
					{
						PacketWriter pw(pbuf);
						pw.write8(S_EntityEnter);
						pw.write32(conn->eid);
						for (Connection *conn : conns)
							if (conn->logged_in)
								conn->send(pw);
					}
					conn->logged_in = true;
					for (Connection *conn2 : conns)
						if (conn2->logged_in) {
							conn->send(PacketWriter(pbuf)
								.write8(S_EntityEnter)
								.write32(conn2->eid));
						}
				} else {
					if (plen == 0)
						continue;
					switch (pr.read8()) {
					case B_Disconnect: {
						fprintf(stderr, "Disconnected: %.*s\n", plen-1, &conn->readbuf[3]);
						conn->dead = true;
						goto leave;
					}
					case C_ChangePosition: {
						if (plen < 17)
							break;
						conn->x = pr.read32();
						conn->y = pr.read32();
						conn->z = pr.read32();
						conn->yaw = pr.read16();
						conn->pitch = pr.read16();
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
						}
						break;
					}
					}
				}
			}
		}
		uint64_t current_frame = time_ticks();
		unprocessed_ms += current_frame - last_frame;
		last_frame = current_frame;
		if (unprocessed_ms > 50) {
			while (unprocessed_ms > 50) {
				level.on_tick();
				unprocessed_ms -= 50;
			}
			uint8_t pbuf[65536];
			for (auto it = block_updates.begin(); it != block_updates.end(); ) {
				PacketWriter pw(pbuf);
				pw.write8(S_BlockUpdates);
				while (pw.pos < 65536 - 6 && it != block_updates.end()) {
					glm::ivec3 pos = *it++;
					pw.write32(level.pos_to_index(pos.x, pos.y, pos.z));
					pw.write8(level.get_tile_id(pos.x, pos.y, pos.z));
					pw.write8(level.get_tile_meta(pos.x, pos.y, pos.z));
				}
				for (Connection *conn : conns)
					if (conn->logged_in)
						conn->send(pw);
			}
			block_updates.clear();
			for (auto it = conns.begin(); it != conns.end(); ) {
				PacketWriter pw(pbuf);
				pw.write8(S_EntityUpdates);
				while (pw.pos < 65536 - 20 && it != conns.end()) {
					Connection *conn = *it++;
					pw.write32(conn->eid);
					pw.write32(conn->x);
					pw.write32(conn->y);
					pw.write32(conn->z);
					pw.write16(conn->yaw);
					pw.write16(conn->pitch);
				}
				for (Connection *conn : conns)
					if (conn->logged_in)
						conn->send(pw);
			}
		}
		poller.process_writes();
		accept_connections();
		close_dead_connections();
	}
	return 0;
}
}
extern "C" int main(int argc, char** argv)
{
	return rsgame::main(argc, argv);
}
