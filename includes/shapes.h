#ifndef __SHAPES_H__
#define __SHAPES_H__

#include "mat.h"
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
} Rectangle;

struct __RectangleCreateParams {
	uint32_t __program;
	Color __color;

	Color border_color;
	int border_width;
};

#define TO_OPENGL_COORDS_X(x, width) (((float)(x) / (width)) * 2.0f - 1.0f) 
#define TO_OPENGL_COORDS_Y(y, height) (-((float)(y) / (height)) * 2.0f + 1.0f) 

Rectangle __rect_create(struct __RectangleCreateParams);
#define rect_create(program, color, ...) (__rect_create(((struct __RectangleCreateParams){\
	.__program = (program), \
	.__color = (color), \
	__VA_ARGS__ \
})))
void rect_show(Rectangle* rect, int x, int y, int w, int h, int screen_w, int screen_h);
void rect_free(Rectangle* rect);

typedef struct {
	Color color;
	Vec4 pos;

	uint32_t vao, vbo, ebo;
	uint32_t program;

	uint32_t indicies_count;
	Vec4 camera;
	float scale;

	Vec4 light_pos;
	Color light_color;

	float reflectivity;
	float shininess;
	uint8_t face_cull : 1;
	uint8_t depth_test : 1;
} Shape3D;

Shape3D shape3d_create(uint32_t program, Color color, float scale, uint32_t* indices, float* triangle_coords);
void shape3d_show(Shape3D* shape, int x, int y, int w, int h, int screen_w, int screen_h);
void shape3d_free(Shape3D* shape);

#endif // __SHAPES_H__
