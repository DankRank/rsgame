// SPDX-License-Identifier: Apache-2.0 OR MIT
#include "common.hh"
#include "tile.hh"
#include "render.hh"
#include "level.hh"
#include "util.hh"
namespace rsgame::tiles {
static const AABB aabb_air     = {{.0f, .0f, .0f}, {.0f, .0f, .0f}};
static const AABB aabb_cube    = {{.0f, .0f, .0f}, {1.f, 1.f, 1.f}};
static const AABB aabb_sapling = {{.1f, .0f, .1f}, {.9f, .8f, .9f}};
static const AABB aabb_flower  = {{.3f, .0f, .3f}, {.7f, .6f, .7f}};
static const AABB aabb_slab    = {{.0f, .0f, .0f}, {1.f, .5f, 1.f}};
const AABB &get_aabb(RenderType rt, uint8_t id) {
	switch (rt) {
		case RenderType::AIR:
			return aabb_air;
		default:
		case RenderType::CUBE:
			return aabb_cube;
		case RenderType::PLANT:
			return id == 6 ? aabb_sapling : aabb_flower;
		case RenderType::SLAB:
			return aabb_slab;
	}
}
RenderType render_type[256] = {RenderType::AIR};
bool is_opaque[256] = {false};
static const uint8_t *texture_data[256] = {nullptr};
uint8_t tex(uint8_t id, int face, int data) {
	const uint8_t *p = texture_data[id];
	if (!p)
		return 0;
	switch (*p++) {
	default:
		return 0;
	case 1:
		return *p;
	case 6:
		return p[face];
	case 16:
		return p[data];
	}
}

static uint8_t tex_buf[105] = {0}, *tex_p = tex_buf;

struct TileBuilder {
	uint8_t id;
	TileBuilder(uint8_t id) :id(id) {
		render_type[id] = RenderType::CUBE;
		is_opaque[id] = true;
	}
	TileBuilder &render_as(RenderType type) {
		render_type[id] = type;
		if (type != RenderType::CUBE)
			is_opaque[id] = false;
		return *this;
	}
private:
	void check_tex(int i) {
		if (tex_p + 1 + i > tex_buf + sizeof(tex_buf)) {
			fprintf(stderr, "tex_buf overrun\n");
			abort();
		}
	}
public:
	TileBuilder &tex(uint8_t tex) {
		check_tex(1);
		texture_data[id] = tex_p;
		*tex_p++ = 1;
		*tex_p++ = tex;
		return *this;
	}
	TileBuilder &tex_grass(uint8_t side, uint8_t top, uint8_t bot) {
		check_tex(6);
		texture_data[id] = tex_p;
		*tex_p++ = 6;
		*tex_p++ = bot;
		*tex_p++ = top;
		*tex_p++ = side;
		*tex_p++ = side;
		*tex_p++ = side;
		*tex_p++ = side;
		return *this;
	}
	TileBuilder &tex_perdata(const uint8_t *texs) {
		check_tex(16);
		texture_data[id] = tex_p;
		*tex_p++ = 16;
		memcpy(tex_p, texs, 16);
		tex_p += 16;
		return *this;
	}
	TileBuilder &transparent() {
		is_opaque[id] = false;
		return *this;
	}
};
static constexpr uint8_t uv(int x, int y) { return y*16 + x; }
void init() {
	uint8_t cloth_tex[16] = {
		uv(0, 4), uv(2, 13), uv(2, 12), uv(2, 11),
		uv(2, 10), uv(2, 9), uv(2, 8), uv(2, 7),
		uv(1, 14), uv(1, 13), uv(1, 12), uv(1, 11),
		uv(1, 10), uv(1, 9), uv(1, 8), uv(1, 7),
	};

	TileBuilder(0).render_as(RenderType::AIR);
	TileBuilder(1).tex(uv(1, 0));
	TileBuilder(2).tex_grass(uv(3, 0), uv(0, 0), uv(2, 0));
	TileBuilder(3).tex(uv(2, 0));
	TileBuilder(4).tex(uv(0, 1));
	TileBuilder(5).tex(uv(4, 0));
	TileBuilder(6).render_as(RenderType::PLANT).tex(uv(15, 0));
	TileBuilder(7).tex(uv(1, 1));
	TileBuilder(12).tex(uv(2, 1));
	TileBuilder(13).tex(uv(3, 1));
	TileBuilder(14).tex(uv(0, 2));
	TileBuilder(15).tex(uv(1, 2));
	TileBuilder(16).tex(uv(2, 2));
	TileBuilder(17).tex_grass(uv(4, 1), uv(5, 1), uv(5, 1));
	TileBuilder(18).transparent().tex(uv(4, 3));
	TileBuilder(19).tex(uv(0, 3));
	TileBuilder(20).transparent().tex(uv(1, 3));
	TileBuilder(35).tex_perdata(cloth_tex);
	TileBuilder(37).render_as(RenderType::PLANT).tex(uv(13, 0));
	TileBuilder(38).render_as(RenderType::PLANT).tex(uv(12, 0));
	TileBuilder(39).render_as(RenderType::PLANT).tex(uv(13, 1));
	TileBuilder(40).render_as(RenderType::PLANT).tex(uv(12, 1));
	TileBuilder(41).tex(uv(7, 1));
	TileBuilder(42).tex(uv(6, 1));
	TileBuilder(43).tex_grass(uv(5, 0), uv(6, 0), uv(6, 0));
	TileBuilder(44).render_as(RenderType::SLAB).tex_grass(uv(5, 0), uv(6, 0), uv(6, 0));
	TileBuilder(45).tex(uv(7, 0));
	TileBuilder(46).tex_grass(uv(8, 0), uv(9, 0), uv(10, 0));
	TileBuilder(47).tex_grass(uv(3, 2), uv(4, 0), uv(4, 0));
	TileBuilder(48).tex(uv(4, 2));
	TileBuilder(49).tex(uv(5, 2));
	//fprintf(stderr, "tex_buf: %d bytes used\n", (int)(tex_p-tex_buf));
}
void dump() {
	printf("{\n");
	for (int i = 0; i < 256; i++) {
		if (render_type[i] == RenderType::AIR)
			continue;
		if (i != 1)
			printf(",");
		printf("\n\t\"%d\": {\n", i);
		const char *rtype;
		switch(render_type[i]) {
			case RenderType::AIR: rtype = "AIR"; break;
			case RenderType::CUBE: rtype = "CUBE"; break;
			case RenderType::PLANT: rtype = "PLANT"; break;
			case RenderType::SLAB: rtype = "SLAB"; break;
		}
		printf("\t\t\"render_type\": \"%s\",\n", rtype);
		printf("\t\t\"is_opaque\": %s,\n", is_opaque[i] ? "true" : "false");
		printf("\t\t\"texture_data\": [");
		if (texture_data[i]) {
			for (int j = 1; j <= texture_data[i][0]; j++) {
				if (j != 1)
					printf(", ");
				printf("%d", texture_data[i][j]);
			}
		}
		printf("]\n");
		printf("\t}");
	}
	printf("\n}\n");
}
}
