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

	int width;
	int height;
	int x;
	int y;
	uint8_t word_wrapping;
	uint8_t is_static;
	const char* text;
};

#define check_shader_compile_status(shader) do { \
	int success; \
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success); \
	if (!success) { \
		char info_log[512]; \
		glGetShaderInfoLog(shader, 512, NULL, info_log);\
		fprintf(stderr, "ERROR:\tUnable to compile shader!\nLOG:\t%s\n", info_log); \
	} \
} while(0)
#define check_program_compile_status(program) do { \
	int success; \
	glGetProgramiv(program, GL_COMPILE_STATUS, &success); \
	if (!success) { \
		char info_log[512]; \
		glGetProgramInfoLog(program, 512, NULL, info_log);\
		fprintf(stderr, "ERROR:\tUnable to compile program!\nLOG:\t%s\n", info_log); \
	} \
} while(0)
uint32_t compile_program(const char* vert_file, const char* frag_file);

Text __text_create(struct __TextCreateParams params);
#define text_create(font, screen_width, screen_height, ...) __text_create(((struct __TextCreateParams){ \
	.__font = (font), \
	.__screen_width = (screen_width), \
	.__screen_height = (screen_height), \
	__VA_ARGS__ \
}))
Vec2i text_cursor_pos(Text* t, Vec2i padding);
void text_show(Text* s);
void text_push(Text* s, char c);
void text_pop(Text* s);
void text_free(Text* s);

#endif // __FONT_H__
