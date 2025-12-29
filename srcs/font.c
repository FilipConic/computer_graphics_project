#include <font.h>

#include <assert.h>
#include <glew.h>

#include <ear_clipping.h>
#include <stdio.h>
#include <ttf.h>
#include <cylibx.h>

#define LETTER_COUNT 128

uint32_t compile_program(const char* vert_file, const char* frag_file) {
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
		// if (i > 32) printf("%d: '%c'\n", (int)i, (char)i);
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

Text __text_create(struct __TextCreateParams params) {
	Text res = {
		.font = params.__font,
		.screen_width = params.__screen_width,
		.screen_height = params.__screen_height,

		.width = params.width,
		.height = params.height,
		.pos = (Vec2i){ .x = params.x, .y = params.y },
		.word_wrapping = params.word_wrapping,
		.is_static = params.is_static,

		.row_height = (int)(2.5 * params.__font->size),
		.letters = cyx_array_new(Letter),
	};

	if (!params.is_static) {
		res.characters = cyx_str_new();
	} else {
		assert(params.text && "ASSERT: You need to provide text to text_create when making it static!");
		res.characters = cyx_str_new();
		for (size_t i = 0; i < strlen(params.text); ++i) {
			text_push(&res, params.text[i]);
		}
	}

	return res;
}
Vec2i text_cursor_pos(Text* t, Vec2i padding) {
	Vec2i pos = { 0 };
	if (!t->last_was_newline) {
		if (cyx_array_length(t->letters)) {
			Letter* last = cyx_array_at(t->letters, -1);
			pos = vec2i_add(padding, vec2i(last->poly.pos.x + last->advance, t->screen_height - last->poly.pos.y - t->row_height));
			if (t->word_wrapping && pos.x + last->advance >= t->pos.x + t->width) {
				pos = vec2i_add(padding, vec2i(t->pos.x, t->pos.y + (t->row_count + 1) * t->row_height));
			}
		} else {
			pos = vec2i_add(padding, vec2i(t->pos.x, t->pos.y));
		}
	} else {
		pos = vec2i_add(padding, vec2i(t->pos.x, t->pos.y + t->row_count * t->row_height));
	}
	return pos;
}
void text_show(Text* s) {
	for (size_t i = 0, j = 0; i < cyx_array_length(s->letters); ++j) {
		if (s->characters[j] > ' ') {
			poly_draw(&s->letters[i++].poly);
		} else if (s->characters[j] == ' ') {
			++i;
		}
	}
}
void text_push(Text* s, char c) {
	Letter* letter = font_get(s->font, c);

	if (!letter) { return; }
	Letter new_letter = *letter;

	if (c != '\n') {
		if (!s->last_was_newline) {
			if (cyx_array_length(s->letters)) {
				Letter* last = cyx_array_at(s->letters, -1);
				new_letter.poly.pos = vec2i(last->poly.pos.x + last->advance, last->poly.pos.y);
				if (s->word_wrapping && new_letter.poly.pos.x + new_letter.advance >= s->pos.x + s->width) {
					s->row_count++;
					new_letter.poly.pos = vec2i(s->pos.x, s->screen_height - s->pos.y - (s->row_count + 1) * s->row_height);
					new_letter.goes_to_next_row = 1;
				}
			} else {
				new_letter.poly.pos = vec2i(s->pos.x, s->screen_height - s->pos.y - s->row_height);
			}
		} else {
			new_letter.poly.pos = vec2i(s->pos.x, s->screen_height - s->pos.y - (s->row_count + 1) * s->row_height);
			s->last_was_newline = 0;
		}
	} else {
		++s->row_count;
		s->last_was_newline = 1;
	}
	if (!s->last_was_newline) {
		cyx_array_append(s->letters, new_letter);
	}
	cyx_str_append_char(&s->characters, c);
	// printf(CYX_STR_FMT "\n", CYX_STR_UNPACK(s->characters));
}
void text_pop(Text* s) {
	if (cyx_str_length(s->characters)) {
		char c = s->characters[cyx_str_length(s->characters) - 1];
		if (c != '\n') {
			if (cyx_array_length(s->letters)) {
				if (s->word_wrapping && (cyx_array_at(s->letters, -1))->goes_to_next_row) {
					--s->row_count;
				}
				cyx_array_pop(s->letters);
			}
		} else {
			--s->row_count;
		}
		cyx_str_pop(s->characters);
	}
}
void text_free(Text* s) {
	cyx_str_free(s->characters);
	cyx_array_free(s->letters);
}

