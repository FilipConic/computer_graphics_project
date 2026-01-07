#include <color.h>
#include <vec2.h>
#include <shapes.h>
// #include <string.h>
#include <stdio.h>

StaticRectangle __rect_create(struct __StaticRectangleCreateParams params) {
	StaticRectangle rect = {
		.program = params.__program,
		.color = params.__color,

		.border_color = params.border_color,
		.border_width = params.border_width,
	};

	float vertices[] = {
		-1.0f,  1.0f,
		-1.0f, -1.0f,
		 1.0f, -1.0f,
		 1.0f,  1.0f,
	};

	uint32_t triangles[] = {
		0, 1, 2,
		0, 2, 3
	};

	glGenVertexArrays(1, &rect.vao);
	glBindVertexArray(rect.vao);

	glGenBuffers(1, &rect.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, rect.vbo);
	glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(float), vertices, GL_STATIC_DRAW);

	glGenBuffers(1, &rect.ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rect.ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(uint32_t), triangles, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), NULL);
	glEnableVertexAttribArray(0);

	return rect;
}
void rect_show(StaticRectangle* rect, int x, int y, int w, int h, int screen_w, int screen_h) {
	glUseProgram(rect->program);

	// int uniform_scaling = glGetUniformLocation(rect->program, "u_scale"); 
	// int uniform_translation = glGetUniformLocation(rect->program, "u_translate"); 
	int uniform_projection = glGetUniformLocation(rect->program, "u_proj"); 
	int uniform_model = glGetUniformLocation(rect->program, "u_model");
	int uniform_color = glGetUniformLocation(rect->program, "u_color");
	int uniform_border = glGetUniformLocation(rect->program, "u_border");
	int uniform_border_color = glGetUniformLocation(rect->program, "u_border_color");

	float proj[16] = { 
		[0]  = 2.0f / screen_w, // 2 / (right - left)
		[5]  = 2.0f / -screen_h, // 2 / (top - bottom)
		[10] = -1.0f, // -2 / (far - near)
		[12] = -1.0f, // -(right + left) / (right - left)
		[13] = 1.0f, // -(top + bottom) / (top - bottom)
		// [14] = 0.0f, // -(far + near) / (far - near)
		[15] = 1.0f, // 1
	};
	glUniformMatrix4fv(uniform_projection, 1, GL_FALSE, proj);

	float model[16] = {
		[0]  = w / 2.f,
		[5]  = h / 2.f,
		[10] = 1.f,
		[12] = x + w / 2.f,
		[13] = y + h / 2.f,
		[15] = 1.f,
	};
	glUniformMatrix4fv(uniform_model, 1, GL_FALSE, model);

	glUniform4f(uniform_color, COLOR_UNPACK_F(rect->color));

	if (rect->border_width > 0) {
		glUniform4f(uniform_border_color, COLOR_UNPACK_F(rect->border_color));
		glUniform2f(uniform_border, rect->border_width * 2.f / w, rect->border_width * 2.f / h);
	} else {
		glUniform2f(uniform_border, 0, 0);
	}

	glBindVertexArray(rect->vao);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

	glBindVertexArray(0);
	glUseProgram(0);
}
void rect_free(StaticRectangle* rect) {
	glDeleteVertexArrays(1, &rect->vao);
	glDeleteBuffers(1, &rect->vbo);
	glDeleteBuffers(1, &rect->ebo);
}

