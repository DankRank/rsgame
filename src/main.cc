// SPDX-License-Identifier: Apache-2.0 OR MIT
#include "common.hh"
#include "level.hh"
#include "render.hh"
#include "tile.hh"
#include "util.hh"
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
	while (is_running) {
		SDL_Event ev;
		while (SDL_PollEvent(&ev)) {
			if (ev.type == SDL_QUIT) {
				is_running = false;
				break;
			}
			if (ev.type == SDL_WINDOWEVENT) {
				if (ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
					width = ev.window.data1;
					height = ev.window.data2;
					glViewport(0, 0, width, height);
				}
			}
			if (ev.type == SDL_KEYDOWN) {
				switch (ev.key.keysym.scancode) {
					case SDL_SCANCODE_ESCAPE:
					case SDL_SCANCODE_Q:
						is_running = false;
						break;
					case SDL_SCANCODE_F11:
						fullscreen = !fullscreen;
						SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
						break;
					case SDL_SCANCODE_LEFT:
						yaw += 0.1;
						break;
					case SDL_SCANCODE_RIGHT:
						yaw -= 0.1;
						break;
					case SDL_SCANCODE_UP:
						pitch += 0.1;
						if (pitch > 3.1415f/2.f)
							pitch = 3.1415f/2.f;
						break;
					case SDL_SCANCODE_DOWN:
						pitch -= 0.1;
						if (pitch < -3.1415f/2.f)
							pitch = -3.1415f/2.f;
						break;
					case SDL_SCANCODE_W:
						pos += look*.1f;
						break;
					case SDL_SCANCODE_S:
						pos -= look*.1f;
						break;
					case SDL_SCANCODE_A:
						pos -= normalize(cross(look, vec3(0, 1, 0)))*.1f;
						break;
					case SDL_SCANCODE_D:
						pos += normalize(cross(look, vec3(0, 1, 0)))*.1f;
						break;
					case SDL_SCANCODE_SPACE:
						pos += vec3(0, 1, 0)*.1f;
						break;
					case SDL_SCANCODE_LSHIFT:
						pos -= vec3(0, 1, 0)*.1f;
						break;
					default:
						break;
				}
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
