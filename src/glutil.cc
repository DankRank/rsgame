// SPDX-License-Identifier: Apache-2.0 OR MIT
#include "common.hh"
#include "glutil.hh"
#include "util.hh"
#include <png.h>
namespace rsgame {
GLuint compile_shader(GLenum type, const char *name, const char *source) {
	GLuint shader = glCreateShader(type);
	if (!shader) {
		fprintf(stderr, "Couldn't glCreateShader!\n");
		return 0;
	}
	const char *sources[3] = {
		shader_prologue,
		type == GL_VERTEX_SHADER ? vertex_prologue : fragment_prologue,
		source
	};
	GLint lens[3] = {-1, -1, -1};
	glShaderSource(shader, 3, sources, lens);
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
void save_png_screenshot(const char *filename, int width, int height) {
	png_image image;
	memset(&image, 0, sizeof(image));
	image.version = PNG_IMAGE_VERSION;
	image.width = width;
	image.height = height;
	image.format = PNG_FORMAT_RGBA;
	std::unique_ptr<char[]> buf(new char[PNG_IMAGE_SIZE(image)]);
	glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, buf.get());
	png_image_write_to_file(&image, filename, 0, buf.get(), -(int)PNG_IMAGE_ROW_STRIDE(image), nullptr);
	if (PNG_IMAGE_FAILED(image)) {
		fprintf(stderr, "%s: %s\n", filename, image.message);
	}
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
bool Frustum::visible(const AABB &aabb) const {
	/* If all AABB points are behind one of the planes,
	 * the entire volume is outside the frustum.
	 *
	 * This is a conservative check.
	 */
	for (int i = 0; i < 6; i++) {
		if (dot(n[i], vec3(aabb.min.x, aabb.min.y, aabb.min.z)) < d[i] &&
			dot(n[i], vec3(aabb.max.x, aabb.min.y, aabb.min.z)) < d[i] &&
			dot(n[i], vec3(aabb.min.x, aabb.max.y, aabb.min.z)) < d[i] &&
			dot(n[i], vec3(aabb.min.x, aabb.min.y, aabb.max.z)) < d[i] &&
			dot(n[i], vec3(aabb.max.x, aabb.max.y, aabb.min.z)) < d[i] &&
			dot(n[i], vec3(aabb.min.x, aabb.max.y, aabb.max.z)) < d[i] &&
			dot(n[i], vec3(aabb.max.x, aabb.min.y, aabb.max.z)) < d[i] &&
			dot(n[i], vec3(aabb.max.x, aabb.max.y, aabb.max.z)) < d[i])
			return false;
	}
	return true;
}
Program::Program(const ProgramInfo &info) {
	prog = 0;
	GLuint vs = load_shader(GL_VERTEX_SHADER, info.vsname);
	if (vs) {
		GLuint fs = load_shader(GL_FRAGMENT_SHADER, info.fsname);
		if (fs) {
			int i;
			prog = create_program(vs, fs);

			i = 0;
			for (auto p : info.attribnames)
				glBindAttribLocation(prog, i++, p);

			link_program(prog, vs, fs);

			u = std::make_unique<GLuint[]>(info.uniformnames.size());
			i = 0;
			for (auto p : info.uniformnames)
				u[i++] = glGetUniformLocation(prog, p);

			glUseProgram(prog);
			i = 0;
			for (auto p : info.texnames)
				glUniform1i(glGetUniformLocation(prog, p), i++);
		}
		glDeleteShader(fs);
	}
	glDeleteShader(vs);
}
void Texture::gen(GLenum target_) {
	target = target_;
	glGenTextures(1, &texture);
}
void Texture::bind(int unit) const {
	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(target, texture);
}
void use_program_tex(const Program &prog, std::initializer_list<Texture> texs) {
	glUseProgram(prog.prog);
	int i = 0;
	for (auto &tex : texs)
		tex.bind(i++);
}
void texture_disable_filtering() {
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}
void VertexArray::setfp(GLuint index, GLuint vb, int size, int stride, int pointer) {
	if (index >= nattr)
		nattr = index+1;
	auto &attr = attrs[index];
	attr.vb = vb;
	attr.size = size;
	attr.stride = stride;
	attr.pointer = pointer;
}
void VertexArray::setff(GLuint index, int size, float f0, float f1, float f2, float f3) {
	(void)size;
	if (index >= nattr)
		nattr = index+1;
	auto &attr = attrs[index];
	attr.size = 0;
	attr.f[0] = f0;
	attr.f[1] = f1;
	attr.f[2] = f2;
	attr.f[3] = f3;
}
void VertexArray::bind() const {
	GLuint vb = -1;
	for (size_t i = 0; i < nattr; i++) {
		auto &attr = attrs[i];
		if (attr.size) {
			if (vb != attr.vb) {
				vb = attr.vb;
				glBindBuffer(GL_ARRAY_BUFFER, vb);
			}
			glEnableVertexAttribArray(i);
			glVertexAttribPointer(i, attr.size, GL_FLOAT, GL_FALSE, attr.stride*sizeof(float), (void*)(attr.pointer*sizeof(float)));
		} else {
			glDisableVertexAttribArray(i);
			glVertexAttrib4fv(i, attr.f);
		}
	}
}
bool has_instanced_arrays() {
	static bool result = false;
	static bool inited = false;
	if (!inited) {
		int glver = epoxy_gl_version();
		result = glver >= 33
			|| glver >= 30 && !epoxy_is_desktop_gl()
			|| epoxy_has_gl_extension("GL_ANGLE_instanced_arrays")
			|| epoxy_has_gl_extension("GL_ARB_instanced_arrays") && epoxy_has_gl_extension("GL_ARB_draw_instanced")
			|| epoxy_has_gl_extension("GL_EXT_instanced_arrays")
			|| epoxy_has_gl_extension("GL_NV_instanced_arrays") && epoxy_has_gl_extension("GL_NV_draw_instanced");
		fprintf(stderr, "has_instanced_arrays: %s\n", result ? "true" : "false");
		inited = true;
	}
	return result;
}
}
