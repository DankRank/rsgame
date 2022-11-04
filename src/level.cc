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
		set_tile(3, 0, i, 35, 0);
	set_tile(3, 1, 0, 50, 0);
	set_tile(3, 1, 1, 50, 1);
	set_tile(3, 1, 2, 50, 2);
	set_tile(3, 1, 3, 50, 3);
	set_tile(3, 1, 4, 50, 4);
	set_tile(3, 1, 5, 75, 0);
	set_tile(3, 1, 6, 75, 1);
	set_tile(3, 1, 7, 75, 2);
	set_tile(3, 1, 8, 75, 3);
	set_tile(3, 1, 9, 75, 4);
	set_tile(3, 1, 10, 76, 0);
	set_tile(3, 1, 11, 76, 1);
	set_tile(3, 1, 12, 76, 2);
	set_tile(3, 1, 13, 76, 3);
	set_tile(3, 1, 14, 76, 4);
	set_tile(3, 1, 15, 55, 0);
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
}
