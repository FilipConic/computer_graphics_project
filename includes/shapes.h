#ifndef __SHAPES_H__
#define __SHAPES_H__

#include <stddef.h>
#include <stdint.h>
#include <glew.h>
#include <vec2.h>
#include <color.h>

typedef struct {
	Vec2i pos;
	Vec2i wh;

	Color color;
	Color border_color;
	int border_width;
	Vec2i screen_wh;

	Vec2f top_left;
	Vec2f bottom_right;

	uint32_t vao, vbo, ebo;
	uint32_t program;
} StaticRectangle;

struct __StaticRectangleCreateParams {
	uint32_t __program;
	Vec2i __pos;
	Vec2i __wh;
	Vec2i __screen_wh;
	Color __color;

	int width;
	int height;

	Color border_color;
	int border_width;
};

#define TO_OPENGL_COORDS_X(x, width) (((float)(x) / (width)) * 2.0f - 1.0f) 
#define TO_OPENGL_COORDS_Y(y, height) (-((float)(y) / (height)) * 2.0f + 1.0f) 

StaticRectangle __s_rect_create(struct __StaticRectangleCreateParams);
#define s_rect_create(program, screen_w, screen_h, pos_x, pos_y, width, height, color, ...) (__s_rect_create(((struct __StaticRectangleCreateParams){\
	.__program = (program), \
	.__screen_wh = ((Vec2i){ .x = (screen_w), .y = (screen_h) }), \
	.__pos = ((Vec2i){ .x = (pos_x), .y = (pos_y) }), \
	.__wh = ((Vec2i){ .x = (width), .y = (height) }), \
	.__color = (color), \
	__VA_ARGS__ \
})))
void s_rect_show(StaticRectangle* rect);
void s_rect_set_pos(StaticRectangle* rect, Vec2i pos);
void s_rect_free(StaticRectangle* rect);

#endif // __SHAPES_H__
