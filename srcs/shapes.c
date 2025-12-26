#include <color.h>
#include <vec2.h>
#include <shapes.h>
#include <stdio.h>


#define CLAMP(val, from, to) \
		if ((val) > (to)) { (val) = (to); } \
		if ((val) < (from)) { (val) = (from); }
StaticRectangle __s_rect_create(struct __StaticRectangleCreateParams params) {
	Vec2f wh = vec2i_to_vec2f(params.__wh);
	Vec2f screen_wh = vec2i_to_vec2f(params.__screen_wh);

	StaticRectangle rect = {
		.program = params.__program,
		.pos = params.__pos,
		.color = params.__color,
		.screen_wh = params.__screen_wh,

		.wh = params.__wh,
		.border_color = params.border_color,
		.border_width = params.border_width,
	};

	s_rect_set_pos(&rect, rect.pos);

	float vertices[] = {
		0.0, 2.0f * wh.y / (float)screen_wh.y,
		0.0, 0.0,
		2.0f * wh.x / (float)screen_wh.x, 0.0,
		2.0f * wh.x / (float)screen_wh.x, 2.0f * wh.y / (float)screen_wh.y,
	};

	uint32_t triangles[] = {
		0, 1, 2,
		0, 2, 3
	};

	glGenVertexArrays(1, &rect.vao);
	glBindVertexArray(rect.vao);

	glGenBuffers(1, &rect.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, rect.vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices) / sizeof(*vertices) * sizeof(float), vertices, GL_STATIC_DRAW);

	glGenBuffers(1, &rect.ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rect.ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(triangles) / sizeof(*triangles) * sizeof(uint32_t), triangles, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), NULL);
	glEnableVertexAttribArray(0);

	return rect;
}
void s_rect_set_pos(StaticRectangle* rect, Vec2i pos) {
	rect->pos = pos;

	rect->top_left = vec2f(
		TO_OPENGL_COORDS_X(pos.x, rect->screen_wh.x),
		TO_OPENGL_COORDS_Y(pos.y, rect->screen_wh.y));
	rect->bottom_right = vec2f(
		TO_OPENGL_COORDS_X(pos.x + rect->wh.x, rect->screen_wh.x),
		TO_OPENGL_COORDS_Y(pos.y + rect->wh.y, rect->screen_wh.y));
}
void s_rect_show(StaticRectangle* rect) {
	glUseProgram(rect->program);

	int uniform_pos = glGetUniformLocation(rect->program, "u_pos"); 
	glUniform2f(uniform_pos, rect->top_left.x, rect->bottom_right.y);

	int uniform_color = glGetUniformLocation(rect->program, "u_color");
	glUniform4f(uniform_color, COLOR_UNPACK_F(rect->color));

	int uniform_border_color = glGetUniformLocation(rect->program, "u_border_color");
	int uniform_rect_info = glGetUniformLocation(rect->program, "u_rect_info");
	int uniform_border = glGetUniformLocation(rect->program, "u_border");
	if (rect->border_width > 0) {
		glUniform4f(uniform_border_color, COLOR_UNPACK_F(rect->border_color));
		glUniform4f(uniform_rect_info, rect->top_left.x, rect->top_left.y, rect->bottom_right.x, rect->bottom_right.y);
		glUniform2f(uniform_border, 2.0 * rect->border_width / (float)rect->screen_wh.x, 2.0 * rect->border_width / (float)rect->screen_wh.y);
	} else {
		glUniform2f(uniform_border, 0.0, 0.0);
	}

	glBindVertexArray(rect->vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rect->ebo);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	glUseProgram(0);
}
void s_rect_free(StaticRectangle* rect) {
	glDeleteVertexArrays(1, &rect->vao);
	glDeleteBuffers(1, &rect->vbo);
	glDeleteBuffers(1, &rect->ebo);
}

