#ifndef __FONT_H__
#define __FONT_H__

#include <ttf.h>
#include <ear_clipping.h>

typedef struct {
	Polygon poly;
	uint8_t found;
	uint8_t goes_to_next_row;
	int32_t advance;
} Letter;

typedef struct {
	TTF ttf;
	int size;
	float scale;

	uint32_t program;
	Letter letters[128];
} FontTTF;

typedef struct {
	FontTTF* font; 

	int screen_width;
	int screen_height;

	int row_height;
	int row_count;

	int width;
	int height;
	Vec2i pos;

	Letter* letters;

	char* characters;

	uint8_t word_wrapping;
	uint8_t is_static;
	uint8_t last_was_newline;
} Text;

FontTTF font_compile(const char* font_file_path, int size, int screen_width, int screen_height);
void font_free(FontTTF* font);

struct __TextCreateParams {
	FontTTF* __font;
	int __screen_width;
	int __screen_height;
	int __width;
	int __height;

	int x;
	int y;
	uint8_t word_wrapping;
	uint8_t is_static;
	const char* text;
};

Text __text_create(struct __TextCreateParams params);
#define text_create(font, screen_width, screen_height, width, height, ...) __text_create(((struct __TextCreateParams){ \
	.__font = (font), \
	.__screen_height = (screen_height), \
	.__screen_width = (screen_width), \
	.__width = (width), \
	.__height = (height), \
	__VA_ARGS__ \
}))
void text_show(Text* s);
void text_push(Text* s, char c);
void text_pop(Text* s);
void text_free(Text* s);

#endif // __FONT_H__
