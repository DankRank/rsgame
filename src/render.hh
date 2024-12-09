// SPDX-License-Identifier: Apache-2.0 OR MIT
#ifndef RSGAME_RENDER
#define RSGAME_RENDER
#include "level.hh"
#include "raycast.hh"
#include "glutil.hh"
namespace rsgame {
	struct Level;
	struct RaycastResult;
	extern bool render_ao_enabled;
	struct RenderChunk {
		static std::vector<float> data, aodata;
		int x, y, z;
		size_t size, cap;
private:
		GLuint va, vb;
		bool has_ao;
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
		void set_all_dirty();
		void set_dirty1(int x, int y, int z);
		void set_dirty(int x, int y, int z);
		void update(Uint64 target = 0);
		void draw(const Frustum &viewfrustum);
	private:
		void draw_block(Level *level, uint8_t id, int x, int y, int z, int data);
	public:
		RenderLevel(Level *level);
		~RenderLevel();
		RenderLevel(const RenderLevel&) =delete;
		RenderLevel &operator=(const RenderLevel&) =delete;
	};
#ifdef RSGAME_NETCLIENT
	void init_player();
	void draw_players(float *data, int len, vec3 pos, vec3 look);
#endif
	void init_raytarget();
	void draw_raytarget(const RaycastResult &ray);
	void init_hud();
	void draw_hud(int width, int height, uint8_t id, uint8_t data);
	bool load_shaders();
	bool load_textures();
}
#endif
