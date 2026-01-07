#ifndef __EAR_CLIPPING_H__
#define __EAR_CLIPPING_H__

#include <stddef.h>
#include <stdint.h>

#include <ttf.h>
#include <vec2.h>
#include <color.h>

#ifndef BEZIER_STEP
#define BEZIER_STEP 0.2
#endif // BEZIER_STEP

typedef struct {
	uint32_t vao, vbo, ebo, program;
	size_t indices_count;
	Color clr;
	Vec2i pos;
	float projection_mat[16];
} Polygon;

struct __PolygonCreateParams {
	GlyphData* __glyph;
	uint32_t __program;
	float __scale;
	int x;
	int y;
	size_t width;
	size_t height;
};
Polygon __turn_glyph_into_polygon(struct __PolygonCreateParams params);
#define turn_glyph_into_polygon(glyph, program, scale, ...) __turn_glyph_into_polygon(((struct __PolygonCreateParams){ .__glyph = (glyph), .__program = program, .__scale = scale, __VA_ARGS__ }))
void poly_show(Polygon* poly);
void poly_free(Polygon* poly);

#endif // __EAR_CLIPPING_H__
