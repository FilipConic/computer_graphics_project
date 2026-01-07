#ifndef __VEC2_H__
#define __VEC2_H__ 

typedef struct {
	int x, y;
} Vec2i;
typedef struct {
	float x, y;
} Vec2f;

#ifndef EPSILON
#define EPSILON 1e-5
#endif // EPSILON

Vec2i vec2f_to_vec2i(Vec2f a);
Vec2f vec2i_to_vec2f(Vec2i a);

// Vec2i

Vec2i vec2i(int x, int y);
Vec2i vec2i_add(Vec2i a, Vec2i b);
Vec2i vec2i_sub(Vec2i a, Vec2i b);
int vec2i_dot(Vec2i a, Vec2i b);
int vec2i_cross(Vec2i a, Vec2i b);
Vec2i vec2i_mult(Vec2i a, int n);
Vec2i vec2i_div(Vec2i a, int n);
float vec2i_magnitude(Vec2i a);
int vec2i_eq(Vec2i a, Vec2i b);
float vec2i_get_angle(Vec2i a, Vec2i b);

int vec2i_sign(Vec2i p, Vec2i a, Vec2i b);
int vec2i_triangle_collision(Vec2i p, Vec2i a, Vec2i b, Vec2i c);
int vec2i_polygon_collision(Vec2i p, Vec2i* points);
int vec2i_edge_edge_collision(Vec2i a, Vec2i b, Vec2i c, Vec2i d);
int vec2i_rect_collision(Vec2i p, Vec2i pos, Vec2i wh);

Vec2f vec2i_lerp(Vec2i a, Vec2i b, float t);
Vec2f vec2i_bezier_interpolate(Vec2i a, Vec2i b, Vec2i c, float t);

// Vec2f

Vec2f vec2f(float x, float y);
Vec2f vec2f_add(Vec2f a, Vec2f b);
Vec2f vec2f_sub(Vec2f a, Vec2f b);
float vec2f_dot(Vec2f a, Vec2f b);
float vec2f_cross(Vec2f a, Vec2f b);
Vec2f vec2f_mult(Vec2f a, float x);
Vec2f vec2f_div(Vec2f a, float x);
float vec2f_magnitude(Vec2f a);
int vec2f_eq(Vec2f a, Vec2f b);
float vec2f_get_angle(Vec2f a, Vec2f b);

int vec2f_triangle_collision(Vec2f p, Vec2f a, Vec2f b, Vec2f c);
int vec2f_polygon_collision(Vec2f p, Vec2f* points);
int vec2f_edge_edge_collision(Vec2f a, Vec2f b, Vec2f c, Vec2f d);

Vec2f vec2f_lerp(Vec2f a, Vec2f b, float t);
Vec2f vec2f_bezier_interpolate(Vec2f a, Vec2f b, Vec2f c, float t);

#endif // __VEC2_H__
