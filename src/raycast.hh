// SPDX-License-Identifier: Apache-2.0 OR MIT
#ifndef RSGAME_RAYCAST
#define RSGAME_RAYCAST
#include "level.hh"
namespace rsgame {
	struct Level;
	struct RaycastResult {
		int x, y, z, f;
	};
	bool raycast(Level *level, vec3 pos, vec3 look, double maxd, RaycastResult &out);
}
#endif
