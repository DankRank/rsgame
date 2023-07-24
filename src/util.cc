// SPDX-License-Identifier: Apache-2.0 OR MIT
#include "common.hh"
#include "util.hh"
#include <png.h>
#include <stdio.h>
#if defined(WIN32) && defined(RSGAME_BUNDLE)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#undef min
#undef max
#endif
namespace rsgame {
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
#endif
static void *load_file_impl(const char *filename, size_t *size) {
	void *res = SDL_LoadFile(filename, size);
	if (res)
		return res;
#if defined(WIN32) && defined(RSGAME_BUNDLE)
	if (!bundle_loaded)
		load_bundle();
	if (!bundle_failed) {
		auto it = bundle_set.find(filename);
		if (it != bundle_set.end()) {
			res = SDL_malloc(it->len + 1);
			if (res) {
				memcpy(res, it->buf, it->len);
				((char*)res)[it->len] = 0;
				if (size)
					*size = it->len;
				return res;
			}
		}
	}
#endif
	return nullptr;
}
void SDLDeleter::operator()(void *p) {
	SDL_free(p);
}
std::unique_ptr<char[], SDLDeleter> load_file(const char *filename, size_t &size) {
	return std::unique_ptr<char[], SDLDeleter>((char *)load_file_impl(filename, &size));
}
std::unique_ptr<char[], SDLDeleter> load_file(const char *filename) {
	return std::unique_ptr<char[], SDLDeleter>((char *)load_file_impl(filename, 0));
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
	auto source = load_file(filename);
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
	auto data = load_file(filename, size);
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
