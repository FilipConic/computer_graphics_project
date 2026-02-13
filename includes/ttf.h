#ifndef __TTF_H__
#define __TTF_H__

#include <evoco.h>
#include <stdint.h>

typedef struct {
	int32_t x, y;
	uint8_t on_curve;
} Point;

typedef struct {
	char found;
	char ascii_val;
	uint32_t glyph_index;
	Point* points;
	int* contour_end_indicies;
	int32_t advance_width;
	int32_t left_side_bearing;

	int32_t min_x;
	int32_t max_x;
	int32_t min_y;
	int32_t max_y;
} GlyphData;

typedef struct {
	EvoArena arena;
	EvoAllocator alloc;
	GlyphData glyphs[128];
	int32_t units_per_em;
} TTF;

TTF ttf_parse(const char* font_file_path);
GlyphData* ttf_get(TTF* ttf, char c);
void ttf_free(TTF* ttf);

#endif // __TTF_H__
