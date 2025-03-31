// SPDX-License-Identifier: Apache-2.0 OR MIT
#include "common.hh"
#include "level.hh"
#include "render.hh"
#include "tile.hh"
#include "raycast.hh"
#include "util.hh"
#include "glutil.hh"
#include <stdio.h>
#ifdef RSGAME_NETCLIENT
#include "net.hh"
#include <zlib.h>
#endif
namespace rsgame {
bool verbose = true;
static bool gles = false;
static bool glcore = false;
const char *shader_prologue = nullptr;
bool vsync = true;
bool fullscreen = false;
SDL_Window* window = nullptr;
bool is_running = true;
mat4 viewproj;
static Frustum viewfrustum;
#if RSGAME_NETCLIENT
int my_eid = -1;
struct Entity {
	float x, y, z, yaw, pitch;
};
std::unordered_map<int, Entity> entities;
int connect_to_host(const char *connect_host, const char *connect_port)
{
	struct addrinfo hints, *res;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	int err = getaddrinfo(connect_host, connect_port, &hints, &res);
	if (err) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
		return 1;
	}
	int sock = -1;
	for (struct addrinfo *ai = res; ai; ai = ai->ai_next) {
		if (ai->ai_addr->sa_family == AF_INET) {
			char host[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, &((struct sockaddr_in*)ai->ai_addr)->sin_addr, host, sizeof(host));
			uint16_t port = ntohs(((struct sockaddr_in*)ai->ai_addr)->sin_port);
			fprintf(stderr, "Connecting to %s:%u\n", host, port);
		} else if (ai->ai_addr->sa_family == AF_INET6) {
			char host[INET6_ADDRSTRLEN];
			inet_ntop(AF_INET6, &((struct sockaddr_in6*)ai->ai_addr)->sin6_addr, host, sizeof(host));
			uint16_t port = ntohs(((struct sockaddr_in6*)ai->ai_addr)->sin6_port);
			fprintf(stderr, "Connecting to [%s]:%u\n", host, port);
		} else {
			fprintf(stderr, "Connecting to ???\n");
		}
		sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if (sock == -1) {
			net_perror("socket");
			continue;
		}
		if (connect(sock, ai->ai_addr, ai->ai_addrlen) == -1) {
			net_perror("connect");
			net_close(sock);
			sock = -1;
			continue;
		}
		break;
	}
	freeaddrinfo(res);
	return sock;
}
#endif
int main(int argc, char** argv)
{
	tiles::init();

#if RSGAME_NETCLIENT
	if (net_startup() == -1) {
		fprintf(stderr, "WSAStartup failed\n");
		return 1;
	}
	const char *connect_host = "127.0.0.1";
	const char *connect_port = "21814";
	int freeargs = 0;
#endif
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--gles")) {
			gles = true;
		} else if (!strcmp(argv[i], "--glcore")) {
			glcore = true;
		} else if (!strcmp(argv[i], "--nosync")) {
			vsync = false;
		} else if (!strcmp(argv[i], "--dump-tiles")) {
			tiles::dump();
			return 0;
		} else {
#if RSGAME_NETCLIENT
			switch (freeargs++) {
				case 0: connect_host = argv[i]; break;
				case 1: connect_port = argv[i]; break;
			}
#endif
		}
	}
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER)) {
		fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
		return 1;
	}
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	if (gles) {
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
		shader_prologue = "#version 300 es\nprecision mediump float;\n";
	} else if (glcore) {
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		shader_prologue = "#version 150 core\n";
	} else {
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, 0);
		shader_prologue = "#version 130\n";
	}
	int width = 854, height = 480;
#ifdef RSGAME_NETCLIENT
	const char *window_title = "rsgame (netclient)";
#else
	const char *window_title = "rsgame";
