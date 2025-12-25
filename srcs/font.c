#include <font.h>

#include <assert.h>
#include <glew.h>

#include <ear_clipping.h>
#include <stdio.h>
#include <ttf.h>
#include <cylibx.h>

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

#define LETTER_COUNT 128

static uint32_t compile_program(const char* vert_file, const char* frag_file) {
	char* source = cyx_str_from_file(vert_file);
	uint32_t vert_shader = glCreateShader(GL_VERTEX_SHADER);
	int32_t len = cyx_str_length(source);
	glShaderSource(vert_shader, 1, (const char* const*)&source, &len);
	glCompileShader(vert_shader);
	check_shader_compile_status(vert_shader);
	cyx_str_free(source);

	source = cyx_str_from_file(frag_file);
	uint32_t frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
	len = cyx_str_length(source);
	glShaderSource(frag_shader, 1, (const char* const*)&source, &len);
	glCompileShader(frag_shader);
	check_shader_compile_status(frag_shader);
	cyx_str_free(source);

	uint32_t program = glCreateProgram();
	glAttachShader(program, vert_shader);
	glAttachShader(program, frag_shader);
	glLinkProgram(program);
	check_program_compile_status(program);

	glDeleteShader(vert_shader);
	glDeleteShader(frag_shader);
	return program;
}

FontTTF font_compile(const char* font_file_path, int size, int screen_width, int screen_height) {
	assert(size > 0);

	FontTTF font = {
		.size = size,
		.program = compile_program("./resources/shaders/font.vert", "./resources/shaders/font.frag"),
		.ttf = ttf_parse(font_file_path),
	};
	font.scale = 2.f * (float)size / font.ttf.units_per_em;

	for (size_t i = 0; i < LETTER_COUNT; ++i) {
		GlyphData* glyph = ttf_get(&font.ttf, i);
		if (!glyph->found) { continue; }

		Letter* curr = font.letters + i;
		curr->poly = turn_glyph_into_polygon(glyph, font.program, font.scale,
					.height = screen_height, .width = screen_width);
		curr->found = 1;
		curr->advance = glyph->advance_width * font.scale;
	}

	return font;
}
static Letter* font_get(FontTTF* font, char c) {
	Letter* letter = &font->letters[(int)c];
	if (!letter->found) {
		return NULL;
	}
	return letter;
}
void font_free(FontTTF* font) {
	for (size_t i = 0; i < LETTER_COUNT; ++i) {
		Letter* curr = font->letters + i;
		if (!curr->found) { continue; }
		poly_free(&curr->poly);
	}
	glDeleteProgram(font->program);
	ttf_free(&font->ttf);
}

Sentence __sentence_create(struct __SentenceCreateParams params) {
	Sentence res = {
		.pos = (Vec2i){ .x = params.x, .y = params.y },
		.screen_width = params.__screen_width,
		.screen_height = params.__screen_height,
		.word_wrapping = params.word_wrapping,
		.characters = cyx_str_new(),
		.letters = cyx_array_new(Letter),
	};
	return res;
}
void sentence_show(Sentence* s) {
	for (size_t i = 0; i < cyx_array_length(s->letters); ++i) {
		if (s->characters[i] != ' ') {
			poly_draw(&s->letters[i].poly);
		}
	}
}
void sentence_push(Sentence* s, FontTTF* font, char c) {
	Letter* letter = font_get(font, c);
	if (!letter) { return; }
	if (cyx_str_length(s->characters)) {
		Letter* last = cyx_array_at(s->letters, -1);

		Letter new_letter = *letter;
		new_letter.poly.pos = (Vec2i){
			.x = last->poly.pos.x + last->advance,
			.y = s->pos.y,
		};
		cyx_array_append(s->letters, new_letter);
	} else {
		Letter new_letter = *letter;
		new_letter.poly.pos = s->pos;
		cyx_array_append(s->letters, new_letter);
	}
	cyx_str_append_char(&s->characters, c);
	// printf(CYX_STR_FMT "\n", CYX_STR_UNPACK(s->characters));
}
void sentence_pop(Sentence* s) {
	if (cyx_str_length(s->characters)) {
		cyx_array_pop(s->letters);
		cyx_str_pop(s->characters);
	}
}
void sentence_free(Sentence* s) {
	cyx_str_free(s->characters);
	cyx_array_free(s->letters);
}

