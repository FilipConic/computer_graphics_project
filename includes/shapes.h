#ifndef __SHAPES_H__
#define __SHAPES_H__

#include <stddef.h>
#include <stdint.h>
#include <glew.h>
#include <vec2.h>
#include <color.h>

typedef struct {
	Color color;
	Color border_color;
	int border_width;

	uint32_t vao, vbo, ebo;
	uint32_t program;
} StaticRectangle;

struct __StaticRectangleCreateParams {
	uint32_t __program;
	Color __color;

	Color border_color;
	int border_width;
};

#define TO_OPENGL_COORDS_X(x, width) (((float)(x) / (width)) * 2.0f - 1.0f) 
#define TO_OPENGL_COORDS_Y(y, height) (-((float)(y) / (height)) * 2.0f + 1.0f) 

StaticRectangle __rect_create(struct __StaticRectangleCreateParams);
#define rect_create(program, color, ...) (__rect_create(((struct __StaticRectangleCreateParams){\
	.__program = (program), \
	.__color = (color), \
	__VA_ARGS__ \
})))
void rect_show(StaticRectangle* rect, int x, int y, int w, int h, int screen_w, int screen_h);
void rect_free(StaticRectangle* rect);

#endif // __SHAPES_H__
