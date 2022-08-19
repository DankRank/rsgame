// SPDX-License-Identifier: Apache-2.0 OR MIT
#ifndef RSGAME_LEVEL
#define RSGAME_LEVEL
#include "tile.hh"
#include "render.hh"
namespace rsgame {
	struct Level {
		Level();
		uint8_t get_tile_id(int x, int y, int z);
		uint8_t get_tile_meta(int x, int y, int z);
		void set_tile(int x, int y, int z, uint8_t id, uint8_t metadata);
		int xsize, zsize;
		int zbits;
	private:
		std::vector<uint8_t> buf;
		uint8_t *blocks;
		uint8_t *data;
	};
}
#endif
