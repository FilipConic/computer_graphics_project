#include <vec2.h>
#include <math.h>
#include <stdio.h>
#include <cylibx.h>

Vec2i vec2f_to_vec2i(Vec2f a) {
	return (Vec2i){ .x = a.x, .y = a.y };
}
Vec2f vec2i_to_vec2f(Vec2i a) {
	return (Vec2f){ .x = a.x, .y = a.y };
}

Vec2i vec2i(int x, int y) {
	return (Vec2i){ .x = x, .y = y };
}
Vec2i vec2i_add(Vec2i a, Vec2i b) {
	return (Vec2i){ .x = a.x + b.x, .y = a.y + b.y };
}
Vec2i vec2i_sub(Vec2i a, Vec2i b) {
	return (Vec2i){ .x = a.x - b.x, .y = a.y - b.y };
}
int vec2i_dot(Vec2i a, Vec2i b) {
	return a.x * b.x + a.y * b.y;
}
int vec2i_cross(Vec2i a, Vec2i b) {
	return a.x * b.y - a.y * b.x;
}
Vec2i vec2i_mult(Vec2i a, int n) {
	return (Vec2i){ .x = a.x * n, .y = a.y * n };
}
Vec2i vec2i_div(Vec2i a, int n) {
	if (!n) {
		fprintf(stderr, "ERROR:\tTrying to divide an intiger vec2 by 0!\n");
		return (Vec2i){ .x = 0, .y = 0 };
	}
	return (Vec2i){ .x = a.x / n, .y = a.y / n };
}
float vec2i_magnitude(Vec2i a) {
	return sqrtf(a.x * a.x + a.y * a.y);
}
int vec2i_eq(Vec2i a, Vec2i b) {
	return a.x == b.x && a.y == b.y;
}
float vec2i_get_angle(Vec2i a, Vec2i b) {
	if ((!a.x && !a.y) || (!b.x && !b.y)) {
		fprintf(stderr, "ERROR:\tTrying to divide by zero while getting an angle between 2 intiger vectors (%d, %d) (%d, %d)!\n", a.x, a.y, b.x, b.y);
		return 0.0f;
	}
	return acosf(vec2i_dot(a, b) / (vec2i_magnitude(a) * vec2i_magnitude(b)));
}

int vec2i_sign(Vec2i p, Vec2i a, Vec2i b) {
	return (p.x - b.x) * (a.y - b.y) - (a.x - b.x) * (p.y - b.y);
}
int vec2i_triangle_collision(Vec2i p, Vec2i a, Vec2i b, Vec2i c) {
	int d1 = vec2i_sign(p, a, b);
	int d2 = vec2i_sign(p, b, c);
	int d3 = vec2i_sign(p, c, a);

	return (d1 <= 0 && d2 <= 0 && d3 <= 0) || (d1 >= 0 && d2 >= 0 && d3 >= 0);
}
int vec2i_polygon_collision(Vec2i p, Vec2i* points) {
	int sum = 0; 
	for (size_t i = 0; i < cyx_array_length(points); ++i) {
		size_t j = (i + 1) % cyx_array_length(points);
		Vec2i a = points[i];
		Vec2i b = points[j];

		if (!(a.y - b.y)) { continue; }

		float t = (p.y - a.y) / (float)(b.y - a.y);
		if (-EPSILON < t && t < 1.0 - EPSILON) {
			Vec2f intersect = vec2f_add(vec2i_to_vec2f(a), vec2f_mult(vec2i_to_vec2f(vec2i_sub(b, a)), t));

			if (intersect.x > p.x) {
				if (vec2f_eq(intersect, vec2i_to_vec2f(a))) {
					Vec2i c = *cyx_array_at(points, (int)i - 1);
					// printf("c: (%d, %d)\n", c.x, c.y);
					if (!((c.y <= a.y) ^ (b.y <= a.y))) {
						continue;
					}
				} else if (vec2f_eq(intersect, vec2i_to_vec2f(b))) {
					Vec2i c = points[(i + 2) % cyx_array_length(points)];
					// printf("c: (%d, %d)\n", c.x, c.y);
					if (!((c.y <= b.y) ^ (a.y <= b.y))) {
						continue;
					}
				}

				++sum;
				// printf("%d: (%d, %d) <- (%d, %d), (%.2f, %.2f) -> (%d, %d)\n", sum, a.x, a.y, p.x, p.y, intersect.x, intersect.y, b.x, b.y);
			}
		}
	}
	return sum % 2;
}
int vec2i_edge_edge_collision(Vec2i a, Vec2i b, Vec2i c, Vec2i d) {
	int dxcx = d.x - c.x;
	int dycy = d.y - c.y;

	int dd = (b.y - a.y) * dxcx - (b.x - a.x) * dycy;
	if (!dd) { return 0; }

	float ta = ((a.x - c.x) * dycy - (a.y - c.y) * dxcx) / (float)dd;
	float tc = ((a.x - c.x) * (b.y - a.y) - (a.y - c.y) * (b.x - a.x)) / (float)dd;
	if (0.0 < ta && ta < 1.0 && 0.0 < tc && tc < 1.0) {
		return 1;
	}

	return 0;
}

