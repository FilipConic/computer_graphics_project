#include "vec2.h"
#include <ear_clipping.h>

#include <math.h>
#include <glew.h>

#include <cylibx.h>

typedef struct Contour Contour;
struct Contour {
	enum {
		CONTOUR_INSIDE,
		CONTOUR_OUTSIDE,
	} type;
	Vec2i* points;
	Contour* holes;
};

static int get_winding_order(Vec2i* points) {
	float sum = 0;
	for (size_t i = 0; i < cyx_array_length(points); ++i) {
		size_t j = (i + 1) % cyx_array_length(points);
		Vec2i p1 = points[i];
		Vec2i p2 = points[j];
		sum += (p2.x - p1.x) * (p2.y + p1.y);
	}
	return sum < 0 ? -1 : 1;
}
static int is_polygon_inside_polygon(Vec2i* poly1, Vec2i* poly2) {
	for (size_t i = 0; i < cyx_array_length(poly2); ++i) {
		Vec2i p = poly2[i];
		if (!vec2i_polygon_collision(p, poly1)) {
			return 0;
		}
	}

	for (size_t i = 0; i < cyx_array_length(poly1); ++i) {
		Vec2i a = poly1[i];
		Vec2i b = poly1[(i + 1) % cyx_array_length(poly1)];

		for (size_t j = 0; j < cyx_array_length(poly2); ++j) {
			Vec2i c = poly2[j];
			Vec2i d = poly2[(j + 1) % cyx_array_length(poly2)];

			if (vec2i_edge_edge_collision(a, b, c, d)) {
				return 0;
			}
		}
	}

	return 1;
}
static void dedegen_points(Vec2i* points) {
repeat1:
	for (size_t i = 0; i < cyx_array_length(points); ++i) {
		Vec2i p1 = vec2i_div(points[i], 10);
		Vec2i p2 = vec2i_div(points[(i + 1) % cyx_array_length(points)], 10);
		if (vec2i_eq(p1, p2)) {
			cyx_array_remove(points, i);
			goto repeat1;
		}
	}
repeat2:
	for (size_t i = 0; i < cyx_array_length(points); ++i) {
		Vec2i prev = points[(i + cyx_array_length(points) - 1) % cyx_array_length(points)];
		Vec2i curr = points[i];
		Vec2i next = points[(i + 1) % cyx_array_length(points)];

		float angle = vec2i_get_angle(vec2i_sub(prev, curr), vec2i_sub(next, curr));
		if (fabs(angle - M_PI) < 1e-1) {
			cyx_array_remove(points, i);
			goto repeat2;
		}
	}
}
static uint32_t* ear_clipping(Vec2i* points) {
	dedegen_points(points);
	assert(cyx_array_length(points) >= 3);

	if (get_winding_order(points) != 1) {
		cyx_array_reverse(points);
	}

	// printf("points = [");
	// for (size_t i = 0; i < cyx_array_length(points); ++i) {
	// 	if (i) { printf(", "); }
	// 	printf("(%d, %d)", points[i].x, points[i].y);
	// }
	// printf("]\n");

	int* indices = cyx_array_new(int, .reserve = cyx_array_length(points));
	for (size_t i = 0; i < cyx_array_length(points); ++i) {
		cyx_array_append(indices, i);
	}

	uint32_t* triangles = cyx_array_new(uint32_t, .reserve = (cyx_array_length(points) - 2) * 3);
	while (cyx_array_length(indices) > 3) {
		int ear_found = 0;

		for (int i = 0; i < (int)cyx_array_length(indices); ++i) {
			int a = indices[i];
			int b = *cyx_array_at(indices, i - 1);
			int c = indices[(i + 1) % cyx_array_length(indices)];

			Vec2i va = points[a];
			Vec2i vb = points[b];
			Vec2i vc = points[c];

			if (vec2i_cross(vec2i_sub(vb, va), vec2i_sub(vc, va)) < 0) { continue; }

			uint8_t is_ear = 1;
			for (int j = 0; j < (int)cyx_array_length(points); ++j) {
				Vec2i vp = points[j];
				if (vec2i_eq(vp, va) || vec2i_eq(vp, vb) || vec2i_eq(vp, vc)) {
					continue;
				}
				if (vec2i_triangle_collision(vp, vb, va, vc)) {
					// printf("Point %d in triangle %d, %d, %d\n", j, b, a, c);
					is_ear = 0;
					break;
				}
			}

			// printf("%d: { ", a);
			// for (size_t i = 0 ; i < cyx_array_length(indices); ++i) {
			// 	if (i) { printf(", "); }
			// 	printf("%d", indices[i]);
			// }
			// printf(" }\n");

			if (is_ear) {
				// printf("Added: %d, %d, %d\n", b, a, c);
				cyx_array_append_mult(triangles, b, a, c);
				cyx_array_remove(indices, i);
				ear_found = 1;
				break;
			}	
		} 
		if (!ear_found) {
			fprintf(stderr, "ERROR:\tStopped ear clipping, polygon possibly degenerate!\n");
			break;
		}
	}

	if (cyx_array_length(indices) == 3) {
		cyx_array_append_mult(triangles, indices[1], indices[0], indices[2]);
	}

	cyx_array_free(indices);
	return triangles;
}

