// SPDX-License-Identifier: Apache-2.0 OR MIT
#include "common.hh"
#include "raycast.hh"
#include "tile.hh"
namespace rsgame {
/* Raycasting 101
 * We cast three rays in total, one for each axis. WLOG consider the x axis.
 * First we figure out the point where the look ray intersects a yz plane at
 * the nearest integer x. The point is called (x, y, z) and the distance to it
 * is d. Then we figure out the per-axis distance between two adjacent yz plane
 * intersection along the look ray (dx, dy, dz). Then we simply iterate through
 * (x+k*dx, y+k*dy, z+k*dz) until we either find a collision or reach a limit.
 * Note that x is an integer, and dx is either -1 or 1.
 *
 * dx and x are trivially computed, however there's a caveat: when you look +x,
 * the intersection is on the -x side of the block, and vice versa, meanwhile
 * the block coordinates are always on the -x side of the block. This creates
 * an off-by one situation, where if you look -x, the block coordinate is one
 * less than the intersection. We just always pick -x, and take it into account
 * when calculating d.
 *
 * For the rest of the values, we must compute ic, the inverse of cosine of
 * angle between the look ray and the x axis. Because the look ray is a unit
 * vector, it's just a reciprocal of the length of the x component. The way to
 * think about ic, is that is scales the look vector so that the x component is
 * equal to dx. From there you can derive dy = y/|x|, dz = z/|x|. In 2D version
 * of this algorithm (wolf3d et al.) you'll often see dy = tan(th) being used,
 * because in 2D, sin(th) = y.
 *
 * To get d, take the distance between the x player position and the x of the
 * first intersection, and multiply it by ic (plays the role of hyp/adj ratio).
 * y and z are then dervied from it.
 *
 * Repeat this for y and z axes and pick the shortest ray.
 */
bool raycast(Level *level, vec3 pos, vec3 look, double maxd, RaycastResult &out) {
	bool found = false;
	// along x axis
	if (look.x != 0) {
		int dx = look.x >= 0 ? 1 : -1;
		int x = (int)floorf(pos.x) + dx;
		float ic = 1/fabsf(look.x);
		float d = dx == 1 ? fabsf(x-pos.x)*ic : fabsf(x+1-pos.x)*ic;
		float y = pos.y + d*look.y;
		float z = pos.z + d*look.z;
		float dy = ic*look.y;
		float dz = ic*look.z;
		while (d < maxd) {
			if (tiles::render_type[level->get_tile_id(x, (int)floorf(y), (int)floorf(z))] != RenderType::AIR) {
				out.x = x;
				out.y = (int)floorf(y);
				out.z = (int)floorf(z);
				out.f = dx == 1 ? 4 : 5;
				maxd = d;
				found = true;
				break;
			}
			x += dx; y += dy; z += dz; d += ic;
		}
	}
	// along y axis
	if (look.y != 0) {
		int dy = look.y >= 0 ? 1 : -1;
		int y = (int)floorf(pos.y) + dy;
		float ic = 1/fabsf(look.y);
		float d = dy == 1 ? fabsf(y-pos.y)*ic : fabsf(y+1-pos.y)*ic;
		float x = pos.x + d*look.x;
		float z = pos.z + d*look.z;
		float dx = ic*look.x;
		float dz = ic*look.z;
		while (d < maxd) {
			if (tiles::render_type[level->get_tile_id((int)floorf(x), y, (int)floorf(z))] != RenderType::AIR) {
				out.x = (int)floorf(x);
				out.y = y;
				out.z = (int)floorf(z);
				out.f = dy == 1 ? 0 : 1;
				maxd = d;
				found = true;
				break;
			}
			x += dx; y += dy; z += dz; d += ic;
		}
	}
	// along z axis
	if (look.z != 0) {
		int dz = look.z >= 0 ? 1 : -1;
		int z = (int)floorf(pos.z) + dz;
		float ic = 1/fabsf(look.z);
		float d = dz == 1 ? fabsf(z-pos.z)*ic : fabsf(z+1-pos.z)*ic;
		float x = pos.x + d*look.x;
		float y = pos.y + d*look.y;
		float dx = ic*look.x;
		float dy = ic*look.y;
		while (d < maxd) {
			if (tiles::render_type[level->get_tile_id((int)floorf(x), (int)floorf(y), z)] != RenderType::AIR) {
				out.x = (int)floorf(x);
				out.y = (int)floorf(y);
				out.z = z;
				out.f = dz == 1 ? 2 : 3;
				maxd = d;
				found = true;
				break;
			}
			x += dx; y += dy; z += dz; d += ic;
		}
	}
	return found;
}

}
