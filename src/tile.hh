// SPDX-License-Identifier: Apache-2.0 OR MIT
#ifndef RSGAME_TILE
#define RSGAME_TILE
namespace rsgame {
	struct Level;
	enum class RenderType {
		AIR,
		CUBE,
		PLANT,
		SLAB,
	};
	struct Tile {
		static void init();
		static Tile *tiles[256];
		bool is_opaque;
		virtual uint8_t tex_for_side(int f, int data) { (void)f; (void)data; return 0; };
		RenderType render_type;
	};
}
#endif
