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

	// stuff defined in main.cc
	extern bool verbose;
	extern const char *shader_prologue, *vertex_prologue, *fragment_prologue;
	extern mat4 viewproj;
}
#endif
