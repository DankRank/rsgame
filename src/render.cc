// SPDX-License-Identifier: Apache-2.0 OR MIT
#include "common.hh"
#include "render.hh"
#include <stdio.h>
namespace rsgame {
GLuint terrain_prog = 0;
GLuint flat_prog = 0;
static GLuint terrain_u_viewproj = 0;
static GLuint terrain_u_tex = 0;
static GLuint terrain_tex = 0;
static GLuint terrain_u_lighttex = 0;
static GLuint terrain_lighttex = 0;
static GLuint flat_u_viewproj = 0;
enum {
	TERRAIN_TEXTURE_TERRAIN = 0,
	TERRAIN_TEXTURE_LIGHT,
};
enum {
	TERRAIN_I_POSITION = 0,
	TERRAIN_I_TEXCOORD,
	TERRAIN_I_LIGHT,
};
enum {
	FLAT_I_POSITION = 0,
	FLAT_I_COLOR,
};
enum {
	LIGHT_TOP = 0,
	LIGHT_SIDEZ,
	LIGHT_SIDEX,
	LIGHT_BOTTOM,
	LIGHT_NONE,
	LIGHT_WIRE0,
	LIGHT_WIRE1,
	LIGHT_WIRE2,
	LIGHT_WIRE3,
	LIGHT_WIRE4,
	LIGHT_WIRE5,
	LIGHT_WIRE6,
	LIGHT_WIRE7,
	LIGHT_WIRE8,
	LIGHT_WIRE9,
	LIGHT_WIRE10,
	LIGHT_WIRE11,
	LIGHT_WIRE12,
	LIGHT_WIRE13,
	LIGHT_WIRE14,
	LIGHT_WIRE15,
};
static const float light_values[32*4] = {
	1.f, 1.f, 1.f, 1.f, // LIGHT_TOP
	.8f, .8f, .8f, 1.f, // LIGHT_SIDEZ
	.6f, .6f, .6f, 1.f, // LIGHT_SIDEX
	.5f, .5f, .5f, 1.f, // LIGHT_BOTTOM
	.0f, .0f, .0f, 1.f, // LIGHT_NONE
	.0f, .0f, .0f, 1.f, // LIGHT_WIRE0
#define WIRE_LIGHT(data) data/15.f * .6f + .4f, (data > 12 ? data/15.f * data/15.f * .7f - .5f : .0f), .0f, 1.f
	WIRE_LIGHT(1),
	WIRE_LIGHT(2),
	WIRE_LIGHT(3),
	WIRE_LIGHT(4),
	WIRE_LIGHT(5),
	WIRE_LIGHT(6),
	WIRE_LIGHT(7),
	WIRE_LIGHT(8),
	WIRE_LIGHT(9),
	WIRE_LIGHT(10),
	WIRE_LIGHT(11),
	WIRE_LIGHT(12),
	WIRE_LIGHT(13),
	WIRE_LIGHT(14),
	WIRE_LIGHT(15),
#undef WIRE_LIGHT
};
#define LIGHT_MAX (sizeof(light_values)/sizeof(float[4]))
#define LIGHT_VAL(x) ((x)/(float)LIGHT_MAX)

std::vector<float> RenderChunk::data;
RenderChunk::RenderChunk(int x, int y, int z) :x(x), y(y), z(z) {
	glGenVertexArrays(1, &va);
	glGenBuffers(1, &vb);
	glBindBuffer(GL_ARRAY_BUFFER, vb);
	glBindVertexArray(va);
	glEnableVertexAttribArray(TERRAIN_I_POSITION);
	glEnableVertexAttribArray(TERRAIN_I_TEXCOORD);
	glEnableVertexAttribArray(TERRAIN_I_LIGHT);
	glVertexAttribPointer(TERRAIN_I_POSITION, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)0);
	glVertexAttribPointer(TERRAIN_I_TEXCOORD, 2, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(3*sizeof(float)));
	glVertexAttribPointer(TERRAIN_I_LIGHT,    1, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(5*sizeof(float)));
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
static GLuint raytarget_va, raytarget_vb;
static GLuint crosshair_va, crosshair_vb;
static GLuint handitem_va, handitem_vb;
void init_raytarget()
{
	glGenVertexArrays(1, &raytarget_va);
	glGenBuffers(1, &raytarget_vb);
	glBindBuffer(GL_ARRAY_BUFFER, raytarget_vb);
	glBindVertexArray(raytarget_va);
	glEnableVertexAttribArray(FLAT_I_POSITION);
	glVertexAttribPointer(FLAT_I_POSITION, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*8*3, nullptr, GL_STREAM_DRAW);
}
void draw_raytarget(const RaycastResult &ray)
{
	vec3 v = vec3(ray.x, ray.y, ray.z) + ray.aabb->min - vec3(.002f);
	vec3 d = ray.aabb->max - ray.aabb->min + vec3(.004f);
	vec3 raytarget_buf[8] = {
		v + d*vec3(0, 0, 0),
		v + d*vec3(1, 0, 0),
		v + d*vec3(1, 0, 1),
		v + d*vec3(0, 0, 1),
		v + d*vec3(0, 1, 0),
		v + d*vec3(1, 1, 0),
		v + d*vec3(1, 1, 1),
		v + d*vec3(0, 1, 1),
	};
	glUseProgram(flat_prog);
	glUniformMatrix4fv(0, 1, GL_FALSE, value_ptr(viewproj));
	glBindBuffer(GL_ARRAY_BUFFER, raytarget_vb);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float)*8*3, value_ptr(raytarget_buf[0]));
	glBindVertexArray(raytarget_va);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glLineWidth(2.f);
	glVertexAttrib4f(FLAT_I_COLOR, .0f, .0f, .0f, .4f);
	static const uint8_t strip[10] = { 0, 1, 2, 3, 0, 4, 5, 6, 7, 4 };
	static const uint8_t lines[6] = { 1, 5, 2, 6, 3, 7 };
	glDrawElements(GL_LINE_STRIP, 10, GL_UNSIGNED_BYTE, strip);
	glDrawElements(GL_LINES, 6, GL_UNSIGNED_BYTE, lines);
	glDisable(GL_BLEND);
}
static void push_quad(std::vector<float> &data, vec3 a, vec3 ta, vec3 b, vec3 tb, vec3 c, vec3 tc, vec3 d, vec3 td) {
	data << a << ta << b << tb << c << tc << a << ta << c << tc << d << td;
}
void init_hud()
{
	glGenVertexArrays(1, &crosshair_va);
	glGenBuffers(1, &crosshair_vb);
	glBindBuffer(GL_ARRAY_BUFFER, crosshair_vb);
	glBindVertexArray(crosshair_va);
	glEnableVertexAttribArray(FLAT_I_POSITION);
	glVertexAttribPointer(FLAT_I_POSITION, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), (void*)0);
	float crosshair_buf[14*2] = {
		 0,  0,
		 9,  1,
		 1,  1,
		 1,  9,
		-1,  9,
		-1,  1,
		-9,  1,
		-9, -1,
		-1, -1,
		-1, -9,
		 1, -9,
		 1, -1,
		 9, -1,
		 9,  1,
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*14*2, crosshair_buf, GL_STREAM_DRAW);

	glGenVertexArrays(1, &handitem_va);
	glGenBuffers(1, &handitem_vb);
	glBindBuffer(GL_ARRAY_BUFFER, handitem_vb);
	glBindVertexArray(handitem_va);
	glEnableVertexAttribArray(TERRAIN_I_POSITION);
	glEnableVertexAttribArray(TERRAIN_I_TEXCOORD);
	glEnableVertexAttribArray(TERRAIN_I_LIGHT);
	glVertexAttribPointer(TERRAIN_I_POSITION, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)0);
	glVertexAttribPointer(TERRAIN_I_TEXCOORD, 2, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(3*sizeof(float)));
	glVertexAttribPointer(TERRAIN_I_LIGHT,    1, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(5*sizeof(float)));
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*3*2*3*6, nullptr, GL_STREAM_DRAW);
}
void draw_hud(int width, int height, uint8_t id, uint8_t data)
{
	// draw crosshair
	glUseProgram(flat_prog);
	mat4 m(1.f);
	m[0][0] = 5*.5f/width;
	m[1][1] = 5*.5f/height;
	glUniformMatrix4fv(flat_u_viewproj, 1, GL_FALSE, value_ptr(m));
	glBindVertexArray(crosshair_va);
	glDisable(GL_DEPTH_TEST);
	/* this makes the blend function s(1-d) + (1-s)d
	 * s=0: d
	 * s=1: 1-d */
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ONE_MINUS_SRC_COLOR);
	glVertexAttrib4f(FLAT_I_COLOR, 1.f, 1.f, 1.f, 1.f);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 14);
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);

	// draw item in hand
	glUseProgram(terrain_prog);
	m = mat4(1.f);
	glUniformMatrix4fv(terrain_u_viewproj, 1, GL_FALSE, value_ptr(m));
	glBindVertexArray(handitem_va);
	glDisable(GL_DEPTH_TEST);
	std::vector<float> verts;
	switch (tiles::render_type[id]) {
	case RenderType::AIR:
		break;
	case RenderType::CUBE:
	case RenderType::SLAB: {
		/* This isn't a proper isometric projection. Instead this
		 * is the dimetric projection commonly used in pixel art.
		 * Lateral axes have 2:1 x:y slope under this projection.
		 * The y component of a sloped unit vector is 1/sqrt(5):
		 * 1 = sqrt(2y*2y + y*y) = sqrt(5)*y */
		float scale = 1/5.f;
		float dy = scale/glm::root_five<float>();
		float dx = dy*2.f*height/width;
		/* offset is needed to center the cube horizontally
		 * total height: 1 + 2/root_five
		 * total  width: 4/root_five */
		float offset = (1 - 2/glm::root_five<float>())/2*scale*height/width;
		/*  a
		 * b c
		 *  d
		 * e f
		 *  g  */
		vec3 a(1.f-offset-dx,    1.f,             0.f);
		vec3 b(1.f-offset-dx-dx, 1.f-dy,          0.f);
		vec3 c(1.f-offset,       1.f-dy,          0.f);
		vec3 d(1.f-offset-dx,    1.f-dy-dy,       0.f);
		vec3 e(1.f-offset-dx-dx, 1.f-dy-scale,    0.f);
		vec3 f(1.f-offset,       1.f-dy-scale,    0.f);
		vec3 g(1.f-offset-dx,    1.f-dy-dy-scale, 0.f);
		uint8_t tex1 = tiles::tex(id, 1, data);
		float s1 = tex1%16/16.f;
		float t1 = tex1/16/16.f;
		vec3 t1a(s1,        t1,        LIGHT_VAL(LIGHT_TOP));
		vec3 t1b(s1,        t1+1/16.f, LIGHT_VAL(LIGHT_TOP));
		vec3 t1d(s1+1/16.f, t1+1/16.f, LIGHT_VAL(LIGHT_TOP));
		vec3 t1c(s1+1/16.f, t1,        LIGHT_VAL(LIGHT_TOP));
		uint8_t tex2 = tiles::tex(id, 3, data);
		float s2 = tex2%16/16.f;
		float t2 = tex2/16/16.f;
		vec3 t2b(s2,        t2,        LIGHT_VAL(LIGHT_SIDEZ));
		vec3 t2e(s2,        t2+1/16.f, LIGHT_VAL(LIGHT_SIDEZ));
		vec3 t2g(s2+1/16.f, t2+1/16.f, LIGHT_VAL(LIGHT_SIDEZ));
		vec3 t2d(s2+1/16.f, t2,        LIGHT_VAL(LIGHT_SIDEZ));
		uint8_t tex3 = tiles::tex(id, 5, data);
		float s3 = tex3%16/16.f;
		float t3 = tex3/16/16.f;
		vec3 t3d(s3,        t3,        LIGHT_VAL(LIGHT_SIDEX));
		vec3 t3g(s3,        t3+1/16.f, LIGHT_VAL(LIGHT_SIDEX));
		vec3 t3f(s3+1/16.f, t3+1/16.f, LIGHT_VAL(LIGHT_SIDEX));
		vec3 t3c(s3+1/16.f, t3,        LIGHT_VAL(LIGHT_SIDEX));
		push_quad(verts, a, t1a, b, t1b, d, t1d, c, t1c);
		push_quad(verts, b, t2b, e, t2e, g, t2g, d, t2d);
		push_quad(verts, d, t3d, g, t3g, f, t3f, c, t3c);
		glBindBuffer(GL_ARRAY_BUFFER, handitem_vb);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float)*verts.size(), verts.data());
		glDrawArrays(GL_TRIANGLES, 0, verts.size()/6);
		break;
	}
	case RenderType::PLANT:
	case RenderType::WIRE:
	case RenderType::TORCH: {
		float scaley = (1+2/glm::root_five<float>())/5.f;
		float scalex = scaley*height/width;
		vec3 a(1.f-scalex, 1.f,        0.f);
		vec3 b(1.f-scalex, 1.f-scaley, 0.f);
		vec3 c(1.f,        1.f-scaley, 0.f);
		vec3 d(1.f,        1.f,        0.f);
		uint8_t tex = tiles::tex(id, 0, data);
		float s = tex%16/16.f;
		float t = tex/16/16.f;
		int light = tiles::render_type[id] == RenderType::WIRE ? LIGHT_WIRE15 : LIGHT_TOP;
		vec3 ta(s,        t,        LIGHT_VAL(light));
		vec3 tb(s,        t+1/16.f, LIGHT_VAL(light));
		vec3 tc(s+1/16.f, t+1/16.f, LIGHT_VAL(light));
		vec3 td(s+1/16.f, t,        LIGHT_VAL(light));
		push_quad(verts, a, ta, b, tb, c, tc, d, td);
		glBindBuffer(GL_ARRAY_BUFFER, handitem_vb);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float)*verts.size(), verts.data());
		glDrawArrays(GL_TRIANGLES, 0, verts.size()/6);
		break;
	}
	}
	glEnable(GL_DEPTH_TEST);
}
static void draw_face_basic(float x0, float y0, float z0, float dx, float dy, float dz, int f, int tex, int light, float ds=1.f, float dt=1.f, bool spin = false, float ss = 0.f, float st = 0.f) {
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
	switch (f) {
		case 0:
			a = vec3(x0   , y0   , z0   );
			b = vec3(x0   , y0   , z0+dz);
			c = vec3(x0+dx, y0   , z0+dz);
			d = vec3(x0+dx, y0   , z0   );
			break;
		case 1:
			a = vec3(x0   , y0+dy, z0   );
			b = vec3(x0   , y0+dy, z0+dz);
			c = vec3(x0+dx, y0+dy, z0+dz);
			d = vec3(x0+dx, y0+dy, z0   );
			break;
		case 2:
			a = vec3(x0+dx, y0+dy, z0   );
			b = vec3(x0+dx, y0   , z0   );
			c = vec3(x0   , y0   , z0   );
			d = vec3(x0   , y0+dy, z0   );
			break;
		case 3:
			a = vec3(x0   , y0+dy, z0+dz);
			b = vec3(x0   , y0   , z0+dz);
			c = vec3(x0+dx, y0   , z0+dz);
			d = vec3(x0+dx, y0+dy, z0+dz);
			break;
		case 4:
			a = vec3(x0   , y0+dy, z0   );
			b = vec3(x0   , y0   , z0   );
			c = vec3(x0   , y0   , z0+dz);
			d = vec3(x0   , y0+dy, z0+dz);
			break;
		case 5:
			a = vec3(x0+dx, y0+dy, z0+dz);
			b = vec3(x0+dx, y0   , z0+dz);
			c = vec3(x0+dx, y0   , z0   );
			d = vec3(x0+dx, y0+dy, z0   );
			break;
	}
	vec3 ta,tb,tc,td;
	float s = tex%16/16.f;
	float t = tex/16/16.f;
	ta = vec3(s+ss/16.f        , t+st/16.f        , LIGHT_VAL(light));
	tb = vec3(s+ss/16.f        , t+st/16.f+dt/16.f, LIGHT_VAL(light));
	tc = vec3(s+ss/16.f+ds/16.f, t+st/16.f+dt/16.f, LIGHT_VAL(light));
	td = vec3(s+ss/16.f+ds/16.f, t+st/16.f        , LIGHT_VAL(light));
	if (spin) {
		td = std::exchange(ta, std::exchange(tb, std::exchange(tc, td)));
	}
	if (f != 0) {
		push_quad(RenderChunk::data, a, ta, b, tb, c, tc, d, td);
	} else {
		push_quad(RenderChunk::data, a, ta, d, tb, c, tc, b, td);
	}
}
static void draw_face(int x, int y, int z, int f, int tex, int light) {
	draw_face_basic(x, y, z, 1.f, 1.f, 1.f, f, tex, light);
}
void RenderLevel::draw_block(Level *level, uint8_t id, int x, int y, int z, int data)
{
	switch (tiles::render_type[id]) {
	case RenderType::AIR:
		break;
	case RenderType::CUBE:
		if (!tiles::is_opaque[level->get_tile_id(x, y-1, z)])
			draw_face(x, y, z, 0, tiles::tex(id, 0, data), LIGHT_BOTTOM);
		if (!tiles::is_opaque[level->get_tile_id(x, y+1, z)])
			draw_face(x, y, z, 1, tiles::tex(id, 1, data), LIGHT_TOP);
		if (!tiles::is_opaque[level->get_tile_id(x, y, z-1)])
			draw_face(x, y, z, 2, tiles::tex(id, 2, data), LIGHT_SIDEZ);
		if (!tiles::is_opaque[level->get_tile_id(x, y, z+1)])
			draw_face(x, y, z, 3, tiles::tex(id, 3, data), LIGHT_SIDEZ);
		if (!tiles::is_opaque[level->get_tile_id(x-1, y, z)])
			draw_face(x, y, z, 4, tiles::tex(id, 4, data), LIGHT_SIDEX);
		if (!tiles::is_opaque[level->get_tile_id(x+1, y, z)])
			draw_face(x, y, z, 5, tiles::tex(id, 5, data), LIGHT_SIDEX);
		break;
	case RenderType::PLANT: {
		int tex = tiles::tex(id, 0, data);
		float s = tex%16/16.f;
		float t = tex/16/16.f;
		vec3 ta(s       , t       , LIGHT_VAL(LIGHT_TOP));
		vec3 tb(s       , t+1/16.f, LIGHT_VAL(LIGHT_TOP));
		vec3 tc(s+1/16.f, t+1/16.f, LIGHT_VAL(LIGHT_TOP));
		vec3 td(s+1/16.f, t       , LIGHT_VAL(LIGHT_TOP));
		float p = .05f;
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
			draw_face(x, y, z, 0, tiles::tex(id, 0, data), LIGHT_BOTTOM);
		draw_face_basic(x, y-.5f, z, 1.f, 1.f, 1.f, 1, tiles::tex(id, 1, data), LIGHT_TOP);
		if (!tiles::is_opaque[level->get_tile_id(x, y, z-1)])
			draw_face_basic(x, y, z, 1.f, .5f, 1.f, 2, tiles::tex(id, 2, data), LIGHT_SIDEZ, 1.f, .5f);
		if (!tiles::is_opaque[level->get_tile_id(x, y, z+1)])
			draw_face_basic(x, y, z, 1.f, .5f, 1.f, 3, tiles::tex(id, 3, data), LIGHT_SIDEZ, 1.f, .5f);
		if (!tiles::is_opaque[level->get_tile_id(x-1, y, z)])
			draw_face_basic(x, y, z, 1.f, .5f, 1.f, 4, tiles::tex(id, 4, data), LIGHT_SIDEX, 1.f, .5f);
		if (!tiles::is_opaque[level->get_tile_id(x+1, y, z)])
			draw_face_basic(x, y, z, 1.f, .5f, 1.f, 5, tiles::tex(id, 5, data), LIGHT_SIDEX, 1.f, .5f);
		break;
	case RenderType::WIRE: {
		bool pinched = tiles::is_opaque[level->get_tile_id(x, y+1, z)];
		bool mxo = tiles::is_opaque[level->get_tile_id(x-1, y, z)];
		bool pxo = tiles::is_opaque[level->get_tile_id(x+1, y, z)];
		bool mzo = tiles::is_opaque[level->get_tile_id(x, y, z-1)];
		bool pzo = tiles::is_opaque[level->get_tile_id(x, y, z+1)];
		bool mx = tiles::is_power_source[level->get_tile_id(x-1, y, z)]
			|| (!mxo     && tiles::is_power_source[level->get_tile_id(x-1, y-1, z)])
			|| (!pinched && tiles::is_power_source[level->get_tile_id(x-1, y+1, z)]);
		bool px = tiles::is_power_source[level->get_tile_id(x+1, y, z)]
			|| (!pxo     && tiles::is_power_source[level->get_tile_id(x+1, y-1, z)])
			|| (!pinched && tiles::is_power_source[level->get_tile_id(x+1, y+1, z)]);
		bool mz = tiles::is_power_source[level->get_tile_id(x, y, z-1)]
			|| (!mzo     && tiles::is_power_source[level->get_tile_id(x, y-1, z-1)])
			|| (!pinched && tiles::is_power_source[level->get_tile_id(x, y+1, z-1)]);
		bool pz = tiles::is_power_source[level->get_tile_id(x, y, z+1)]
			|| (!pzo     && tiles::is_power_source[level->get_tile_id(x, y-1, z+1)])
			|| (!pinched && tiles::is_power_source[level->get_tile_id(x, y+1, z+1)]);
		int tex = tiles::tex(id, 1, data);
		bool straight = false;
		bool spin = false;
		if ((mx || px) && !mz && !pz) {
			straight = true;
		} else if ((mz || pz) && !mx && !px) {
			straight = true;
			spin = true;
		}
		float sx = .0f, sz = .0f;
		float dx = 1.f, dz = 1.f;
		if (!straight && (mx || px || mz || pz)) {
			// clip T- and L-intersections
			if (!mx) {
				dx -= .3125f;
				sx += .3125f;
			}
			if (!px)
				dx -= .3125f;
			if (!mz) {
				dz -= .3125f;
				sz += .3125f;
			}
			if (!pz)
				dz -= .3125f;
		}
		draw_face_basic(x+sx, y-.9375f, z+sz, dx, 1.f, dz, 1, tex + (int)straight, LIGHT_WIRE0+level->get_tile_meta(x, y, z), dx, dz, spin, sx, sz);
		if (!pinched) {
			if (mxo && level->get_tile_id(x-1, y+1, z) == 55)
				draw_face_basic(x-.9375f, y, z, 1.f, 1.f, 1.f, 5, tex+1, LIGHT_SIDEX, 1.f, 1.f, true);
			if (pxo && level->get_tile_id(x+1, y+1, z) == 55)
				draw_face_basic(x+.9375f, y, z, 1.f, 1.f, 1.f, 4, tex+1, LIGHT_SIDEX, 1.f, 1.f, true);
			if (mzo && level->get_tile_id(x, y+1, z-1) == 55)
				draw_face_basic(x, y, z-.9375f, 1.f, 1.f, 1.f, 3, tex+1, LIGHT_SIDEZ, 1.f, 1.f, true);
			if (pzo && level->get_tile_id(x, y+1, z+1) == 55)
				draw_face_basic(x, y, z+.9375f, 1.f, 1.f, 1.f, 2, tex+1, LIGHT_SIDEZ, 1.f, 1.f, true);
		}
		break;
	}
	case RenderType::TORCH:
		vec3 svec;
		switch (data) {
			default: svec = vec3(.0f, .0f, .0f); break;
			case 1: svec = vec3(-.3125f, .0f, .0f); break;
			case 2: svec = vec3(.3125f, .0f, .0f); break;
			case 3: svec = vec3(.0f, .0f, -.3125f); break;
			case 4: svec = vec3(.0f, .0f, .3125f); break;
		}
		{
			int tex = tiles::tex(id, 1, data);
			float s = tex%16/16.f;
			float t = tex/16/16.f;
			vec3 ta(s+7/256.f, t+6/256.f, LIGHT_VAL(LIGHT_TOP));
			vec3 tb(s+7/256.f, t+8/256.f, LIGHT_VAL(LIGHT_TOP));
			vec3 tc(s+9/256.f, t+8/256.f, LIGHT_VAL(LIGHT_TOP));
			vec3 td(s+9/256.f, t+6/256.f, LIGHT_VAL(LIGHT_TOP));
			vec3 a = vec3(x+7/16.f, y+10/16.f, z+7/16.f) + svec;
			vec3 b = vec3(x+7/16.f, y+10/16.f, z+9/16.f) + svec;
			vec3 c = vec3(x+9/16.f, y+10/16.f, z+9/16.f) + svec;
			vec3 d = vec3(x+9/16.f, y+10/16.f, z+7/16.f) + svec;
			push_quad(RenderChunk::data, a, ta, b, tb, c, tc, d, td);
		}
		draw_face_basic(x+svec.x, y, z+svec.z+.4375f, 1.f, 1.f, 1.f, 2, tiles::tex(id, 2, data), LIGHT_SIDEZ);
		draw_face_basic(x+svec.x, y, z+svec.z-.4375f, 1.f, 1.f, 1.f, 3, tiles::tex(id, 3, data), LIGHT_SIDEZ);
		draw_face_basic(x+svec.x+.4375f, y, z+svec.z, 1.f, 1.f, 1.f, 4, tiles::tex(id, 4, data), LIGHT_SIDEX);
		draw_face_basic(x+svec.x-.4375f, y, z+svec.z, 1.f, 1.f, 1.f, 5, tiles::tex(id, 5, data), LIGHT_SIDEX);
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
		glBindAttribLocation(terrain_prog, TERRAIN_I_POSITION, "i_position");
		glBindAttribLocation(terrain_prog, TERRAIN_I_TEXCOORD, "i_texcoord");
		glBindAttribLocation(terrain_prog, TERRAIN_I_LIGHT, "i_light");
		link_program(terrain_prog, vs, fs);
		glDeleteShader(vs);
		glDeleteShader(fs);
		terrain_u_viewproj = glGetUniformLocation(terrain_prog, "u_viewproj");
		terrain_u_tex = glGetUniformLocation(terrain_prog, "u_tex");
		terrain_u_lighttex = glGetUniformLocation(terrain_prog, "u_lighttex");
	}
	vs = load_shader(GL_VERTEX_SHADER, "assets/flat.vert");
	fs = load_shader(GL_FRAGMENT_SHADER, "assets/flat.frag");
	if (vs && fs) {
		flat_prog = create_program(vs, fs);
		glBindAttribLocation(flat_prog, FLAT_I_POSITION, "i_position");
		glBindAttribLocation(flat_prog, FLAT_I_COLOR, "i_color");
		link_program(flat_prog, vs, fs);
		glDeleteShader(vs);
		glDeleteShader(fs);
		flat_u_viewproj = glGetUniformLocation(terrain_prog, "u_viewproj");
	}
	return !!terrain_prog && !!flat_prog;
}
bool load_textures() {
	glUseProgram(terrain_prog);
	glUniform1i(terrain_u_tex, TERRAIN_TEXTURE_TERRAIN);
	glGenTextures(1, &terrain_tex);
	glActiveTexture(GL_TEXTURE0 + TERRAIN_TEXTURE_TERRAIN);
	glBindTexture(GL_TEXTURE_2D, terrain_tex);
	if (!load_png("assets/terrain.png"))
	//if (!load_png("terrain.png"))
		return false;
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glUniform1i(terrain_u_lighttex, TERRAIN_TEXTURE_LIGHT);
	glGenTextures(1, &terrain_lighttex);
	glActiveTexture(GL_TEXTURE0 + TERRAIN_TEXTURE_LIGHT);
	// using 2D texture for GLES support
	glBindTexture(GL_TEXTURE_2D, terrain_lighttex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, LIGHT_MAX, 1, 0, GL_RGBA, GL_FLOAT, light_values);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	return true;
}
}
