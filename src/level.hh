// SPDX-License-Identifier: Apache-2.0 OR MIT
#ifndef RSGAME_LEVEL
#define RSGAME_LEVEL
#include "tile.hh"
#include "render.hh"
namespace rsgame {
	struct RenderLevel;
	struct ScheduledUpdate {
		int x, y, z;
		uint8_t id;
		long target_tick;
		long insertion_order;
	};
	struct ScheduledUpdateHashSpace {
		constexpr size_t operator()(const ScheduledUpdate &u) const {
			size_t hash = u.x;
			hash ^= u.y + 0x9e3779b9 + (hash << 6) + (hash >> 2);
			hash ^= u.z + 0x9e3779b9 + (hash << 6) + (hash >> 2);
			hash ^= u.id + 0x9e3779b9 + (hash << 6) + (hash >> 2);
			return hash;
		}
	};
	struct ScheduledUpdateEqualSpace {
		constexpr bool operator()(const ScheduledUpdate &lhs, const ScheduledUpdate &rhs) const {
			return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z && lhs.id == rhs.id;
		}
	};
	struct ScheduledUpdateLessTime {
		constexpr bool operator()(const ScheduledUpdate &lhs, const ScheduledUpdate &rhs) const {
			return
				lhs.target_tick < rhs.target_tick || lhs.target_tick == rhs.target_tick && (
				lhs.insertion_order < rhs.insertion_order || lhs.insertion_order == rhs.insertion_order);
		}
	};
	struct Level {
		Level();
		RenderLevel *rl = nullptr;
		uint8_t get_tile_id(int x, int y, int z);
		uint8_t get_tile_meta(int x, int y, int z);
		void set_tile(int x, int y, int z, uint8_t id, uint8_t metadata);
		int xsize, zsize;
		int zbits;
		long tick;
		void on_tick();
	private:
		std::unordered_set<ScheduledUpdate, ScheduledUpdateHashSpace, ScheduledUpdateEqualSpace> scheduled_tiles;
		std::set<ScheduledUpdate, ScheduledUpdateLessTime> scheduled_updates;
		long scheduled_update_counter = 0;
	public:
		void schedule_update(int x, int y, int z, long when);
		bool in_wire_propagation = false;
		bool powers_weakly(int x, int y, int z, int f);
		bool powers_strongly(int x, int y, int z, int f);
		bool powered_strongly_from(int x, int y, int z, int f);
		bool powered_strongly(int x, int y, int z);
		bool powered_weakly(int x, int y, int z);
		void on_block_scheduled_update(int x, int y, int z, uint8_t id);
		void on_block_update(int x, int y, int z);
		void on_block_add(int x, int y, int z, uint8_t id);
		void on_block_remove(int x, int y, int z, uint8_t id);
		void update_neighbors(int x, int y, int z);
		void update_wire_neighbors(int x, int y, int z);
		void wire_propagation_start(int x, int y, int z);
		void wire_propagation(int x, int y, int z, int sx, int sy, int sz);
	private:
		std::vector<uint8_t> buf;
		uint8_t *blocks;
		uint8_t *data;
		std::unordered_set<glm::ivec3> pending_wire_updates_set;
		std::vector<glm::ivec3> pending_wire_updates;
	};
}
#endif
