// SPDX-License-Identifier: Apache-2.0 OR MIT
#include "common.hh"
#include "level.hh"
#include "render.hh"
#include "tile.hh"
#include "raycast.hh"
#include "util.hh"
namespace rsgame {
bool verbose = true;
bool gles = false;
static bool glcore = false;
const char *shader_prologue = nullptr;
bool vsync = true;
bool fullscreen = false;
SDL_Window* window = nullptr;
bool is_running = true;
mat4 viewproj;
Frustum viewfrustum;
int main(int argc, char** argv)
{
	tiles::init();

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
	window = SDL_CreateWindow("rsgame", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height,
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

	fprintf(stderr, "Loading level...\n");
	Level level;

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

	GLuint raytarget_va, raytarget_vb;
	glGenVertexArrays(1, &raytarget_va);
	glGenBuffers(1, &raytarget_vb);
	glBindBuffer(GL_ARRAY_BUFFER, raytarget_vb);
	glBindVertexArray(raytarget_va);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
	float raytarget_buf[4*3];
	init_hud();

#if 0
	GLuint debug_va, debug_vb;
	glGenVertexArrays(1, &debug_va);
	glGenBuffers(1, &debug_vb);
	glBindBuffer(GL_ARRAY_BUFFER, debug_vb);
	glBindVertexArray(debug_va);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
	std::vector<float> debug_buf;
	auto cube = [&](vec3 a, vec3 b, vec3 c, vec3 d, vec3 e, vec3 f, vec3 g, vec3 h) {
		debug_buf << a << b << b << c << c << d << d << a;
		debug_buf << e << f << f << g << g << h << h << e;
		debug_buf << a << e << b << f << c << g << d << h;
	};
#endif

	double avg_frame_time = 0;
	Uint64 unprocessed_ms = 0;
	Uint64 last_frame = SDL_GetTicks64();
	bool key_left = false, key_right = false, key_up = false, key_down = false;
	bool key_w = false, key_s = false, key_a = false, key_d = false;
	bool key_sp = false, key_shift = false, key_ctrl = false;
	bool physics_enabled = false;
	vec3 phys_velxz = vec3(0, 0, 0);
	float phys_vely = 0.f;
	bool phys_ground = false;
	uint8_t id_in_hand = 35;
	uint8_t data_in_hand = 1;

	RaycastResult ray;
	bool ray_valid = false;
	auto remove_block = [&](){
		if (ray_valid) {
			uint8_t old_id = level.get_tile_id(ray.x, ray.y, ray.z);
			level.set_tile(ray.x, ray.y, ray.z, 0, 0);
			level.on_block_remove(ray.x, ray.y, ray.z, old_id);
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
			level.set_tile(x, y, z, id_in_hand, data);
			level.on_block_add(x, y, z, id_in_hand);
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
						is_running = false;
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
					case SDL_SCANCODE_1: id_in_hand = 35; data_in_hand = 1; break;
					case SDL_SCANCODE_2: id_in_hand = 55; data_in_hand = 0; break;
					case SDL_SCANCODE_3: id_in_hand = 76; data_in_hand = 0; break;
					case SDL_SCANCODE_L: pos = vec3(33.f, 2.f, 32.f); break;
					default:
						break;
				}
			}
			if (ev.type == SDL_KEYDOWN || ev.type == SDL_KEYUP) {
				bool down = ev.type == SDL_KEYDOWN;
				switch (ev.key.keysym.scancode) {
					case SDL_SCANCODE_LEFT: key_left = down; break;
					case SDL_SCANCODE_RIGHT: key_right = down; break;
					case SDL_SCANCODE_UP: key_up = down; break;
					case SDL_SCANCODE_DOWN: key_down = down; break;
					case SDL_SCANCODE_W: key_w = down; break;
					case SDL_SCANCODE_S: key_s = down; break;
					case SDL_SCANCODE_A: key_a = down; break;
					case SDL_SCANCODE_D: key_d = down; break;
					case SDL_SCANCODE_SPACE: key_sp = down; break;
					case SDL_SCANCODE_LSHIFT: key_shift = down; break;
					case SDL_SCANCODE_LCTRL: key_ctrl = down; break;
					default: break;
				}
			}
		}
		// process input
		{
			Uint64 current_frame = SDL_GetTicks64();
			unprocessed_ms += current_frame - last_frame;
			last_frame = current_frame;
			float speed = key_ctrl ? .3f : .15f;
			float lookspeed = key_ctrl ? .25f : .1f;
			while (unprocessed_ms > 50) {
				if (key_left)
					yaw += lookspeed;
				if (key_right)
					yaw -= lookspeed;
				if (key_up) {
					pitch += lookspeed;
					if (pitch > 3.1415f/2.f)
						pitch = 3.1415f/2.f;
				}
				if (key_down) {
					pitch -= lookspeed;
					if (pitch < -3.1415f/2.f)
						pitch = -3.1415f/2.f;
				}
				if (!physics_enabled) {
					if (key_w)
						pos += look*speed;
					if (key_s)
						pos -= look*speed;
					if (key_a)
						pos -= normalize(cross(look, vec3(0, 1, 0)))*speed;
					if (key_d)
						pos += normalize(cross(look, vec3(0, 1, 0)))*speed;
					if (key_sp)
						pos += vec3(0, 1, 0)*speed;
					if (key_shift)
						pos -= vec3(0, 1, 0)*speed;
				} else {
					raycast_in_physics = true;
					vec3 want = vec3(0, 0, 0);
					vec3 fwd = normalize(vec3(look.x, 0, look.z));
					vec3 right = normalize(vec3(-look.z, 0, look.x));
					if (key_w)
						want += fwd;
					if (key_s)
						want -= fwd;
					if (key_a)
						want -= right;
					if (key_d)
						want += right;
					// apply friction
					if (phys_ground) {
						const float friction = .1;
						float speed = length(phys_velxz);
						if (speed)
							phys_velxz *= (speed > friction ? speed - friction : 0) / speed;
					}
					// apply acceleration
					if (want != vec3(0, 0, 0)) {
						const float acc = .1f;
						const float limit = .1f;
						want = normalize(want);
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
					if (key_sp && phys_ground) {
						printf("%f\n", length(phys_velxz));
						phys_vely = .5f;
						pos.y += phys_vely;
						phys_ground = false;
					}
				}
				level.on_tick();
				unprocessed_ms -= 50;
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
		rl->draw();

		raycast_in_physics = false;
		ray_valid = raycast(&level, pos, look, 10., ray);
		if (ray_valid) {
			raytarget_face(ray, raytarget_buf);
			glBindBuffer(GL_ARRAY_BUFFER, raytarget_vb);
			glBufferData(GL_ARRAY_BUFFER, sizeof(float)*4*3, raytarget_buf, GL_STREAM_DRAW);
			glBindVertexArray(raytarget_va);
			glDisable(GL_DEPTH_TEST);
			glVertexAttrib2f(1, 0.f, 0.f);
			glVertexAttrib1f(2, 0.f);
			glDrawArrays(GL_LINE_LOOP, 0, 4);
			glEnable(GL_DEPTH_TEST);
		}

		draw_hud(width, height);
#if 0
		glBindBuffer(GL_ARRAY_BUFFER, debug_vb);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float)*debug_buf.size(), debug_buf.data(), GL_STREAM_DRAW);
		glBindVertexArray(debug_va);
		glDrawArrays(GL_LINES, 0, debug_buf.size()/3);
		debug_buf.clear();
#endif
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
