#ifndef __FONT_H__
#define __FONT_H__

#include <ttf.h>
#include <ear_clipping.h>

typedef struct {
	Polygon poly;
	uint8_t found;
	int32_t advance;
} Letter;

typedef struct {
	TTF ttf;
	int size;
	float scale;

	Vec2i screen_wh;

	uint32_t program;
	Letter letters[128];
} FontTTF;

FontTTF font_compile(uint32_t program, const char* font_file_path, int size, int screen_width, int screen_height);
int font_show(FontTTF* font, char c, int x, int y, int screen_w, int screen_h);
int font_get_advance(FontTTF* font, char c);
int font_get_text_width(FontTTF* font, size_t n, const char* str);
void font_free(FontTTF* font);

#endif // __FONT_H__