#endif
	window = SDL_CreateWindow(window_title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height,
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	if (!window) {
		fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
		return 1;
	}
	SDL_GLContext context = SDL_GL_CreateContext(window);
	if (!context) {
		fprintf(stderr, "SDL_GL_CreateContext: %s\n", SDL_GetError());
		return 1;
	}
	SDL_GL_MakeCurrent(window, context);
	gles = !epoxy_is_desktop_gl();
	{
		int glver = epoxy_gl_version();
		int glslver = epoxy_glsl_version();
		const char *suffix = gles ? " ES" : "";
		fprintf(stderr, "OpenGL%s %d.%d / ", suffix, glver/10, glver%10);
		fprintf(stderr, "GLSL%s %d.%02d\n", suffix, glslver/100, glslver%100);
	}

	// Dummy VAO for GL3.0 Core
	GLuint va;
	glGenVertexArrays(1, &va);
	glBindVertexArray(va);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glViewport(0, 0, width, height);
	SDL_GL_SetSwapInterval(0); // disable vsync while loading

	fprintf(stderr, "Loading shaders...\n");
	if (!load_shaders())
		return 1;
	fprintf(stderr, "Loading textures...\n");
	if (!load_textures())
		return 1;

#ifdef RSGAME_NETCLIENT
	fprintf(stderr, "Connecting to server...\n");
	int sock;
	if ((sock = connect_to_host(connect_host, connect_port)) == -1) {
		printf("Couldn't connect\n");
		return 1;
	}
#endif

	fprintf(stderr, "Loading level...\n");
	Level level;
#ifdef RSGAME_NETCLIENT
	{
		uint8_t pbuf[2+21];
		PacketWriter(pbuf)
			.write8(C_ClientIntroduction)
			.write32(RSGAME_NETPROTO)
			.send(sock);
		net_read(sock, pbuf, 2+21);
		PacketReader pr(pbuf);
		uint16_t plen = pr.read16();
		if (!plen) {
			fprintf(stderr, "Protocol error\n");
			return 1;
		}
		uint16_t pid = pr.read8();
		if (pid == B_Disconnect) {
			fprintf(stderr, "Disconnected: %.*s\n", std::min(16, plen-1), &pbuf[3]);
			return 1;
		} else if (pid != S_ServerIntroduction) {
			fprintf(stderr, "Protocol error\n");
			return 1;
		}
		uint32_t proto = pr.read32();
		if (proto != RSGAME_NETPROTO) {
			fprintf(stderr, "Incompatible protocol %X instead of %X\n", proto, RSGAME_NETPROTO);
			return 1;
		}
		if (plen != 21) {
			fprintf(stderr, "Bad packet size\n");
			return 1;
		}
		my_eid = pr.read32();
		uint32_t xsize = pr.read32();
		uint32_t zsize = pr.read32();
		uint32_t zbits = pr.read32();
		level = Level(xsize, zsize, zbits);
		{
			z_stream strm;
			uint8_t zbuf[1024];
			memset(&strm, 0, sizeof(strm));
			inflateInit(&strm);
			strm.next_out = level.buf.data();
			strm.avail_out = level.buf.size();
			int res = Z_OK;
			while (res == Z_OK) {
				int r = net_read(sock, zbuf, 1024);
				if (r <= 0) {
					net_perror("read");
					return 1;
				}
				strm.next_in = zbuf;
				strm.avail_in = sizeof(zbuf);
				fprintf(stderr, "bytes remaining: %d\n", strm.avail_out);
				res = inflate(&strm, Z_NO_FLUSH);
				if (res == Z_OK) {
					if (strm.avail_in != 0) {
						fprintf(stderr, "zlib doesn't work the way I want it to work\n");
						return 1;
					}
				} else if (res != Z_STREAM_END) {
					fprintf(stderr, "zlib failed: %s\n", strm.msg);
					return 1;
				}
			}
			if (strm.avail_out != 0) {
				fprintf(stderr, "zlib stream too short\n");
				return 1;
			}
			inflateEnd(&strm);
		}
	}
#endif

	fprintf(stderr, "Allocating chunks...\n");
	RenderLevel *rl = new RenderLevel(&level);
	for (int i = 0; i < level.xsize>>4; i++)
		for (int j = 0; j < level.zsize>>4; j++)
			rl->on_load_chunk(i, j);
	level.rl = rl;

	fprintf(stderr, "Updating chunks...\n");
	{
		float to_load = (level.xsize>>4) * (level.zsize>>4) * 8;
		Uint64 update_start = SDL_GetPerformanceCounter();
		while (rl->dirty_chunks.size()) {
			rl->update(SDL_GetPerformanceCounter()+SDL_GetPerformanceFrequency()*50/1000);
			float c = rl->dirty_chunks.size()/to_load;
			glClearColor(c, c, c, 1.f);
			glClear(GL_COLOR_BUFFER_BIT);
			SDL_GL_SwapWindow(window);
		}
		Uint64 update_end = SDL_GetPerformanceCounter();
		double update_time = (double)(update_end - update_start) * 1000 / SDL_GetPerformanceFrequency();
		fprintf(stderr, "Updated in %.2f ms\n", update_time);
	}

	SDL_GL_SetSwapInterval((int)vsync);

	float yaw = glm::radians(180.f), pitch = 0.0;
	vec3 pos{0};
	vec3 look{0};

#ifdef RSGAME_NETCLIENT
	int oldx = 0, oldy = 0, oldz = 0;
	short oldyaw = 0, oldpitch = 0;
	init_player();
#endif
	init_raytarget();
	init_hud();

	double avg_frame_time = 0;
	Uint64 unprocessed_ms = 0;
	Uint64 last_frame = SDL_GetTicks64();
	enum {
		KEY_LEFT = 0,
		KEY_RIGHT,
		KEY_UP,
		KEY_DOWN,
		KEY_W,
		KEY_S,
		KEY_A,
		KEY_D,
		KEY_SPACE,
		KEY_SHIFT,
		KEY_CTRL,
		KEY_COUNT,
	};
	std::unordered_map<int, int> key_map {
		{SDL_SCANCODE_LEFT,   KEY_LEFT },
		{SDL_SCANCODE_RIGHT,  KEY_RIGHT},
		{SDL_SCANCODE_UP,     KEY_UP   },
		{SDL_SCANCODE_DOWN,   KEY_DOWN },
		{SDL_SCANCODE_W,      KEY_W    },
		{SDL_SCANCODE_S,      KEY_S    },
		{SDL_SCANCODE_A,      KEY_A    },
		{SDL_SCANCODE_D,      KEY_D    },
		{SDL_SCANCODE_SPACE,  KEY_SPACE},
		{SDL_SCANCODE_LSHIFT, KEY_SHIFT},
		{SDL_SCANCODE_LCTRL,  KEY_CTRL },
	};
	bool key_state[KEY_COUNT] = {false};
	bool physics_enabled = false;
	bool hud_enabled = true;
	bool screenshot_requested = false;
	vec3 phys_velxz = vec3(0, 0, 0);
	float phys_vely = 0.f;
	bool phys_ground = false;
	uint8_t id_in_hand = 35;
	uint8_t data_in_hand = 1;
	uint8_t cloth_color = 1;

	RaycastResult ray;
	bool ray_valid = false;
	auto remove_block = [&](){
		if (ray_valid) {
			uint8_t old_id = level.get_tile_id(ray.x, ray.y, ray.z);
#ifdef RSGAME_NETCLIENT
			uint8_t old_data = level.get_tile_meta(ray.x, ray.y, ray.z);
			uint8_t pbuf[2+9];
			PacketWriter(pbuf)
				.write8(C_ChangeBlock)
				.write32(level.pos_to_index(ray.x, ray.y, ray.z))
				.write8(old_id)
				.write8(old_data)
				.write8(0)
				.write8(0)
				.send(sock);
			level.set_tile(ray.x, ray.y, ray.z, 0, 0);
#else
			level.set_tile(ray.x, ray.y, ray.z, 0, 0);
			level.on_block_remove(ray.x, ray.y, ray.z, old_id);
#endif
		}
	};
	auto place_block = [&](){
		if (ray_valid) {
			int x = ray.x, y = ray.y, z = ray.z;
			switch (ray.f) {
				case 0: y--; break;
				case 1: y++; break;
				case 2: z--; break;
				case 3: z++; break;
				case 4: x--; break;
				case 5: x++; break;
			}
			uint8_t data = data_in_hand;
			if (tiles::render_type[id_in_hand] == RenderType::TORCH) {
				switch (ray.f) {
					case 2: data = 4; break;
					case 3: data = 3; break;
					case 4: data = 2; break;
					case 5: data = 1; break;
					default: data = 5; break;
				}
			}
#ifdef RSGAME_NETCLIENT
			uint8_t old_id = level.get_tile_id(x, y, z);
			uint8_t old_data = level.get_tile_meta(x, y, z);
			uint8_t pbuf[2+9];
			PacketWriter(pbuf)
				.write8(C_ChangeBlock)
				.write32(level.pos_to_index(x, y, z))
				.write8(old_id)
				.write8(old_data)
				.write8(id_in_hand)
				.write8(data)
				.send(sock);
			level.set_tile(x, y, z, id_in_hand, data);
#else
			level.set_tile(x, y, z, id_in_hand, data);
			level.on_block_add(x, y, z, id_in_hand);
#endif
		}
	};
	while (is_running) {
		SDL_Event ev;
		while (SDL_PollEvent(&ev)) {
			if (ev.type == SDL_QUIT) {
				is_running = false;
				break;
			} else if (ev.type == SDL_WINDOWEVENT) {
				if (ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
					width = ev.window.data1;
					height = ev.window.data2;
					glViewport(0, 0, width, height);
				} else if (ev.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
					/* NOTE: we can't restore mouse mode on FOCUS_GAINED, because
					 * on xfwm4 there's a bug where if you alt-tab from relative
					 * mode, it immediately gives focus back to your window. */
					SDL_SetRelativeMouseMode(SDL_FALSE);
				}
			} else if (ev.type == SDL_MOUSEMOTION) {
				if (SDL_GetRelativeMouseMode()) {
					yaw -= ev.motion.xrel * 0.001;
					pitch -= ev.motion.yrel * 0.001;
					if (pitch > 3.1415f/2.f)
						pitch = 3.1415f/2.f;
					if (pitch < -3.1415f/2.f)
						pitch = -3.1415f/2.f;
				}
			} else if (ev.type == SDL_MOUSEBUTTONDOWN) {
				if (!SDL_GetRelativeMouseMode()) {
					SDL_SetRelativeMouseMode(SDL_TRUE);
				} else {
					if (ev.button.button == SDL_BUTTON_LEFT) {
						remove_block();
					} else if (ev.button.button == SDL_BUTTON_RIGHT) {
						place_block();
					}
				}
			} else if (ev.type == SDL_KEYDOWN && !ev.key.repeat) {
				switch (ev.key.keysym.scancode) {
					case SDL_SCANCODE_ESCAPE:
					case SDL_SCANCODE_Q:
#ifdef RSGAME_NETCLIENT
						{
							uint8_t buf[2+5];
							PacketWriter(buf)
								.write8(B_Disconnect)
								.write_str("Quit", 4)
								.send(sock);
							net_close(sock);
						}
#endif
						is_running = false;
						break;
					case SDL_SCANCODE_F1:
						hud_enabled = !hud_enabled;
						break;
					case SDL_SCANCODE_F2:
						screenshot_requested = true;
						break;
					case SDL_SCANCODE_F10:
						SDL_SetRelativeMouseMode((SDL_bool)!SDL_GetRelativeMouseMode());
						break;
					case SDL_SCANCODE_F11:
						fullscreen = !fullscreen;
						SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
						break;
					case SDL_SCANCODE_RETURN:
						remove_block();
						break;
					case SDL_SCANCODE_BACKSLASH:
						place_block();
						break;
					case SDL_SCANCODE_F:
						physics_enabled = !physics_enabled;
						break;
					case SDL_SCANCODE_1: id_in_hand = 35; data_in_hand = cloth_color; break;
					case SDL_SCANCODE_2: id_in_hand = 55; data_in_hand = 0; break;
					case SDL_SCANCODE_3: id_in_hand = 76; data_in_hand = 0; break;
					case SDL_SCANCODE_4: id_in_hand = 44; data_in_hand = 0; break;
					case SDL_SCANCODE_LEFTBRACKET:
						if (id_in_hand == 35) {
							cloth_color--;
							cloth_color &= 15;
							data_in_hand = cloth_color;
						}
						break;
					case SDL_SCANCODE_RIGHTBRACKET:
						if (id_in_hand == 35) {
							cloth_color++;
							cloth_color &= 15;
							data_in_hand = cloth_color;
						}
						break;
					case SDL_SCANCODE_L: pos = vec3(33.f, 2.f, 32.f); break;
					case SDL_SCANCODE_O:
						render_ao_enabled = !render_ao_enabled;
						fprintf(stderr, "Ambient Occlusion: %s\n", render_ao_enabled ? "on" : "off");
						rl->set_all_dirty();
						break;
					default:
						break;
				}
			}
			if (ev.type == SDL_KEYDOWN || ev.type == SDL_KEYUP) {
				auto it = key_map.find(ev.key.keysym.scancode);
				if (it != key_map.end())
					key_state[it->second] = ev.type == SDL_KEYDOWN;
			}
		}
		// process input
		{
			Uint64 current_frame = SDL_GetTicks64();
			unprocessed_ms += current_frame - last_frame;
			last_frame = current_frame;
			float speed = key_state[KEY_CTRL] ? .3f : .15f;
			float lookspeed = key_state[KEY_CTRL] ? .25f : .1f;
			while (unprocessed_ms > 50) {
				if (key_state[KEY_LEFT])
					yaw += lookspeed;
				if (key_state[KEY_RIGHT])
					yaw -= lookspeed;
				if (key_state[KEY_UP]) {
					pitch += lookspeed;
					if (pitch > 3.1415f/2.f)
						pitch = 3.1415f/2.f;
				}
				if (key_state[KEY_DOWN]) {
					pitch -= lookspeed;
					if (pitch < -3.1415f/2.f)
						pitch = -3.1415f/2.f;
				}
				if (!physics_enabled) {
					if (key_state[KEY_W])
						pos += look*speed;
					if (key_state[KEY_S])
						pos -= look*speed;
					if (key_state[KEY_A])
						pos -= normalize(cross(look, vec3(0, 1, 0)))*speed;
					if (key_state[KEY_D])
						pos += normalize(cross(look, vec3(0, 1, 0)))*speed;
					if (key_state[KEY_SPACE])
						pos += vec3(0, 1, 0)*speed;
					if (key_state[KEY_SHIFT])
						pos -= vec3(0, 1, 0)*speed;
				} else {
					raycast_in_physics = true;
					vec3 want = vec3(0, 0, 0);
					vec3 fwd = normalize(vec3(look.x, 0, look.z));
					vec3 right = normalize(vec3(-look.z, 0, look.x));
					if (key_state[KEY_W])
						want += fwd;
					if (key_state[KEY_S])
						want -= fwd;
					if (key_state[KEY_A])
						want -= right;
					if (key_state[KEY_D])
						want += right;
					// apply friction
					if (phys_ground) {
						const float friction = .1f;
						float speed = length(phys_velxz);
						if (speed)
							phys_velxz *= (speed > friction ? speed - friction : 0) / speed;
					}
					// apply acceleration
					if (want != vec3(0, 0, 0)) {
						const float acc = .1f;
						const float limit = .1f;
						want = normalize(want);
						// NOTE: speed is limited along the acceleration vector to enable airstrafing
						float v = dot(phys_velxz, want);
						if (v < limit) {
							phys_velxz += (v + acc < limit ? acc : limit - v)*want;
						}
					}
					// horizontal collision
					RaycastResult r;
					for (float shift = 0.f; shift <= 1.5f; shift += .5f)
						if (raycast(&level, pos-vec3(0, shift, 0), normalize(phys_velxz), .5, r)) {
							switch (r.f) {
							case 2: case 3: phys_velxz.z = 0; break;
							case 4: case 5: phys_velxz.x = 0; break;
							}
						}
					pos += phys_velxz;
					// gravity
					if (phys_vely > -1.f)
						phys_vely -= .1f;
					// vertical collision
					bool down_collision = false;
					for (float sx = -.4f; sx <= .4f; sx += .4f)
						for (float sy = -.4f; sy <= .4f; sy += .4f)
							if (raycast(&level, pos+vec3(sx, 0, sy), vec3(0, -1, 0), 1.6, r)) {
								down_collision = true;
								goto found_collision;
							}
				found_collision:
					if (down_collision) {
						if (phys_vely < 0) {
							phys_vely = 0;
							pos.y = r.y + 2.6f;
						}
						phys_ground = true;
					} else {
						phys_ground = false;
					}
					if (pos.y <= 1.6f) {
						phys_vely = 0;
						phys_ground = true;
						pos.y = 1.6f;
					}
					if (!phys_ground) {
						pos.y += phys_vely;
					}
					if (key_state[KEY_SPACE] && phys_ground) {
						printf("%f\n", length(phys_velxz));
						phys_vely = .5f;
						pos.y += phys_vely;
						phys_ground = false;
					}
				}
				unprocessed_ms -= 50;
#ifdef RSGAME_NETCLIENT
				{
					uint8_t pbuf[2+17];
					int x = pos.x * 32;
					int y = pos.y * 32;
					int z = pos.z * 32;
					int nyaw = yaw * 65535 / (2*glm::pi<float>());
					int npitch = pitch * 65535 / (2*glm::pi<float>());
					if (x != oldx || y != oldy || z != oldz || nyaw != oldyaw || npitch != oldpitch) {
						PacketWriter(pbuf)
							.write8(C_ChangePosition)
							.write32(x)
							.write32(y)
							.write32(z)
							.write16(nyaw)
							.write16(npitch)
							.send(sock);
						oldx = x;
						oldy = y;
						oldz = z;
						oldyaw = yaw;
						oldpitch = pitch;
					}
				}
				{
					static uint8_t pbuf[2+65536];
					static int ppos = 0;
					net_nonblock(sock);
					for (;;) {
						if (ppos < 2) {
							int r = net_read(sock, pbuf+ppos, 2-ppos);
							if (r == -1 || r == 0) {
								if (r && net_again())
									break;
								if (r)
									net_perror("net_read");
								return 1;
							}
							ppos += r;
						} else {
							int plen = pbuf[0]<<16 | pbuf[1];
							if (!plen) {
								ppos = 0;
								continue;
							}
							int r = net_read(sock, pbuf+ppos, plen+2-ppos);
							if (r == -1 || r == 0) {
								if (r && net_again())
									break;
								if (r)
									net_perror("net_read");
								return 1;
							}
							ppos += r;
							if (ppos == plen+2) {
								PacketReader pr(pbuf+2);
								switch (pr.read8()) {
									case B_Disconnect:
										fprintf(stderr, "Disconnected\n");
										return 1;
									case S_BlockUpdates:
										for (int i = 0; i < (plen-1)/6; i++) {
											uint32_t index = pr.read32();
											uint8_t id = pr.read8();
											uint8_t data = pr.read8();
											ivec3 bpos = level.index_to_pos(index);
											level.set_tile(bpos.x, bpos.y, bpos.z, id, data);
											rl->set_dirty(bpos.x, bpos.y, bpos.z);
										}
										break;
									case S_EntityEnter: {
										uint32_t eid = pr.read32();
										entities.emplace(eid, Entity());
										break;
									}
									case S_EntityLeave: {
										uint32_t eid = pr.read32();
										entities.erase(eid);
										break;
									}
									case S_EntityUpdates:
										for (int i = 0; i < (plen-1)/20; i++) {
											uint32_t eid = pr.read32();
											auto it = entities.find(eid);
											if (it != entities.end()) {
												it->second.x = (int32_t)pr.read32()/32.f;
												it->second.y = (int32_t)pr.read32()/32.f;
												it->second.z = (int32_t)pr.read32()/32.f;
												it->second.yaw = pr.read16()*2.f*glm::pi<float>()/65535;
												it->second.pitch = pr.read16()*2.f*glm::pi<float>()/65535;
											}
										}
										break;
									default:
										fprintf(stderr, "Protocol error\n");
										return 1;
								}
								ppos = 0;
							}
						}
					}
					net_block(sock);
				}
#else
				level.on_tick();
#endif
			}
		}


		look = normalize(vec3(-sinf(yaw), 0, -cosf(yaw))*cosf(pitch) + vec3(0, sinf(pitch), 0));
		mat4 view = glm::lookAt(pos, pos+look, vec3(0, 1, 0));
		constexpr float vfov = glm::radians(70.f), near = .05f, far = 256.f;
		float aspect = width/(float)height;
		mat4 proj = glm::perspective(vfov, aspect, near, far);
		viewproj = proj * view;

		Uint64 frame_start = SDL_GetPerformanceCounter();
		glClearColor(0.f, .7f, .6f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		viewfrustum.from_viewproj(pos, look, vec3(0, 1, 0), vfov, aspect, near, far);
		rl->update();
		rl->draw(viewfrustum);
#ifdef RSGAME_NETCLIENT
		{
			float player_pts[4*256];
			float *p = player_pts;
			for (auto &ent: entities) {
				if (p == (&player_pts)[1])
					break;
				if (ent.first != my_eid) {
					*p++ = ent.second.x;
					*p++ = ent.second.y;
					*p++ = ent.second.z;
					*p++ = ent.second.yaw;
				}
			}
			draw_players(player_pts, (p-player_pts)/4, pos, look);
		}
#endif
		raycast_in_physics = false;
		ray_valid = raycast(&level, pos, look, 10., ray);
		if (ray_valid && hud_enabled)
			draw_raytarget(ray);

		if (hud_enabled)
			draw_hud(width, height, id_in_hand, data_in_hand);

		if (screenshot_requested) {
			screenshot_requested = false;
			save_png_screenshot("screenshot.png", width, height);
		}
		SDL_GL_SwapWindow(window);
		Uint64 frame_end = SDL_GetPerformanceCounter();
		double frame_time = (double)(frame_end - frame_start) / SDL_GetPerformanceFrequency();
		avg_frame_time = (9*avg_frame_time + frame_time)/10;
		//fprintf(stderr, "frame time: %.2f | %.2f fps\n", avg_frame_time*1000, 1/avg_frame_time);
	}
	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
}
extern "C" int main(int argc, char** argv)
{
	return rsgame::main(argc, argv);
}