Vec2f vec2i_lerp(Vec2i a, Vec2i b, float t) {
	return vec2f_add(vec2i_to_vec2f(a), vec2f_mult(vec2i_to_vec2f(vec2i_sub(b, a)), t));
}
Vec2f vec2i_bezier_interpolate(Vec2i a, Vec2i b, Vec2i c, float t) {
	return vec2f_lerp(vec2i_lerp(a, b, t), vec2i_lerp(b, c, t), t);
}

Vec2f vec2f(float x, float y) {
	return (Vec2f){ .x = x, .y = y };
}
Vec2f vec2f_add(Vec2f a, Vec2f b) {
	return (Vec2f){ .x = a.x + b.x, .y = a.y + b.y };
}
Vec2f vec2f_sub(Vec2f a, Vec2f b) {
	return (Vec2f){ .x = a.x - b.x, .y = a.y - b.y };
}
float vec2f_dot(Vec2f a, Vec2f b) {
	return a.x * b.x + a.y * b.y;
}
float vec2f_cross(Vec2f a, Vec2f b) {
	return a.x * b.y - a.y * b.x;
}
Vec2f vec2f_mult(Vec2f a, float x) {
	return (Vec2f){ .x = a.x * x, .y = a.y * x };
}
Vec2f vec2f_div(Vec2f a, float x) {
	if (fabsf(x) < EPSILON) {
		fprintf(stderr, "ERROR:\tDivision by zero!\n");
		return (Vec2f){ 0 };
	}
	return (Vec2f){ .x = a.x / x, .y = a.y / x };
}
float vec2f_magnitude(Vec2f a) {
	return sqrtf(a.x * a.x + a.y * a.y);
}
int vec2f_eq(Vec2f a, Vec2f b) {
	return fabsf(a.x - b.x) < EPSILON && fabsf(a.y - b.y) < EPSILON;
}
float vec2f_get_angle(Vec2f a, Vec2f b) {
	float a_len = vec2f_magnitude(a);
	float b_len = vec2f_magnitude(b);
	if (fabsf(a_len) < EPSILON) {
		fprintf(stderr, "ERROR:\tTrying to divide by zero while getting an angle between 2 vectors! "
				"[a = (%.2f, %.2f), b = (%.2f, %.2f)]\n", a.x, a.y, b.x, b.y);
		return 0;
	}
	return acosf(vec2f_dot(a, b) / (a_len * b_len));
}

int vec2f_triangle_collision(Vec2f p, Vec2f a, Vec2f b, Vec2f c) {
	float c1 = vec2f_cross(vec2f_sub(b, a), vec2f_sub(p, a));
	float c2 = vec2f_cross(vec2f_sub(c, b), vec2f_sub(p, b));
	float c3 = vec2f_cross(vec2f_sub(a, c), vec2f_sub(p, c));

	return (c1 > EPSILON && c2 > EPSILON && c3 > EPSILON) || (c1 < EPSILON && c2 < EPSILON && c3 < EPSILON);
}
int vec2f_polygon_collision(Vec2f p, Vec2f* points) {
	int sum = 0;
	for (size_t i = 0; i < cyx_array_length(points); ++i) {
		size_t j = (i + 1) % cyx_array_length(points);
		Vec2f a = points[i];
		Vec2f b = points[j];

		if (fabsf(b.y - a.y) < EPSILON) { continue; }

		float t = (p.y - a.y) / (b.y - a.y);

		if (-EPSILON < t && t < 1.0 - EPSILON) {
			Vec2f intersect = vec2f_add(a, vec2f_mult(vec2f_sub(b, a), t));
			if (intersect.x > p.x) {
				++sum;
			}
		}
	}
	return sum % 2;
}
int vec2f_edge_edge_collision(Vec2f a, Vec2f b, Vec2f c, Vec2f d) {
	float dxcx = d.x - c.x;
	float dycy = d.y - c.y;

	float dd = (b.y - a.y) * dxcx - (b.x - a.x) * dycy;
	if (fabsf(dd) < EPSILON) {
		return 0;
	}

	float ta = ((a.x - c.x) * dycy - (a.y - c.y) * dxcx) / dd;
	float tc = ((a.x - c.x) * (b.y - a.y) - (a.y - c.y) * (b.x - a.x)) / dd;
	if (0.0 < ta && ta < 1.0 && 0.0 < tc && tc < 1.0) {
		return 1;
	}

	return 0;
}

Vec2f vec2f_lerp(Vec2f a, Vec2f b, float t) {
	return vec2f_add(a, vec2f_mult(vec2f_sub(b, a), t));
}
Vec2f vec2f_bezier_interpolate(Vec2f a, Vec2f b, Vec2f c, float t) {
	return vec2f_lerp(vec2f_lerp(a, b, t), vec2f_lerp(b, c, t), t);
}
