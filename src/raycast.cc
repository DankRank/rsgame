// SPDX-License-Identifier: Apache-2.0 OR MIT
#include "common.hh"
#include "raycast.hh"
#include "tile.hh"
namespace rsgame {
bool raycast_in_physics = false;
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
static bool raycast_collide(vec3 pos, vec3 look, double &maxd, RaycastResult &out, int x, int y, int z, int f, uint8_t id, double d);
bool raycast(Level *level, vec3 pos, vec3 look, double maxd, RaycastResult &out)
{
	bool found = false;
	// check initial block
	{
		int xx = (int)floorf(pos.x), yy = (int)floorf(pos.y), zz = (int)floorf(pos.z);
		uint8_t id = level->get_tile_id(xx, yy, zz);
		RenderType rt = tiles::render_type[id];
		if (rt != RenderType::AIR && rt != RenderType::CUBE)
			if (raycast_collide(pos, look, maxd, out, xx, yy, zz, 0, id, 0))
				return true;
	}
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
			int xx = x, yy = (int)floorf(y), zz = (int)floorf(z);
			if (raycast_collide(pos, look, maxd, out, xx, yy, zz, dx == 1 ? 4 : 5, level->get_tile_id(xx, yy, zz), d)) {
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
			int xx = (int)floorf(x), yy = y, zz = (int)floorf(z);
			if (raycast_collide(pos, look, maxd, out, xx, yy, zz, dy == 1 ? 0 : 1, level->get_tile_id(xx, yy, zz), d)) {
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
			int xx = (int)floorf(x), yy = (int)floorf(y), zz = z;
			if (raycast_collide(pos, look, maxd, out, xx, yy, zz, dz == 1 ? 2 : 3, level->get_tile_id(xx, yy, zz), d)) {
				found = true;
				break;
			}
			x += dx; y += dy; z += dz; d += ic;
		}
	}
	return found;
}
/* Preconditions:
 * - look ray collides with the full cube
 * - d < maxd
 * - pos is outside the full cube
 * Combination of these conditions mean that checks against CUBE always
 * succeed. (And AIR always fails regardless of any conditions.
 *
 * For blocks with funny shapes, we do ray-AABB intersection:
 * https://education.siggraph.org/static/HyperGraph/raytrace/rtinter3.htm
 * From the ray's point of view, an AABB consists of 6 planes, 2 for each axis.
 * We can sort these planes by their distance along the ray. Along each axis
 * there will be a near plane and a far plane. We intersect AABB when we have
 * passed all 3 near planes, but haven't passed any of the far planes yet. So
 * we find the farthest near plane (fmin) and the nearest far plane (fmax).
 * If fmax < fmin, there is no intersection. Otherwise, if fmax < 0, the AABB
 * is intersected by the opposite ray, but not by this one, and if fmin < 0,
 * the origin is within the AABB (which, for our purposes means no
 * intersection). Otherwise we have an intersection.
 */
inline static bool raycast_collide(vec3 pos, vec3 look, double &maxd, RaycastResult &out, int x, int y, int z, int f, uint8_t id, double d)
{
	RenderType rt = tiles::render_type[id];
	if (rt == RenderType::AIR)
		return false;
	if (rt == RenderType::CUBE) {
		out.x = x, out.y = y, out.z = z, out.f = f;
		out.aabb = &tiles::get_aabb(rt, id);
		maxd = d;
		return true;
	}
	if (raycast_in_physics && rt == RenderType::PLANT)
		return false;
	const AABB& aabb = tiles::get_aabb(rt, id);
	using std::min;
	using std::max;
	// NOTE: relies on IEEE754 division by zero semantics
	vec3 ilook = 1.f/look;
	vec3 pmin = (vec3(x, y, z)+aabb.min-pos)*ilook;
	vec3 pmax = (vec3(x, y, z)+aabb.max-pos)*ilook;
	float fmin = max({min(pmin.x, pmax.x), min(pmin.y, pmax.y), min(pmin.z, pmax.z)});
	float fmax = min({max(pmin.x, pmax.x), max(pmin.y, pmax.y), max(pmin.z, pmax.z)});
	if (fmin < 0.f || fmax < fmin)
		return false;
	if (fmin >= maxd)
		return true;
	out.x = x, out.y = y, out.z = z;
	// find which plane does the distance come from
	if (fmin == pmin.x) out.f = 4;
	else if (fmin == pmax.x) out.f = 5;
	else if (fmin == pmin.y) out.f = 0;
	else if (fmin == pmax.y) out.f = 1;
	else if (fmin == pmin.z) out.f = 2;
	else /*if (fmin == pmax.z)*/ out.f = 3;
	maxd = fmin;
	out.aabb = &aabb;
	return true;
}

}
