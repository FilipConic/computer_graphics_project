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
	int screen_width;
	int screen_height;
	Vec2i pos;

	Letter* letters;

	char* characters;

	uint8_t word_wrapping;
} Sentence;

typedef struct {
	TTF ttf;
	int size;
	float scale;

	uint32_t program;
	Letter letters[128];
} FontTTF;

FontTTF font_compile(const char* font_file_path, int size, int screen_width, int screen_height);
void font_free(FontTTF* font);

struct __SentenceCreateParams {
	int __screen_width;
	int __screen_height;

	int x;
	int y;
	uint8_t word_wrapping;
};

Sentence __sentence_create(struct __SentenceCreateParams params);
#define sentence_create(screen_width, screen_height, ...) __sentence_create(((struct __SentenceCreateParams){ .__screen_height = screen_height, .__screen_width = screen_width, __VA_ARGS__ }))
void sentence_show(Sentence* s);
void sentence_push(Sentence* s, FontTTF* font, char c);
void sentence_pop(Sentence* s);
void sentence_free(Sentence* s);

#endif // __FONT_H__
