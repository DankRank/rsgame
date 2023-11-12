// SPDX-License-Identifier: Apache-2.0 OR MIT
#include "common.hh"
#include "util.hh"
#include <png.h>
#include <stdio.h>
#include <filesystem>
namespace fs = std::filesystem;
#if defined(WIN32) && (defined(RSGAME_BUNDLE) || !defined(RSGAME_PORTABLE))
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <objbase.h>
#include <shlobj.h>
#undef near
#undef far
#undef min
#undef max
#endif
namespace rsgame {
static void *load_file_raw(const char *filename, size_t *size) {
	if (verbose)
		fprintf(stderr, "trying file: %s\n", filename);
	return SDL_LoadFile(filename, size);
}
#if defined(WIN32)
static void *load_file_raw(const wchar_t *filename, size_t *size) {
	if (verbose)
		fprintf(stderr, "trying file: %S\n", filename);
	FILE *fp = _wfopen(filename, L"rb");
	return fp ? SDL_LoadFile_RW(SDL_RWFromFP(fp, SDL_TRUE), size, 1) : nullptr;
}
#endif
static void *load_file_raw(const fs::path &pa, size_t *size) {
	return load_file_raw(pa.c_str(), size);
}
#if defined(WIN32) && defined(RSGAME_BUNDLE)
static bool bundle_loaded = false;
static bool bundle_failed = true;
struct BundleEntry {
	char name[257];
	void *buf;
	size_t len;
};
static bool operator<(const BundleEntry &lhs, const BundleEntry &rhs) {
	return strcmp(lhs.name, rhs.name) < 0;
}
static bool operator<(const BundleEntry &lhs, const char *rhs) {
	return strcmp(lhs.name, rhs) < 0;
}
static bool operator<(const char *lhs, const BundleEntry &rhs) {
	return strcmp(lhs, rhs.name) < 0;
}
static std::set<BundleEntry, std::less<>> bundle_set;
static void load_bundle() {
	bundle_loaded = true;
	HRSRC hResInfo = FindResourceW(nullptr, MAKEINTRESOURCEW(102), MAKEINTRESOURCEW(256));
	if (!hResInfo)
		return;
	HGLOBAL hResData = LoadResource(nullptr, hResInfo);
	if (!hResData)
		return;
	char *buf = (char *)LockResource(hResData);
	DWORD len = SizeofResource(nullptr, hResInfo);
	if (!buf || len < 16 || memcmp(buf+len-16, "assets00", 8))
		return;
	int head_size = *(uint32_t*)(buf+len-8);
	bundle_failed = false;
	for (int i = 0; i < head_size/(256+4+4); i++) {
		int ofs = (256+4+4)*i;
		BundleEntry ent;
		strncpy(ent.name, buf+ofs, 256);
		ent.name[257] = 0;
		ent.buf = buf + head_size + *(uint32_t*)(buf+ofs+256);
		ent.len = *(uint32_t*)(buf+ofs+256+4);
		bundle_set.insert(ent);
	}
}
static void *load_file_bundle(int type, const char *filename, size_t *size) {
	if (type == FILE_DATA) {
		if (!bundle_loaded)
			load_bundle();
		if (!bundle_failed) {
			auto it = bundle_set.find(filename);
			if (it != bundle_set.end()) {
				void *res = SDL_malloc(it->len + 1);
				if (res) {
					memcpy(res, it->buf, it->len);
					((char*)res)[it->len] = 0;
					if (size)
						*size = it->len;
					return res;
				}
			}
		}
	}
	return 0;
}
#endif
#define APPDIRNAME "rsgame"
#if !defined(WIN32) && !defined(RSGAME_PORTABLE)
static fs::path get_file_path_xdghome(int type) {
	static const char *homeenvs[] = { "XDG_DATA_HOME", "XDG_CONFIG_HOME", "XDG_STATE_HOME" };
	static const char *homedefault[] = { ".local/share", ".config", ".local/state" };
	fs::path pa;
	const char *p;

	p = SDL_getenv(homeenvs[type]);
	if (!p || !*p) {
		p = SDL_getenv("HOME");
		if (!p || !*p)
			p = "/";
		return fs::path(p) / homedefault[type] / APPDIRNAME;
	} else {
		return fs::path(p) / APPDIRNAME;
	}
}
static void *load_file_xdgdirs(int type, const char *filename, size_t *size) {
	static const char *dirsenvs[] = { "XDG_DATA_DIRS", "XDG_CONFIG_DIRS", nullptr };
	static const char *dirsdefault[] = { "/usr/local/share:/usr/share", "/etc/xdg", nullptr };
	const char *p, *q;
	void *res;

	if (dirsenvs[type]) {
		p = SDL_getenv(dirsenvs[type]);
		if (!p || !*p)
			p = dirsdefault[type];
		while ((q = strchr(p, ':'))) {
			if ((res = load_file_raw(fs::path(p, q) / APPDIRNAME / filename, size)))
				return res;
			p = q+1;
		}
		if ((res = load_file_raw(fs::path(p) / APPDIRNAME / filename, size)))
			return res;
	}
	return 0;
}
#endif
#if defined(WIN32) && !defined(RSGAME_PORTABLE)
static fs::path get_file_path_appdata(int type) {
	wchar_t *p = nullptr;
	if (!SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &p)) {
		static const char *suffixes[] = { "share", "config", "state" };
		fs::path pa = fs::path(p) / APPDIRNAME / suffixes[type];
		CoTaskMemFree(p);
		return pa;
	} else {
		CoTaskMemFree(p);
		return fs::path();
	}
}
#endif
static void *load_file_workdir(int type, const char *filename, size_t *size) {
	if (type == FILE_DATA)
		return load_file_raw(fs::path("assets") / filename, size);
	else
		return load_file_raw(filename, size);
}
#if defined(WIN32)
static void *load_file_exedir(int type, const char *filename, size_t *size) {
	wchar_t p[MAX_PATH];
	if (GetModuleFileNameW(nullptr, p, MAX_PATH) == MAX_PATH)
		return nullptr;
	if (type == FILE_DATA)
		return load_file_raw(fs::path(p).parent_path() / "assets" / filename, size);
	else
		return load_file_raw(fs::path(p).parent_path() / filename, size);
}
#endif
static void *load_file_impl(int type, const char *filename, size_t *size) {
	/* Search for the file in the following order:
	 * 1) ~/.local/share/rsgame, ~/.config/rsgame, ~/.local/state/rsgame (Unix)
	 * 2) %APPDATA%/rsgame/share, %APPDATA%/rsgame/config, %APPDATA%/rsgame/state (Win32)
	 * 3) Current working directory
	 * 4) Application (.exe) directory (Win32)
	 * 5) /usr/share/rsgame, /etc/xdg/rsgame (Unix)
	 * 6) Bundled resources (Win32, only FILE_DATA)
	 *
	 * 3 and 4 look for FILE_DATA in the assets subdirectory.
	 */
	void *res;
#if !defined(WIN32) && !defined(RSGAME_PORTABLE)
	if ((res = load_file_raw(get_file_path_xdghome(type) / filename, size)))
		return res;
#endif
#if defined(WIN32) && !defined(RSGAME_PORTABLE)
	fs::path pa = get_file_path_appdata(type);
	if (!pa.empty())
		if ((res = load_file_raw(pa / filename, size)))
			return res;
#endif
	if ((res = load_file_workdir(type, filename, size)))
		return res;
#if defined(WIN32)
	if ((res = load_file_exedir(type, filename, size)))
		return res;
#endif
#if !defined(WIN32) && !defined(RSGAME_PORTABLE)
	if ((res = load_file_xdgdirs(type, filename, size)))
		return res;
#endif
#if defined(WIN32) && defined(RSGAME_BUNDLE)
	if ((res = load_file_bundle(type, filename, size)))
		return res;
#endif
	return nullptr;
}
void SDLDeleter::operator()(void *p) {
	SDL_free(p);
}
std::unique_ptr<char[], SDLDeleter> load_file(int type, const char *filename, size_t &size) {
	return std::unique_ptr<char[], SDLDeleter>((char *)load_file_impl(type, filename, &size));
}
std::unique_ptr<char[], SDLDeleter> load_file(int type, const char *filename) {
	return std::unique_ptr<char[], SDLDeleter>((char *)load_file_impl(type, filename, 0));
}
GLuint compile_shader(GLenum type, const char *name, const char *source) {
	GLuint shader = glCreateShader(type);
	if (!shader) {
		fprintf(stderr, "Couldn't glCreateShader!\n");
		return 0;
	}
	const char *sources[2] = {
		shader_prologue,
		source
	};
	GLint lens[2] = {-1, -1};
	glShaderSource(shader, 2, sources, lens);
	glCompileShader(shader);
	GLint status, loglen;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &loglen);
	if (loglen && (status == GL_FALSE || verbose)) {
		std::vector<char> buf(loglen);
		glGetShaderInfoLog(shader, loglen, NULL, buf.data());
		fprintf(stderr, "%s shader %s:\n%s", type == GL_VERTEX_SHADER ? "Vertex" : "Fragment", name, buf.data());
	}
	if (status == GL_FALSE) {
		glDeleteShader(shader);
		return 0;
	}
	return shader;
}
GLuint load_shader(GLenum type, const char *filename) {
	auto source = load_file(FILE_DATA, filename);
	if (!source) {
		fprintf(stderr, "Couldn't open %s\n", filename);
		return 0;
	}
	return compile_shader(type, filename, source.get());
}
GLuint create_program(GLuint vs, GLuint fs) {
	GLuint program = glCreateProgram();
	if (program) {
		glAttachShader(program, vs);
		glAttachShader(program, fs);
	}
	return program;
}
void link_program(GLuint &program, GLuint vs, GLuint fs) {
	glLinkProgram(program);
	GLint status, loglen;
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &loglen);
	if (loglen && (status == GL_FALSE || verbose)) {
		std::vector<char> buf(loglen);
		glGetProgramInfoLog(program, loglen, NULL, buf.data());
		fprintf(stderr, "glLinkProgram:\n%s", buf.data());
	}
	glDetachShader(program, vs);
	glDetachShader(program, fs);
	if (status == GL_FALSE) {
		glDeleteProgram(program);
		program = 0;
	} else {
		/* FIXME: this isn't exactly correct, since the validation takes
		 * into account stuff like current values of uniforms, and we
		 * haven't assigned to them yet */
		glValidateProgram(program);
		glGetProgramiv(program, GL_VALIDATE_STATUS, &status);
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &loglen);
		if (loglen && (status == GL_FALSE || verbose)) {
			std::vector<char> buf(loglen);
			glGetProgramInfoLog(program, loglen, NULL, buf.data());
			fprintf(stderr, "glValidateProgram:\n%s", buf.data());
		}
	}
}
std::vector<float> &operator <<(std::vector<float> &lhs, vec3 rhs) {
	lhs.push_back(rhs.x);
	lhs.push_back(rhs.y);
	lhs.push_back(rhs.z);
	return lhs;
}
bool load_png(const char *filename) {
	size_t size = 0;
	auto data = load_file(FILE_DATA, filename, size);
	if (!data) {
		fprintf(stderr, "Couldn't open %s\n", filename);
		return false;
	}

	png_image image;
	memset(&image, 0, sizeof(image));
	image.version = PNG_IMAGE_VERSION;
	png_image_begin_read_from_memory(&image, data.get(), size);
	if (PNG_IMAGE_FAILED(image)) {
		fprintf(stderr, "%s: %s\n", filename, image.message);
		return false;
	}
	image.format = PNG_FORMAT_RGBA;
	std::unique_ptr<char[]> buf(new char[PNG_IMAGE_SIZE(image)]);
	png_image_finish_read(&image, nullptr, buf.get(), 0, nullptr);
	if (PNG_IMAGE_FAILED(image)) {
		fprintf(stderr, "%s: %s\n", filename, image.message);
		return false;
	}
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width, image.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf.get());
	png_image_free(&image);
	return true;
}
void Frustum::from_viewproj(vec3 pos, vec3 look, vec3 upish, float vfov, float aspect, float near, float far) {
	/* All implementation of this that I've seen construct the points for
	 * side planes on the near (or far) plane intersection. Since we dot
	 * product with pos for those planes, we only need to know the direction
	 * for the normals. By using the plane that's 1 unit along the look vector
	 * we can avoid multiplying look, dx and dy by near (or far). */
	vec3 right = normalize(cross(look, upish));
	vec3 up = normalize(cross(right, look));
	float dy = tanf(vfov/2);
	float dx = aspect*dy;
	n[0] = look; // near
	n[1] = -look; // far
	n[2] = normalize(cross(up,     look + right*dx)); // right
	n[3] = normalize(cross(-right, look + up*dy)); // up
	n[4] = normalize(cross(-up,    look - right*dx)); // left
	n[5] = normalize(cross(right,  look - up*dy)); // down
	d[0] = dot(n[0], pos+look*near);
	d[1] = dot(n[1], pos+look*far);
	d[2] = dot(n[2], pos);
	d[3] = dot(n[3], pos);
	d[4] = dot(n[4], pos);
	d[5] = dot(n[5], pos);
}
}
