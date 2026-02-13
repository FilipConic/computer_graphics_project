#include <font.h>

#include <string.h>
#include <assert.h>
#include <glew.h>

#include <ear_clipping.h>
#include <stdio.h>
#include <ttf.h>

#define CYLIBX_ALLOC
#include <cylibx.h>

#define LETTER_COUNT 128

FontTTF font_compile(uint32_t program, const char* font_file_path, int size, int screen_width, int screen_height) {
	assert(size > 0);

	FontTTF font = {
		.size = size,
		.program = program,
		.ttf = ttf_parse(font_file_path),
		.screen_wh = vec2i(screen_width, screen_height),
	};
	font.scale = (float)size / font.ttf.units_per_em;

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
int font_show(FontTTF* font, char c, int x, int y, int screen_w, int screen_h) {
	Letter* letter = &font->letters[(int)c];
	if (!letter->found) {
		return 0;
	} else if (c == ' ') {
		return letter->advance;
	}

	poly_show(&letter->poly, x, y + font->size, 2.0f, 2.0f, screen_w, screen_h);
	return letter->advance;
}
int font_get_advance(FontTTF* font, char c) {
	Letter* letter = &font->letters[(int)c];
	if (!letter->found) {
		return 0;
	}
	return letter->advance;
}
int font_get_text_width(FontTTF* font, size_t n, const char* str) {
	int sum = 0;
	for (size_t i = 0; i < n; ++i) {
		sum += font_get_advance(font, str[i]);
	}
	return sum;
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

