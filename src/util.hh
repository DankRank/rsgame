// SPDX-License-Identifier: Apache-2.0 OR MIT
#ifndef RSGAME_UTIL
#define RSGAME_UTIL
namespace rsgame {
	struct SDLDeleter {
		void operator()(void *p);
	};
	enum {
		FILE_DATA,
		FILE_CONFIG,
		FILE_STATE,
	};
	std::unique_ptr<char[], SDLDeleter> load_file(int type, const char *filename, size_t &size);
	std::unique_ptr<char[], SDLDeleter> load_file(int type, const char *filename);
	GLuint compile_shader(GLenum type, const char *name, const char *source);
	GLuint load_shader(GLenum type, const char *filename);
	GLuint create_program(GLuint vs, GLuint fs);
	void link_program(GLuint &program, GLuint vs, GLuint fs);
	std::vector<float> &operator <<(std::vector<float> &lhs, vec3 rhs);
	bool load_png(const char *filename);
	void save_png_screenshot(const char *filename, int width, int height);
	struct Frustum {
		vec3 n[6];
		float d[6];
		void from_viewproj(vec3 pos, vec3 look, vec3 upish, float vfov, float aspect, float near, float far);
	};

	// stuff defined in main.cc
	extern bool verbose;
	extern bool gles;
	extern const char *shader_prologue;
	extern mat4 viewproj;
	extern Frustum viewfrustum;
}
#endif
