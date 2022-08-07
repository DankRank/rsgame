// SPDX-License-Identifier: Apache-2.0 OR MIT
#include "common.hh"
#include "tile.hh"
#include "render.hh"
#include "level.hh"
namespace rsgame {
struct AirTile : Tile {
	AirTile() { is_opaque = false; render_type = RenderType::AIR; }
};
struct OpaqueTile : Tile {
	OpaqueTile() { is_opaque = true; render_type = RenderType::CUBE; }
};
struct TexturedTile : OpaqueTile {
	uint8_t tex;
	TexturedTile(uint8_t tex) :tex(tex) {}
	uint8_t tex_for_side(int f, int data) {
		(void)f;
		(void)data;
		return tex;
	}
};
struct PlantTile : Tile {
	uint8_t tex;
	PlantTile(uint8_t tex) :tex(tex) { is_opaque = false; render_type = RenderType::PLANT; }
	uint8_t tex_for_side(int f, int data) {
		(void)f;
		(void)data;
		return tex;
	}
};
struct GlassTile : TexturedTile {
	GlassTile(uint8_t tex) :TexturedTile(tex) {
		is_opaque = false;
	}
};
struct GrassTile : OpaqueTile {
	uint8_t tex, top, bot;
	GrassTile(uint8_t tex, uint8_t top, uint8_t bot) :tex(tex), top(top), bot(bot) {}
	uint8_t tex_for_side(int f, int data) {
		(void)f;
		(void)data;
		switch (f) {
		case 0:
			return bot;
		case 1:
			return top;
		default:
			return tex;
		}
	}
};
struct SlabTile : GrassTile {
	SlabTile(uint8_t tex, uint8_t top) :GrassTile(tex, top, top) {
		render_type = RenderType::SLAB;
		is_opaque = false;
	}
};
struct ClothTile : OpaqueTile {
	uint8_t tex_for_side(int f, int data) {
		(void)f;
		if (data == 0)
			return 4*16;
		else
			return 14*16 + 2 - (data&7)*16 - (data>>3);
	}
};
Tile *Tile::tiles[256] = {0};
void Tile::init() {
	tiles[0] = new AirTile();
	for (int i = 1; i < 256; i++)
		tiles[i] = tiles[0];
	tiles[1] = new TexturedTile(1);
	tiles[2] = new GrassTile(3, 0, 2);
	tiles[3] = new TexturedTile(2);
	tiles[4] = new TexturedTile(1*16 + 0);
	tiles[5] = new TexturedTile(4);
	tiles[6] = new PlantTile(15);
	tiles[7] = new TexturedTile(1*16 + 1);
	tiles[12] = new TexturedTile(1*16 + 2);
	tiles[13] = new TexturedTile(1*16 + 3);
	tiles[14] = new TexturedTile(2*16 + 0);
	tiles[15] = new TexturedTile(2*16 + 1);
	tiles[16] = new TexturedTile(2*16 + 2);
	tiles[17] = new GrassTile(1*16 + 4, 1*16 + 5, 1*16 + 5);
	tiles[18] = new GlassTile(3*16 + 4);
	tiles[19] = new TexturedTile(3*16 + 0);
	tiles[20] = new GlassTile(3*16 + 1);
	tiles[35] = new ClothTile();
	tiles[37] = new PlantTile(13);
	tiles[38] = new PlantTile(12);
	tiles[39] = new PlantTile(1*16 + 13);
	tiles[40] = new PlantTile(1*16 + 12);
	tiles[41] = new TexturedTile(1*16 + 7);
	tiles[42] = new TexturedTile(1*16 + 6);
	tiles[43] = new GrassTile(5, 6, 6);
	tiles[44] = new SlabTile(5, 6);
	tiles[45] = new TexturedTile(7);
	tiles[46] = new GrassTile(8, 9, 10);
	tiles[47] = new GrassTile(2*16+3, 4, 4);
	tiles[48] = new TexturedTile(2*16 + 4);
	tiles[49] = new TexturedTile(2*16 + 5);
}
}
