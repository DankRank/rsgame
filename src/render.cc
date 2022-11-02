// SPDX-License-Identifier: Apache-2.0 OR MIT
#include "common.hh"
#include "render.hh"
#include <epoxy/gl.h>
namespace rsgame {
GLuint terrain_prog = 0;
static GLuint terrain_u_viewproj = 0;
static GLuint terrain_u_tex = 0;
static GLuint terrain_tex = 0;

std::vector<float> RenderChunk::data;
RenderChunk::RenderChunk(int x, int y, int z) :x(x), y(y), z(z) {
	glGenVertexArrays(1, &va);
	glGenBuffers(1, &vb);
	glBindBuffer(GL_ARRAY_BUFFER, vb);
	glBindVertexArray(va);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(3*sizeof(float)));
	glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(5*sizeof(float)));
	size = 0;
	cap = 0;
}
RenderChunk::~RenderChunk() {
	glDeleteVertexArrays(1, &va);
	glDeleteBuffers(1, &vb);
}
void RenderChunk::flip() {
	glBindBuffer(GL_ARRAY_BUFFER, vb);
	size = data.size();
	if (size <= cap) {
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float)*size, data.data());
	} else {
		glBufferData(GL_ARRAY_BUFFER, sizeof(float)*size, data.data(), GL_STREAM_DRAW);
		cap = size;
	}
}
void RenderChunk::draw() {
	glBindVertexArray(va);
	glDrawArrays(GL_TRIANGLES, 0, size/6);
}
static constexpr uint64_t rc_coord(int x, int z) {
	return (uint64_t)x << 36 & 0xFFFFFFF0'00000000 | (uint64_t)z<<4 & 0xFFFFFFF0;
}