static void contour_free(void* val) {
	Contour* c = val;
	cyx_array_free(c->points);
	if (c->type == CONTOUR_OUTSIDE && c->holes) {
		cyx_array_free(c->holes);
	}
}
static void contour_add(Contour* arr, Contour in) {
	in.type = CONTOUR_INSIDE;
	for (size_t i = 0; i < cyx_array_length(arr); ++i) {
		Contour* out = arr + i;
		if (is_polygon_inside_polygon(out->points, in.points)) {
			if (!out->holes) {
				out->holes = cyx_array_new(Contour, .defer_fn = contour_free, .reserve = 2);
			}
			cyx_array_append(out->holes, in);
		}
	}
}

static Vec2i point_to_vec2i(Point p) {
	return (Vec2i){ .x = p.x, .y = p.y };
}
static Vec2i* transform_points_to_vec2i(Point** points_ptr) {
repeat:
	for (size_t i = 0; i < cyx_array_length(*points_ptr); ++i) {
		Point p1 = (*points_ptr)[i];
		Point p2 = (*points_ptr)[(i + 1) % cyx_array_length(*points_ptr)];
		if (p1.on_curve == p2.on_curve) {
			cyx_array_insert(*points_ptr, ((Point){ .x = (p1.x + p2.x) / 2, .y = (p1.y + p2.y) / 2, .on_curve = !p1.on_curve }), i + 1);
			goto repeat;
		}
	}
	assert(cyx_array_length(*points_ptr) % 2 == 0);

	Point* points = *points_ptr;
	Vec2i* res = cyx_array_new(Vec2i);
	for (size_t i = 0; i < cyx_array_length(points); i += 2) {
		Point p1 = points[i];
		Point p2 = points[(i + 1) % cyx_array_length(points)];
		Point p3 = points[(i + 2) % cyx_array_length(points)];
		assert(p1.on_curve);
		assert(!p2.on_curve);
		assert(p3.on_curve);

		for (float t = 0.0; t < 1.0; t += BEZIER_STEP) {
			Vec2i vec = vec2f_to_vec2i(vec2i_bezier_interpolate(point_to_vec2i(p1), point_to_vec2i(p2), point_to_vec2i(p3), t));
			cyx_array_append(res, vec);
		}
	}
	return res;
}
static Contour* turn_glyph_into_contours(GlyphData* glyph) {
	Contour* res = cyx_array_new(Contour, .defer_fn = contour_free, .reserve = 4);
	size_t prev = 0;
	
	Point* working_points = cyx_array_new(Point);
	for (size_t i = 0; i < cyx_array_length(glyph->contour_end_indicies); ++i) {
		cyx_array_length(working_points) = 0;

		for (size_t j = prev; j <= (size_t)glyph->contour_end_indicies[i]; ++j) {
			cyx_array_append(working_points, glyph->points[j]);
		}
		Vec2i* points = transform_points_to_vec2i(&working_points);
		if (get_winding_order(points) == 1) {
			cyx_array_append(res, ((Contour){ .points = points, .type = CONTOUR_OUTSIDE }));
		} else {
			contour_add(res, (Contour){ .points = points, .type = CONTOUR_INSIDE });
		}
		prev = glyph->contour_end_indicies[i] + 1;
	}
	cyx_array_free(working_points);
	return res;
}
static int find_max_x_index(Contour* inside) {
	assert(inside->type == CONTOUR_INSIDE);

	Vec2i* points = inside->points;
	size_t idx = 0;
	for (size_t i = 1; i < cyx_array_length(points); ++i) {
		if (points[i].x > points[idx].x) { idx = i; }
	}
	return (int)idx;
}
static int* get_reflex_angles(Vec2i* points) {
	int* reflex = cyx_array_new(int);
	for (int i = 0; i < (int)cyx_array_length(points); ++i) {
		Vec2i A = points[i];
		Vec2i B = points[(i + 1) % cyx_array_length(points)];
		Vec2i C = *cyx_array_at(points, i - 1);
		if (vec2i_cross(vec2i_sub(B, A), vec2i_sub(C, A)) > EPSILON) { 
			cyx_array_append(reflex, i);
		}
	}
	return reflex;
}
static int shoot_ray(Contour* in, Vec2i* points) {
	assert(in->type == CONTOUR_INSIDE);

	int m = find_max_x_index(in); // step 1
	Vec2i M = in->points[m];

	Vec2i Vi = { 0 };
	Vec2i Vi_1 = { 0 };
	Vec2i I = { 0 };
	int found = 0;
	float t = 0;
	size_t i = 0;
	for (; i < cyx_array_length(points); ++i) {
		Vi = points[i];
		Vi_1 = points[(i + 1) % cyx_array_length(points)];

		if (Vi.y > M.y || Vi_1.y < M.y || Vi.y == Vi_1.y) { continue; }

		t = (M.y - Vi.y) / (float)(Vi_1.y - Vi.y); // step 2
		I = vec2f_to_vec2i( // I = Vi + t * (Vi_1 - Vi)
			vec2f_add(vec2i_to_vec2f(Vi), vec2f_mult(vec2i_to_vec2f(vec2i_sub(Vi_1, Vi)), t))
		); 

		if (-EPSILON < t && t < 1.0 + EPSILON && I.x > M.x) { 
			found = 1;
			break;
		}
	}
	assert(found);


	if (fabsf(t) < EPSILON) { // step 3 
		return i;
	} else if (fabsf(t - 1.0f) < EPSILON) {
		return (i + 1) % cyx_array_length(points);
	}


	Vec2i P = Vi_1.x > Vi.x ? Vi_1 : Vi;
	int P_idx = Vi_1.x > Vi.x ? (i + 1) % cyx_array_length(points) : i; // step 4

	int* reflex_angles = get_reflex_angles(points);
	for (int j = 0; j < (int)cyx_array_length(reflex_angles); ++j) { // step 5
		int idx = reflex_angles[j];
		Vec2i A = points[idx];
		if (A.x < M.x) { continue; }
		if (vec2i_eq(A, P)) { continue; }

		if (vec2i_triangle_collision(A, M, I, P)) {
			goto arent_outside;
		}
	}

	cyx_array_free(reflex_angles);
	return P_idx;
arent_outside:
	// step 6
	{
		float min_angle = vec2i_get_angle(vec2i_sub(I, M), vec2i_sub(P, M));
		int res_idx = -1;
		for (size_t j = 0; j < cyx_array_length(reflex_angles); ++j) {
			size_t idx = reflex_angles[j];
			Vec2i R = points[idx];
			if (R.x < M.x) { continue; }
			float angle = vec2i_get_angle(vec2i_sub(I, M), vec2i_sub(P, M));
			if (angle < min_angle) {
				min_angle = angle;
				res_idx = idx;
			}
		}
		assert(res_idx != -1);

		cyx_array_free(reflex_angles);
		return res_idx;
	}
	return 0;
}
static Vec2i* connect_points(Vec2i* in_points, Vec2i* out_points, size_t in_idx, size_t out_idx) {
	Vec2i* points = cyx_array_new(Vec2i);
	for (size_t i = 0; i <= out_idx; ++i) {
		cyx_array_append(points, out_points[i]);
	}
	for (size_t j = in_idx; j < cyx_array_length(in_points); ++j) {
		cyx_array_append(points, in_points[j]);
	}
	for (size_t j = 0; j <= in_idx; ++j) {
		cyx_array_append(points, in_points[j]);
	}
	for (size_t i = out_idx; i < cyx_array_length(out_points); ++i) {
		cyx_array_append(points, out_points[i]);
	}

	// printf("Connecting %zu, %zu\n", in_idx, out_idx);
	//
	// printf("out_points = [");
	// for (size_t i = 0; i < cyx_array_length(out_points); ++i) {
	// 	if (i) { printf(", "); }
	// 	printf("(%d, %d)", out_points[i].x, out_points[i].y);
	// }
	// printf("]\n");
	//
	// printf("in_points = [");
	// for (size_t i = 0; i < cyx_array_length(in_points); ++i) {
	// 	if (i) { printf(", "); }
	// 	printf("(%d, %d)", in_points[i].x, in_points[i].y);
	// }
	// printf("]\n");

	return points;
}

