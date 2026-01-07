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
int font_show(FontTTF* font, char c, int x, int y);
int font_get_advance(FontTTF* font, char c);
void font_free(FontTTF* font);

#define check_shader_compile_status(shader, shader_name) do { \
	int success; \
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success); \
	if (!success) { \
		char info_log[512]; \
		glGetShaderInfoLog(shader, 512, NULL, info_log);\
		fprintf(stderr, "ERROR:\tUnable to compile shader [\"%s\"]!\nLOG:\t%s\n", shader_name, info_log); \
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

#endif // __FONT_H__