void RenderLevel::on_load_chunk(int x, int z) {
	uint64_t c = rc_coord(x, z);
	for (int y = 0; y < 128/16; y++) {
		auto it = chunks.find(c+y);
		if (it == chunks.end()) {
			it = chunks.insert(std::make_pair(c + y, new RenderChunk(x<<4, y<<4, z<<4))).first;
		} else {
			fprintf(stderr, "RenderLevel: chunk %d,%d,%d loaded twice\n", x, y, z);
		}
		dirty_chunks.insert(it->second);
	}
}
void RenderLevel::on_unload_chunk(int x, int z) {
	uint64_t c = rc_coord(x, z);
	for (int y = 0; y < 128/16; y++) {
		auto it = chunks.find(c+y);
		if (it != chunks.end()) {
			dirty_chunks.erase(it->second);
			delete it->second;
			chunks.erase(it);
		} else {
			fprintf(stderr, "RenderLevel: chunk %d,%d,%d unloaded twice\n", x, y, z);
		}
	}
}
void RenderLevel::set_dirty1(int x, int y, int z) {
	x>>=4; y>>=4; z>>=4;
	if (y < 0 || y > 7)
		return;
	auto it = chunks.find(rc_coord(x, z)+y);
	if (it != chunks.end())
		dirty_chunks.insert(it->second);
}
void RenderLevel::set_dirty(int x, int y, int z) {
	if ((x&15) == 0)
		set_dirty1(x-16, y, z);
	else if ((x&15) == 15)
		set_dirty1(x+16, y, z);
	if ((y&15) == 0)
		set_dirty1(x, y-16, z);
	else if ((y&15) == 15)
		set_dirty1(x, y+16, z);
	if ((z&15) == 0)
		set_dirty1(x, y, z-16);
	else if ((z&15) == 15)
		set_dirty1(x, y, z+16);
	set_dirty1(x, y, z);
}
void RenderLevel::update(Uint64 target) {
	int count = 0;
	auto it = dirty_chunks.begin();
	while (it != dirty_chunks.end() && count < 64) {
		auto rc = *it;
		RenderChunk::data.clear();
		for (int y = rc->y; y < rc->y+16; y++)
			for (int z = rc->z; z < rc->z+16; z++)
				for (int x = rc->x; x < rc->x+16; x++)
					draw_block(level, level->get_tile_id(x, y, z), x, y, z, level->get_tile_meta(x, y, z));
		rc->flip();
		it = dirty_chunks.erase(it);
		if (target) {
			if (SDL_GetPerformanceCounter() > target)
				break;
		} else {
			count++;
		}
	}
}
void RenderLevel::draw() {
	glUseProgram(terrain_prog);
	glUniformMatrix4fv(terrain_u_viewproj, 1, GL_FALSE, value_ptr(viewproj));
	for (auto &kv : chunks) {
		auto &rc = *kv.second;
		int i;
		if (rc.size == 0)
			continue;
		for (i = 0; i < 6; i++) {
			if (dot(viewfrustum.n[i], vec3(rc.x,    rc.y,    rc.z   )) < viewfrustum.d[i] &&
				dot(viewfrustum.n[i], vec3(rc.x+16, rc.y,    rc.z   )) < viewfrustum.d[i] &&
				dot(viewfrustum.n[i], vec3(rc.x,    rc.y+16, rc.z   )) < viewfrustum.d[i] &&
				dot(viewfrustum.n[i], vec3(rc.x,    rc.y,    rc.z+16)) < viewfrustum.d[i] &&
				dot(viewfrustum.n[i], vec3(rc.x+16, rc.y+16, rc.z   )) < viewfrustum.d[i] &&
				dot(viewfrustum.n[i], vec3(rc.x,    rc.y+16, rc.z+16)) < viewfrustum.d[i] &&
				dot(viewfrustum.n[i], vec3(rc.x+16, rc.y,    rc.z+16)) < viewfrustum.d[i] &&
				dot(viewfrustum.n[i], vec3(rc.x+16, rc.y+16, rc.z+16)) < viewfrustum.d[i])
				break;
		}
		if (i == 6)
			rc.draw();
		//else
		//	printf("check %i failed\n", i);
	}
}
static void push_quad(std::vector<float> &data, vec3 a, vec3 ta, vec3 b, vec3 tb, vec3 c, vec3 tc, vec3 d, vec3 td) {
	data << a << ta << b << tb << c << tc << a << ta << c << tc << d << td;
}
void raytarget_face(const RaycastResult &r, float *buf)
{
	// same stuff as in draw_face_basic, but simplified
	static const vec3 vecs[6][3] = {
		{ vec3(0, 0, 0), vec3(0,  0, 1), vec3( 1, 0,  0) },
		{ vec3(0, 1, 0), vec3(0,  0, 1), vec3( 1, 0,  0) },
		{ vec3(1, 1, 0), vec3(0, -1, 0), vec3(-1, 0,  0) },
		{ vec3(0, 1, 1), vec3(0, -1, 0), vec3( 1, 0,  0) },
		{ vec3(0, 1, 0), vec3(0, -1, 0), vec3( 0, 0,  1) },
		{ vec3(1, 1, 1), vec3(0, -1, 0), vec3( 0, 0, -1) },
	};
	vec3 v(r.x, r.y, r.z);
	v += vecs[r.f][0];
	*buf++ = v.x; *buf++ = v.y; *buf++ = v.z;
	v += vecs[r.f][1];
	*buf++ = v.x; *buf++ = v.y; *buf++ = v.z;
	v += vecs[r.f][2];
	*buf++ = v.x; *buf++ = v.y; *buf++ = v.z;
	v -= vecs[r.f][1];
	*buf++ = v.x; *buf++ = v.y; *buf++ = v.z;
}
static void draw_face_basic(float x0, float y0, float z0, float dx, float dy, float dz, int f, int tex, float ds=1.f, float dt=1.f) {
	// verticies are defined in the texture order:
	// s,t     s+1,t
	//  A <------ D
	//  | \       ^
	//  |   \     |
	//  |     \   |
	//  V       \ |
	//  B ------> C
	// s,t+1   s+1,t+1
	//
	//    f   =   0     1   2     3    4     5
	//          bottom top back front left right
	// s follows  +x   +x   -x   +x    +z   -z
	// t follows  +z   +z   -y   -y    -y   -y
	//
	// Note how top and bottom are the same.
	// In order to draw bottom correctly, we must swap B/D.
	// This makes the texture on the bottom appear flipped.
	vec3 a,b,c,d;
	float light;
	switch (f) {
		case 0:
			a = vec3(x0   , y0   , z0   );
			b = vec3(x0   , y0   , z0+dz);
			c = vec3(x0+dx, y0   , z0+dz);
			d = vec3(x0+dx, y0   , z0   );
			light = .5f;
			break;
		case 1:
			a = vec3(x0   , y0+dy, z0   );
			b = vec3(x0   , y0+dy, z0+dz);
			c = vec3(x0+dx, y0+dy, z0+dz);
			d = vec3(x0+dx, y0+dy, z0   );
			light = 1.f;
			break;
		case 2:
			a = vec3(x0+dx, y0+dy, z0   );
			b = vec3(x0+dx, y0   , z0   );
			c = vec3(x0   , y0   , z0   );
			d = vec3(x0   , y0+dy, z0   );
			light = .8f;
			break;
		case 3:
			a = vec3(x0   , y0+dy, z0+dz);
			b = vec3(x0   , y0   , z0+dz);
			c = vec3(x0+dx, y0   , z0+dz);
			d = vec3(x0+dx, y0+dy, z0+dz);
			light = .8f;
			break;
		case 4:
			a = vec3(x0   , y0+dy, z0   );
			b = vec3(x0   , y0   , z0   );
			c = vec3(x0   , y0   , z0+dz);
			d = vec3(x0   , y0+dy, z0+dz);
			light = .6f;
			break;
		case 5:
			a = vec3(x0+dx, y0+dy, z0+dz);
			b = vec3(x0+dx, y0   , z0+dz);
			c = vec3(x0+dx, y0   , z0   );
			d = vec3(x0+dx, y0+dy, z0   );
			light = .6f;
			break;
	}
	vec3 ta,tb,tc,td;
	float s = tex%16/16.f;
	float t = tex/16/16.f;
	ta = vec3(s        , t        , light);
	tb = vec3(s        , t+dt/16.f, light);
	tc = vec3(s+ds/16.f, t+dt/16.f, light);
	td = vec3(s+ds/16.f, t        , light);
	if (f != 0) {
		push_quad(RenderChunk::data, a, ta, b, tb, c, tc, d, td);
	} else {
		push_quad(RenderChunk::data, a, ta, d, tb, c, tc, b, td);
	}
}
static void draw_face(int x, int y, int z, int f, int tex) {
	draw_face_basic(x, y, z, 1.f, 1.f, 1.f, f, tex);
}
void RenderLevel::draw_block(Level *level, uint8_t id, int x, int y, int z, int data)
{
	switch (tiles::render_type[id]) {
	case RenderType::AIR:
		break;
	case RenderType::CUBE:
		if (!tiles::is_opaque[level->get_tile_id(x, y-1, z)])
			draw_face(x, y, z, 0, tiles::tex(id, 0, data));
		if (!tiles::is_opaque[level->get_tile_id(x, y+1, z)])
			draw_face(x, y, z, 1, tiles::tex(id, 1, data));
		if (!tiles::is_opaque[level->get_tile_id(x, y, z-1)])
			draw_face(x, y, z, 2, tiles::tex(id, 2, data));
		if (!tiles::is_opaque[level->get_tile_id(x, y, z+1)])
			draw_face(x, y, z, 3, tiles::tex(id, 3, data));
		if (!tiles::is_opaque[level->get_tile_id(x-1, y, z)])
			draw_face(x, y, z, 4, tiles::tex(id, 4, data));
		if (!tiles::is_opaque[level->get_tile_id(x+1, y, z)])
			draw_face(x, y, z, 5, tiles::tex(id, 5, data));
		break;
	case RenderType::PLANT: {
		int tex = tiles::tex(id, 0, data);
		float s = tex%16/16.f;
		float t = tex/16/16.f;
		vec3 ta(s       , t       , 1.f);
		vec3 tb(s       , t+1/16.f, 1.f);
		vec3 tc(s+1/16.f, t+1/16.f, 1.f);
		vec3 td(s+1/16.f, t       , 1.f);
		float p = 0.05;
		float q = 1-p;
		vec3 a(x+p, y,   z+p);
		vec3 b(x+q, y,   z+p);
		vec3 c(x+p, y,   z+q);
		vec3 d(x+q, y,   z+q);
		vec3 e(x+p, y+1, z+p);
		vec3 f(x+q, y+1, z+p);
		vec3 g(x+p, y+1, z+q);
		vec3 h(x+q, y+1, z+q);

		push_quad(RenderChunk::data, g, ta, c, tb, b, tc, f, td);
		push_quad(RenderChunk::data, f, ta, b, tb, c, tc, g, td);
		push_quad(RenderChunk::data, e, ta, a, tb, d, tc, h, td);
		push_quad(RenderChunk::data, h, ta, d, tb, a, tc, e, td);
		break;
	}
	case RenderType::SLAB:
		if (!tiles::is_opaque[level->get_tile_id(x, y-1, z)])
			draw_face(x, y, z, 0, tiles::tex(id, 0, data));
		draw_face_basic(x, y-.5f, z, 1.f, 1.f, 1.f, 1, tiles::tex(id, 1, data));
		if (!tiles::is_opaque[level->get_tile_id(x, y, z-1)])
			draw_face_basic(x, y, z, 1.f, .5f, 1.f, 2, tiles::tex(id, 2, data), 1.f, .5f);
		if (!tiles::is_opaque[level->get_tile_id(x, y, z+1)])
			draw_face_basic(x, y, z, 1.f, .5f, 1.f, 3, tiles::tex(id, 3, data), 1.f, .5f);
		if (!tiles::is_opaque[level->get_tile_id(x-1, y, z)])
			draw_face_basic(x, y, z, 1.f, .5f, 1.f, 4, tiles::tex(id, 4, data), 1.f, .5f);
		if (!tiles::is_opaque[level->get_tile_id(x+1, y, z)])
			draw_face_basic(x, y, z, 1.f, .5f, 1.f, 5, tiles::tex(id, 5, data), 1.f, .5f);

		break;
	}

}
RenderLevel::RenderLevel(Level *level) :level(level) {}
RenderLevel::~RenderLevel() {
	for (auto &kv : chunks)
		delete kv.second;
}
bool load_shaders() {
	GLuint vs = load_shader(GL_VERTEX_SHADER, "assets/terrain.vert");
	GLuint fs = load_shader(GL_FRAGMENT_SHADER, "assets/terrain.frag");
	if (vs && fs) {
		terrain_prog = create_program(vs, fs);
		glBindAttribLocation(terrain_prog, 0, "i_position");
		glBindAttribLocation(terrain_prog, 1, "i_texcoord");
		glBindAttribLocation(terrain_prog, 2, "i_light");
		link_program(terrain_prog, vs, fs);
		glDeleteShader(vs);
		glDeleteShader(fs);
		terrain_u_viewproj = glGetUniformLocation(terrain_prog, "u_viewproj");
		terrain_u_tex = glGetUniformLocation(terrain_prog, "u_tex");
	}
	return !!terrain_prog;
}
bool load_textures() {
	glUseProgram(terrain_prog);
	glUniform1i(terrain_u_tex, 0);
	glGenTextures(1, &terrain_tex);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, terrain_tex);
	if (!load_png("assets/terrain.png"))
	//if (!load_png("terrain.png"))
		return false;
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	return true;
}
}
