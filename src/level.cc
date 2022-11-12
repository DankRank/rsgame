// SPDX-License-Identifier: Apache-2.0 OR MIT
#include "common.hh"
#include "level.hh"
namespace rsgame {
Level::Level() {
	xsize = 512;
	zsize = 512;
	zbits = 9;
	buf.resize(xsize*zsize*128*3/2);
	blocks = buf.data();
	data = buf.data() + xsize*zsize*128;
	set_tile(0, 0, 0, 0, 0);
	set_tile(0, 0, 1, 1, 0);
	set_tile(1, 0, 1, 2, 0);
	set_tile(1, 0, 0, 3, 0);
	set_tile(0, 1, 0, 4, 0);
	set_tile(0, 1, 1, 5, 0);
	set_tile(1, 1, 1, 1, 0);
	set_tile(1, 1, 0, 1, 0);

	set_tile(10, 0, 0, 1, 0);
	set_tile(10, 1, 0, 2, 0);
	set_tile(10, 2, 0, 3, 0);
	set_tile(10, 3, 0, 4, 0);
	set_tile(10, 4, 0, 5, 0);
	set_tile(10, 5, 0, 7, 0);
	set_tile(10, 6, 0, 12, 0);
	set_tile(10, 7, 0, 13, 0);
	set_tile(10, 8, 0, 14, 0);
	set_tile(10, 9, 0, 15, 0);
	set_tile(10, 10, 0, 16, 0);
	set_tile(10, 11, 0, 17, 0);
	set_tile(10, 12, 0, 18, 0);
	set_tile(10, 13, 0, 19, 0);
	set_tile(10, 14, 0, 20, 0);
	set_tile(10, 15, 0, 41, 0);
	set_tile(10, 16, 0, 42, 0);
	set_tile(10, 17, 0, 43, 0);
	set_tile(10, 18, 0, 44, 0);
	set_tile(10, 19, 0, 45, 0);
	set_tile(10, 20, 0, 46, 0);
	set_tile(10, 21, 0, 47, 0);
	set_tile(10, 22, 0, 48, 0);
	set_tile(10, 23, 0, 49, 0);

	set_tile(5, 0, 5, 17, 0);
	set_tile(5, 2, 5, 2, 0);
	set_tile(5, 4, 5, 43, 0);
	set_tile(5, 6, 5, 46, 0);
	set_tile(5, 8, 5, 47, 0);
	set_tile(5, 10, 5, 44, 0);

	set_tile(7, 0, 0, 3, 0);
	set_tile(7, 0, 1, 3, 0);
	set_tile(7, 0, 2, 3, 0);
	set_tile(7, 0, 3, 3, 0);
	set_tile(7, 0, 4, 3, 0);

	set_tile(7, 1, 0, 6, 0);
	set_tile(7, 1, 1, 37, 0);
	set_tile(7, 1, 2, 38, 0);
	set_tile(7, 1, 3, 39, 0);
	set_tile(7, 1, 4, 40, 0);

	set_tile(10, 0, 10, 35, 0);
	set_tile(10, 1, 10, 35, 1);
	set_tile(10, 2, 10, 35, 2);
	set_tile(10, 3, 10, 35, 3);
	set_tile(10, 4, 10, 35, 4);
	set_tile(10, 5, 10, 35, 5);
	set_tile(10, 6, 10, 35, 6);
	set_tile(10, 7, 10, 35, 7);
	set_tile(10, 8, 10, 35, 8);
	set_tile(10, 9, 10, 35, 9);
	set_tile(10, 10, 10, 35, 10);
	set_tile(10, 11, 10, 35, 11);
	set_tile(10, 12, 10, 35, 12);
	set_tile(10, 13, 10, 35, 13);
	set_tile(10, 14, 10, 35, 14);
	set_tile(10, 15, 10, 35, 15);

	for (int i = 0; i <= 15; i++)
		set_tile(3, 0, i, 35, 1);
	set_tile(3, 1, 0, 50, 5);
	set_tile(3, 1, 1, 50, 1);
	set_tile(3, 1, 2, 50, 2);
	set_tile(3, 1, 3, 50, 3);
	set_tile(3, 1, 4, 50, 4);
	set_tile(3, 1, 5, 75, 5);
	set_tile(3, 1, 6, 75, 1);
	set_tile(3, 1, 7, 75, 2);
	set_tile(3, 1, 8, 75, 3);
	set_tile(3, 1, 9, 75, 4);
	set_tile(3, 1, 10, 76, 5);
	set_tile(3, 1, 11, 76, 1);
	set_tile(3, 1, 12, 76, 2);
	set_tile(3, 1, 13, 76, 3);
	set_tile(3, 1, 14, 76, 4);
	set_tile(3, 1, 15, 55, 0);
	for (int i = 0; i < 16; i++)
		for (int j = 0; j < 16; j++)
			set_tile(32+i, 0, 32+j, 35, 1);
}
uint8_t Level::get_tile_id(int x, int y, int z) {
	if (x < 0 || x > xsize-1 || z < 0 || z > zsize-1 || y < 0 || y > 127)
		return 0;
	return blocks[x << (zbits+7) | z << 7 | y];
}
uint8_t Level::get_tile_meta(int x, int y, int z) {
	if (x < 0 || x > xsize-1 || z < 0 || z > zsize-1 || y < 0 || y > 127)
		return 0;
	return data[x << (zbits+6) | z << 6 | y >> 1]>>(y<<2 & 4) & 15;
}
void Level::set_tile(int x, int y, int z, uint8_t id, uint8_t metadata) {
	if (x < 0 || x > xsize-1 || z < 0 || z > zsize-1 || y < 0 || y > 127)
		return;
	int idx = x << (zbits+7) | z << 7 | y;
	blocks[idx] = id;
	uint8_t &meta = data[idx>>1];
	if (y&1)
		meta = meta&0x0F | metadata<<4;
	else
		meta = meta&0xF0 | metadata&15;
}
/* Scheduled update system
 * Up to a 1000 scheduled updates are processed every tick.
 * The updates are ordered by their target tick and by
 * their insertion order. If an update for a given block has
 * been already scheduled, any futher schedules are ignored.
 * The block id is recorded in the update, and the update
 * is discarded if the block changes in the meantime.
 */
void Level::on_tick() {
	int updates_processed = 0;
	while (!scheduled_updates.empty() && updates_processed < 1000) {
		auto it = scheduled_updates.begin();
		if (it->target_tick > tick)
			break;
		auto u = *it;
		scheduled_tiles.erase(*it);
		scheduled_updates.erase(it);
		if (u.id == get_tile_id(u.x, u.y, u.z))
			on_block_scheduled_update(u.x, u.y, u.z, u.id);
		updates_processed++;
	}
	tick++;
}
void Level::schedule_update(int x, int y, int z, long when) {
	uint8_t id = get_tile_id(x, y, z);
	ScheduledUpdate u;
	u.x = x, u.y = y, u.z = z, u.id = id;
	if (scheduled_tiles.find(u) == scheduled_tiles.end()) {
		u.target_tick = when;
		u.insertion_order = scheduled_update_counter++;
		scheduled_tiles.insert(u);
		scheduled_updates.insert(u);
	}
}
bool Level::powers_weakly(int x, int y, int z, int f) {
	uint8_t id = get_tile_id(x, y, z);
	if (id != 55 && id != 76)
		return false;
	if (id == 76) {
		uint8_t fdata;
		uint8_t data = get_tile_meta(x, y, z);
		switch (f) {
			case 2: fdata = 4; break;
			case 3: fdata = 3; break;
			case 4: fdata = 2; break;
			case 5: fdata = 1; break;
			default: fdata = 5; break;
		}
		if (fdata == data)
			return false;
		return fdata != data;
	} else {
		if (in_wire_propagation)
			return false;
		uint8_t data = get_tile_meta(x, y, z);
		if (data == 0)
			return false;
		if (f == 1)
			return true;
		bool pinched = tiles::is_opaque[get_tile_id(x, y+1, z)];
		bool mxo = tiles::is_opaque[get_tile_id(x-1, y, z)];
		bool pxo = tiles::is_opaque[get_tile_id(x+1, y, z)];
		bool mzo = tiles::is_opaque[get_tile_id(x, y, z-1)];
		bool pzo = tiles::is_opaque[get_tile_id(x, y, z+1)];
		bool mx = tiles::is_power_source[get_tile_id(x-1, y, z)]
			|| (!mxo     && tiles::is_power_source[get_tile_id(x-1, y-1, z)])
			|| (!pinched && tiles::is_power_source[get_tile_id(x-1, y+1, z)]);
		bool px = tiles::is_power_source[get_tile_id(x+1, y, z)]
			|| (!pxo     && tiles::is_power_source[get_tile_id(x+1, y-1, z)])
			|| (!pinched && tiles::is_power_source[get_tile_id(x+1, y+1, z)]);
		bool mz = tiles::is_power_source[get_tile_id(x, y, z-1)]
			|| (!mzo     && tiles::is_power_source[get_tile_id(x, y-1, z-1)])
			|| (!pinched && tiles::is_power_source[get_tile_id(x, y+1, z-1)]);
		bool pz = tiles::is_power_source[get_tile_id(x, y, z+1)]
			|| (!pzo     && tiles::is_power_source[get_tile_id(x, y-1, z+1)])
			|| (!pinched && tiles::is_power_source[get_tile_id(x, y+1, z+1)]);
		switch (f) {
			case 2: return mz && !mx && !px;
			case 3: return pz && !mx && !px;
			case 4: return mx && !mz && !pz;
			case 5: return px && !mz && !pz;
		}
		return false;
	}
}
bool Level::powers_strongly(int x, int y, int z, int f) {
	uint8_t id = get_tile_id(x, y, z);
	if (id != 55 && id != 76)
		return false;
	if (id == 76) {
		return f == 0;
	} else {
		return powers_weakly(x, y, z, f);
	}
}
bool Level::powered_strongly_from(int x, int y, int z, int f) {
	if (tiles::is_opaque[get_tile_id(x, y, z)]) {
		return
			powers_strongly(x, y-1, z, 0) ||
			powers_strongly(x, y+1, z, 1) ||
			powers_strongly(x, y, z-1, 2) ||
			powers_strongly(x, y, z+1, 3) ||
			powers_strongly(x-1, y, z, 4) ||
			powers_strongly(x+1, y, z, 5);
	} else {
		return powers_weakly(x, y, z, f);
	}
}
bool Level::powered_strongly(int x, int y, int z) {
	return
		powered_strongly_from(x, y-1, z, 0) ||
		powered_strongly_from(x, y+1, z, 1) ||
		powered_strongly_from(x, y, z-1, 2) ||
		powered_strongly_from(x, y, z+1, 3) ||
		powered_strongly_from(x-1, y, z, 4) ||
		powered_strongly_from(x+1, y, z, 5);
}
bool Level::powered_weakly(int x, int y, int z) {
	return
		powers_weakly(x, y-1, z, 0) ||
		powers_weakly(x, y+1, z, 1) ||
		powers_weakly(x, y, z-1, 2) ||
		powers_weakly(x, y, z+1, 3) ||
		powers_weakly(x-1, y, z, 4) ||
		powers_weakly(x+1, y, z, 5);
}
void Level::on_block_scheduled_update(int x, int y, int z, uint8_t id) {
	if (id == 75 || id == 76) {
		uint8_t data = get_tile_meta(x, y, z);
		bool is_powered;
		switch (data) {
			case 1: is_powered = powered_strongly_from(x-1, y, z, 4); break;
			case 2: is_powered = powered_strongly_from(x+1, y, z, 5); break;
			case 3: is_powered = powered_strongly_from(x, y, z-1, 2); break;
			case 4: is_powered = powered_strongly_from(x, y, z+1, 3); break;
			default: is_powered = powered_strongly_from(x, y-1, z, 0); break;
		}
		if (id == 75) {
			if (!is_powered) {
				set_tile(x, y, z, 76, data);
				on_block_remove(x, y, z, 75);
				on_block_add(x, y, z, 76);
			}
		} else {
			if (is_powered) {
				set_tile(x, y, z, 75, data);
				on_block_remove(x, y, z, 76);
				on_block_add(x, y, z, 75);
			}
		}
	}
}
void Level::on_block_update(int x, int y, int z) {
	uint8_t id = get_tile_id(x, y, z);
	if (id == 50 || id == 75 || id == 76) {
		bool needs_removal;
		switch (get_tile_meta(x, y, z)) {
			case 1: needs_removal = !tiles::is_opaque[get_tile_id(x-1, y, z)]; break;
			case 2: needs_removal = !tiles::is_opaque[get_tile_id(x+1, y, z)]; break;
			case 3: needs_removal = !tiles::is_opaque[get_tile_id(x, y, z-1)]; break;
			case 4: needs_removal = !tiles::is_opaque[get_tile_id(x, y, z+1)]; break;
			default: needs_removal = !tiles::is_opaque[get_tile_id(x, y-1, z)]; break;
		}
		if (needs_removal) {
			set_tile(x, y, z, 0, 0);
			on_block_remove(x, y, z, id);
		} else {
			schedule_update(x, y, z, tick+2);
		}
	} else if (id == 55) {
		if (!tiles::is_opaque[get_tile_id(x, y-1, z)]) {
			set_tile(x, y, z, 0, 0);
			on_block_remove(x, y, z, id);
		} else {
			wire_propagation_start(x, y, z);
		}
	}
}
void Level::on_block_add(int x, int y, int z, uint8_t id) {
	if (id == 76) {
		update_neighbors(x, y-1, z);
		update_neighbors(x, y+1, z);
		update_neighbors(x-1, y, z);
		update_neighbors(x+1, y, z);
		update_neighbors(x, y, z-1);
		update_neighbors(x, y, z+1);
	} else if (id == 55) {
		wire_propagation_start(x, y, z);
		update_neighbors(x, y+1, z);
		update_neighbors(x, y-1, z);
		update_wire_neighbors(x-1, y, z);
		update_wire_neighbors(x+1, y, z);
		update_wire_neighbors(x, y, z-1);
		update_wire_neighbors(x, y, z+1);
		update_wire_neighbors(x-1, y + (tiles::is_opaque[get_tile_id(x-1, y, z)]), z);
		update_wire_neighbors(x+1, y + (tiles::is_opaque[get_tile_id(x+1, y, z)]), z);
		update_wire_neighbors(x, y + (tiles::is_opaque[get_tile_id(x, y, z-1)]), z-1);
		update_wire_neighbors(x, y + (tiles::is_opaque[get_tile_id(x, y, z+1)]), z+1);
	}
	update_neighbors(x, y, z);
	rl->set_dirty(x, y, z);
}
void Level::on_block_remove(int x, int y, int z, uint8_t id) {
	if (id == 76) {
		update_neighbors(x, y-1, z);
		update_neighbors(x, y+1, z);
		update_neighbors(x-1, y, z);
		update_neighbors(x+1, y, z);
		update_neighbors(x, y, z-1);
		update_neighbors(x, y, z+1);
	} else if (id == 55) {
		update_neighbors(x, y+1, z);
		update_neighbors(x, y-1, z);
		wire_propagation_start(x, y, z);
		update_wire_neighbors(x-1, y, z);
		update_wire_neighbors(x+1, y, z);
		update_wire_neighbors(x, y, z-1);
		update_wire_neighbors(x, y, z+1);
		update_wire_neighbors(x-1, y + (tiles::is_opaque[get_tile_id(x-1, y, z)]), z);
		update_wire_neighbors(x+1, y + (tiles::is_opaque[get_tile_id(x+1, y, z)]), z);
		update_wire_neighbors(x, y + (tiles::is_opaque[get_tile_id(x, y, z-1)]), z-1);
		update_wire_neighbors(x, y + (tiles::is_opaque[get_tile_id(x, y, z+1)]), z+1);
	}
	update_neighbors(x, y, z);
	rl->set_dirty(x, y, z);
}
void Level::update_neighbors(int x, int y, int z) {
	on_block_update(x-1, y, z);
	on_block_update(x+1, y, z);
	on_block_update(x, y-1, z);
	on_block_update(x, y+1, z);
	on_block_update(x, y, z-1);
	on_block_update(x, y, z+1);
}
void Level::update_wire_neighbors(int x, int y, int z) {
	if (get_tile_id(x, y, z) == 55) {
		update_neighbors(x, y, z);
		update_neighbors(x-1, y, z);
		update_neighbors(x+1, y, z);
		update_neighbors(x, y, z-1);
		update_neighbors(x, y, z+1);
		update_neighbors(x, y-1, z);
		update_neighbors(x, y+1, z);
	}
}
void Level::wire_propagation_start(int x, int y, int z) {
	wire_propagation(x, y, z, x, y, z);
	pending_wire_updates_set.clear();
	std::vector<glm::ivec3> updates = std::move(pending_wire_updates);
	for (auto v : updates)
		update_neighbors(v.x, v.y, v.z);
}
void Level::wire_propagation(int x, int y, int z, int sx, int sy, int sz) {
	int old_strength = get_tile_meta(x, y, z);
	int new_strength = 0;
	in_wire_propagation = true;
	bool powered = powered_strongly(x, y, z);
	in_wire_propagation = false;
	if (powered) {
		new_strength = 15;
	} else {
		for (int d = 0; d < 4; d++) {
			int dx = x, dz = z;
			switch (d) {
				case 0: dx--; break;
				case 1: dx++; break;
				case 2: dz--; break;
				case 3: dz++; break;
			}
			if (dx != sx || y != sy || dz != sz)
				new_strength = std::max(new_strength, get_tile_id(dx, y, dz) == 55 ? get_tile_meta(dx, y, dz) : 0);
			if (tiles::is_opaque[get_tile_id(dx, y, dz)]) {
				if (!tiles::is_opaque[get_tile_id(x, y+1, z)])
					if (dx != sx || y+1 != sy || dz != sz)
						new_strength = std::max(new_strength, get_tile_id(dx, y+1, dz) == 55 ? get_tile_meta(dx, y+1, dz) : 0);
			} else {
				if (dx != sx || y-1 != sy || dz != sz)
					new_strength = std::max(new_strength, get_tile_id(dx, y-1, dz) == 55 ? get_tile_meta(dx, y-1, dz) : 0);
			}
		}
		new_strength = std::max(new_strength-1, 0);
	}
	if (old_strength != new_strength) {
		if (get_tile_id(x, y, z) == 55) {
			set_tile(x, y, z, 55, new_strength);
			rl->set_dirty(x, y, z);
		}
		int target = std::max(new_strength-1, 0);
		for (int d = 0; d < 4; d++) {
			int dx = x, dz = z;
			switch (d) {
				case 0: dx--; break;
				case 1: dx++; break;
				case 2: dz--; break;
				case 3: dz++; break;
			}
			int dy = tiles::is_opaque[get_tile_id(dx, y, dz)] ? y+1 : y-1;
			if (get_tile_id(dx, y, dz) == 55)
				if (get_tile_meta(dx, y, dz) != target)
					wire_propagation(dx, y, dz, x, y, z);
			if (get_tile_id(dx, dy, dz) == 55)
				if (get_tile_meta(dx, dy, dz) != target)
					wire_propagation(dx, dy, dz, x, y, z);
		}
		if (old_strength == 0 || new_strength == 0) {
			auto add_update = [this](int x, int y, int z) {
				glm::ivec3 v(x, y, z);
				if (pending_wire_updates_set.find(v) == pending_wire_updates_set.end()) {
					pending_wire_updates_set.insert(v);
					pending_wire_updates.push_back(v);
				}
			};
			add_update(x, y, z);
			add_update(x-1, y, z);
			add_update(x+1, y, z);
			add_update(x, y-1, z);
			add_update(x, y+1, z);
			add_update(x, y, z-1);
			add_update(x, y, z+1);
		}
	}
}
}
