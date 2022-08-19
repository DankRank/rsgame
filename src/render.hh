// SPDX-License-Identifier: Apache-2.0 OR MIT
#ifndef RSGAME_RENDER
#define RSGAME_RENDER
#include "level.hh"
#include "util.hh"
namespace rsgame {
	struct Level;
	struct RenderChunk {
		static std::vector<float> data;
		int x, y, z;
		bool empty;
		size_t size, cap;
private:
		GLuint va, vb;
public:
		RenderChunk(int x, int y, int z);
		void flip();
		void draw();
		~RenderChunk();
		RenderChunk(const RenderChunk&) =delete;
		RenderChunk &operator=(const RenderChunk&) =delete;
		RenderChunk(RenderChunk&&) =default;
		RenderChunk &operator=(RenderChunk&&) =default;
	};
	struct RenderLevel {
		Level *level;
		std::unordered_map<uint64_t, RenderChunk*> chunks;
		std::unordered_set<RenderChunk*> dirty_chunks;
		void on_load_chunk(int x, int z);
		void on_unload_chunk(int x, int z);
		void set_dirty1(int x, int y, int z);
		void set_dirty(int x, int y, int z);
		void update(Uint64 target = 0);
		void draw();
	private:
		void draw_block(Level *level, uint8_t id, int x, int y, int z, int data);
	public:
		RenderLevel(Level *level);
		~RenderLevel();
		RenderLevel(const RenderLevel&) =delete;
		RenderLevel &operator=(const RenderLevel&) =delete;
	};
	extern GLuint terrain_prog;
	bool load_shaders();
	bool load_textures();
}
#endif