typedef struct {
	uint32_t* triangles;
	float* vertices;
} TriangleVerticesPair;
static TriangleVerticesPair turn_contour_into_triangles(Contour* contours) {
	assert(cyx_array_length(contours) > 0);

	uint32_t* triangles = cyx_array_new(uint32_t);
	float* vertices = cyx_array_new(float);
	size_t vertices_len_saved = 0;
	for (size_t c = 0; c < cyx_array_length(contours); ++c) {
		Contour* contour = contours + c;
		assert(contour->type == CONTOUR_OUTSIDE);

		if (get_winding_order(contour->points) != -1) {
			cyx_array_reverse(contour->points);
		}

		uint32_t* new_triangles = NULL;

		if (contour->holes && cyx_array_length(contour->holes) > 0) {
			Contour* in = contour->holes;

			if (get_winding_order(in->points) != 1) {
				cyx_array_reverse(in->points);
			}

			int in_idx = find_max_x_index(in);
			int out_idx = shoot_ray(in, contour->points);
			Vec2i* points = connect_points(in->points, contour->points, in_idx, out_idx);

			for (size_t i = 1; i < cyx_array_length(contour->holes); ++i) {
				in = contour->holes + i;
				if (get_winding_order(in->points) != 1) {
					cyx_array_reverse(in->points);
				}

				int in_idx = find_max_x_index(in);
				int out_idx = shoot_ray(in, points);

				Vec2i* new_points = connect_points(in->points, points, in_idx, out_idx);
				cyx_array_free(points);
				points = new_points;
			}
			new_triangles = ear_clipping(points);

			for (size_t i = 0; i < cyx_array_length(points); ++i) {
				cyx_array_append_mult(vertices, points[i].x, points[i].y, 0.0f);
			}

			cyx_array_free(points);
		} else {
			new_triangles = ear_clipping(contour->points);

			for (size_t i = 0; i < cyx_array_length(contour->points); ++i) {
				cyx_array_append_mult(vertices, contour->points[i].x, contour->points[i].y, 0.0f);
			}
		}

		for (size_t i = 0; i < cyx_array_length(new_triangles); ++i) {
			new_triangles[i] += vertices_len_saved;
		}

		vertices_len_saved = cyx_array_length(vertices) / 3;
		cyx_array_append_mult_n(triangles, cyx_array_length(new_triangles), new_triangles);

		cyx_array_free(new_triangles);
	}

	return (TriangleVerticesPair){ .triangles = triangles, .vertices = vertices };
}
static void set_projection_matrix(float matrix[], size_t width, size_t height) {
	memset(matrix, 0, sizeof(*matrix) * 16);
    matrix[0] = 2.0f / width;
    matrix[5] = 2.0f / height;
   	matrix[10] = -1.0f;
    matrix[12] = -1.0f;
    matrix[13] = -1.0f;
    matrix[15] = 1.0f;
}

