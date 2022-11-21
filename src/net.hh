// SPDX-License-Identifier: Apache-2.0 OR MIT
#ifndef RSGAME_NET
#define RSGAME_NET
// cross-platform code
#ifndef WIN32
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <stdio.h>
namespace rsgame {
	inline int net_close(int fd) {
		return close(fd);
	}
	inline int net_read(int fd, void *buf, size_t len) {
		return read(fd, buf, len);
	}
	inline int net_write(int fd, const void *buf, size_t len) {
		return write(fd, buf, len);
	}
	inline int net_setsockopt(int fd, int level, int optname, const void *optval, int optlen) {
		return setsockopt(fd, level, optname, optval, optlen);
	}
	inline int net_startup() {
		return 0;
	}
	inline int net_nonblock(int sock) {
		int flags = fcntl(sock, F_GETFL);
		if (flags == -1)
			return -1;
		return fcntl(sock, F_SETFL, flags | O_NONBLOCK);
	}
	inline int net_block(int sock) {
		int flags = fcntl(sock, F_GETFL);
		if (flags == -1)
			return -1;
		return fcntl(sock, F_SETFL, flags & ~O_NONBLOCK);
	}
	inline bool net_again() {
		return errno == EAGAIN;
	}
	inline void net_perror(const char *s) {
		perror(s);
	}
}
#else
/* FD_SETSIZE can be raised on Windows according to KB111856.
 * Actually pretty much every operating system allows redefining it. Linux
 * headers are the most notable exception, though the kernel itself has no
 * limitation on the set size. The only OS that I know of that has a hardcoded
 * FD_SETSIZE in the kernel is 4.4BSD. */
#define FD_SETSIZE 256
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#undef near
#undef far
#undef min
#undef max
namespace rsgame {
	inline int net_close(int fd) {
		return closesocket(fd);
	}
	inline int net_read(int fd, void *buf, size_t len) {
		return recv(fd, (char *)buf, len, 0);
	}
	inline int net_write(int fd, const void *buf, size_t len) {
		return send(fd, (const char *)buf, len, 0);
	}
	inline int net_setsockopt(int fd, int level, int optname, const void *optval, int optlen) {
		return setsockopt(fd, level, optname, (const char *)optval, optlen);
	}
	inline int net_startup() {
		WSADATA wsadata;
		return WSAStartup(MAKEWORD(2, 2), &wsadata) ? -1 : 0;
	}
	inline int net_nonblock(int sock) {
		u_long one = 1;
		return ioctlsocket(sock, FIONBIO, &one);
	}
	inline int net_block(int sock) {
		u_long one = 0;
		return ioctlsocket(sock, FIONBIO, &one);
	}
	inline bool net_again() {
		return WSAGetLastError() == WSAEWOULDBLOCK;
	}
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
}
#endif
// rsgame specific helpers
#define RSGAME_NETPROTO 0x20221122
namespace rsgame {
	enum {
		C_ClientIntroduction = 0,
		S_ServerIntroduction,
		B_Disconnect,
		C_ChangePosition = 6,
		C_ChangeBlock,
		S_BlockUpdates,
		S_EntityEnter,
		S_EntityLeave,
		S_EntityUpdates,
	};
	struct PacketReader {
		uint8_t *buf;
		PacketReader(uint8_t *buf) :buf(buf) {}
		uint8_t read8() {
			uint8_t x = buf[0];
			buf += 1;
			return x;
		}
		uint16_t read16() {
			uint16_t x = buf[0]<<8 | buf[1];
			buf += 2;
			return x;
		}
		uint32_t read32() {
			uint32_t x = buf[0]<<24 | buf[1]<<16 | buf[2]<<8 | buf[3];
			buf += 4;
			return x;
		}
	};
	struct PacketWriter {
		uint8_t *buf;
		int pos = 2;
		PacketWriter(uint8_t *buf) :buf(buf) {}
		PacketWriter &write8(uint8_t x) {
			buf[pos++] = x;
			return *this;
		}
		PacketWriter &write16(uint16_t x) {
			buf[pos++] = x>>8;
			buf[pos++] = x;
			return *this;
		}
		PacketWriter &write32(uint32_t x) {
			buf[pos++] = x>>24;
			buf[pos++] = x>>16;
			buf[pos++] = x>>8;
			buf[pos++] = x;
			return *this;
		}
		PacketWriter &write_str(const char *p, int len) {
			memcpy(buf+pos, p, len);
			pos += len;
			return *this;
		}
		void send(int sock) {
			buf[0] = (pos-2)>>16;
			buf[1] = (pos-2);
			if (net_write(sock, buf, pos) != pos) {
				net_perror("net_write");
			}
		}
	};
}
#endif
