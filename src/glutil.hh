// SPDX-License-Identifier: Apache-2.0 OR MIT
#ifndef RSGAME_GLUTIL
#define RSGAME_GLUTIL
namespace rsgame {
	GLuint compile_shader(GLenum type, const char *name, const char *source);
	GLuint load_shader(GLenum type, const char *filename);
	GLuint create_program(GLuint vs, GLuint fs);
	void link_program(GLuint &program, GLuint vs, GLuint fs);
	std::vector<float> &operator <<(std::vector<float> &lhs, vec3 rhs);
	bool load_png(const char *filename);
	void save_png_screenshot(const char *filename, int width, int height);
	struct Frustum {
		/* We represent a frustum as an intersection of 6 half-spaces (= directed planes).
		 * n[i] is the unit normal of the i-th plane, pointing inside the frustum.
		 * n[i]*d[i] is a point on the i-th plane. */
		vec3 n[6];
		float d[6];
		void from_viewproj(vec3 pos, vec3 look, vec3 upish, float vfov, float aspect, float near, float far);
		bool visible(const AABB &aabb) const;
	};
	struct ProgramInfo {
		const char *vsname;
		const char *fsname;
		std::vector<const char *> attribnames;
		std::vector<const char *> uniformnames;
		std::vector<const char *> texnames;
	};
	struct Program {
		GLuint prog;
		std::unique_ptr<GLuint[]> u;
		Program() {}
		Program(const ProgramInfo &info);
	};
	struct Texture {
		GLenum target;
		GLuint texture;
		void gen(GLenum target_);
		void bind(int unit) const;
	};
	void use_program_tex(const Program &prog, std::initializer_list<Texture> texs = {});
	void texture_disable_filtering();
	struct VertexArray {
		unsigned nattr;
		struct Attr {
			unsigned char size;
			union {
				struct {
					float f[4];
				};
				struct {
					GLuint vb;
					int stride;
					int pointer;
				};
			};
		};
		Attr attrs[4];
		void setfp(GLuint index, GLuint buf, int size, int stride, int pointer);
		void setff(GLuint index, int size, float f0, float f1, float f2, float f3);
		void bind() const;
	};
	bool has_instanced_arrays();
}
#endif
