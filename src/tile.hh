// SPDX-License-Identifier: Apache-2.0 OR MIT
#ifndef RSGAME_TILE
#define RSGAME_TILE
namespace rsgame {
	enum class RenderType : uint8_t {
		AIR = 0,
		CUBE,
		PLANT,
		SLAB,
	};
	namespace tiles {
		extern RenderType render_type[256];
		extern bool is_opaque[256];
		uint8_t tex(uint8_t id, int face, int data);
		void init();
		void dump();
	}
}
#endif