Polygon __turn_glyph_into_polygon(struct __PolygonCreateParams params) {
	GlyphData* glyph = params.__glyph;
	uint32_t program = params.__program;

	Contour* cs = turn_glyph_into_contours(glyph);

	TriangleVerticesPair pair = turn_contour_into_triangles(cs);
	uint32_t* triangles = pair.triangles;
	float* vertices = pair.vertices;

	float scale = params.__scale;

	assert(cyx_array_length(vertices) % 3 == 0);
	for (size_t i = 0; i < cyx_array_length(vertices); i += 3) {
		vertices[i + 0] *= scale; // x
		vertices[i + 1] *= -scale; // y
		vertices[i + 2] = 0.0f; // z
	}

	cyx_array_free(cs);

	Polygon poly = {
		.program = program,
		.clr = COLOR_WHITE,
		.indices_count = cyx_array_length(triangles),
		.pos = vec2i(params.x, params.y),
	};
	set_projection_matrix(poly.projection_mat, params.width, params.height);
	
	glGenVertexArrays(1, &poly.vao);
	glBindVertexArray(poly.vao);

	glGenBuffers(1, &poly.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, poly.vbo);
	glBufferData(GL_ARRAY_BUFFER, cyx_array_length(vertices) * sizeof(float), vertices, GL_STATIC_DRAW);

	glGenBuffers(1, &poly.ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, poly.ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, cyx_array_length(triangles) * sizeof(uint32_t), triangles, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), NULL);
	glEnableVertexAttribArray(0);

	cyx_array_free(triangles);
	cyx_array_free(vertices);
	
	return poly;
}
void poly_draw(Polygon* poly) {
	glUseProgram(poly->program);

	int uniform_color = glGetUniformLocation(poly->program, "in_color");
	glUniform4f(uniform_color, poly->clr.r / 255.0, poly->clr.g / 255.0, poly->clr.b / 255.0, 1.0);

	int uniform_pos = glGetUniformLocation(poly->program, "screen_pos");
	glUniform2f(uniform_pos, poly->pos.x, poly->pos.y);

	int uniform_projection = glGetUniformLocation(poly->program, "projection");
	glUniformMatrix4fv(uniform_projection, 1, GL_FALSE, poly->projection_mat);

	glBindVertexArray(poly->vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, poly->ebo);
	glDrawElements(GL_TRIANGLES, poly->indices_count, GL_UNSIGNED_INT, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	glUseProgram(0);
}
void poly_free(Polygon* poly) {
	glDeleteVertexArrays(1, &poly->vao);
	glDeleteBuffers(1, &poly->vbo);
	glDeleteBuffers(1, &poly->ebo);
}

