#include <color.h>
#include <vec2.h>
#include <mat.h>

#include <shapes.h>

#define CYLIBX_ALLOC
#include <cylibx.h>

Rectangle __rect_create(struct __RectangleCreateParams params) {
	Rectangle rect = {
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
void rect_show(Rectangle* rect, int x, int y, int w, int h, int screen_w, int screen_h) {
	glDisable(GL_DEPTH_TEST);
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
	glEnable(GL_DEPTH_TEST);
}
void rect_free(Rectangle* rect) {
	glDeleteVertexArrays(1, &rect->vao);
	glDeleteBuffers(1, &rect->vbo);
	glDeleteBuffers(1, &rect->ebo);
}

Shape3D shape3d_create(uint32_t program, Color color, float scale, uint32_t* indices, float* triangle_coords) {
	assert(scale > 0);
	Shape3D ret = {
		.color = color,

		.program = program,
		.indicies_count = cyx_array_length(indices),
		.scale = scale,
	};
	glGenVertexArrays(1, &ret.vao);
	glBindVertexArray(ret.vao);

	glGenBuffers(1, &ret.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, ret.vbo);
	glBufferData(GL_ARRAY_BUFFER, cyx_array_length(triangle_coords) * sizeof(float), triangle_coords, GL_STATIC_DRAW);

	glGenBuffers(1, &ret.ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ret.ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, cyx_array_length(indices) * sizeof(uint32_t), indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), NULL);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	return ret;
}
void shape3d_show(Shape3D* shape, int x, int y, int w, int h, int screen_w, int screen_h) {
	if (shape->face_cull) {
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glFrontFace(GL_CCW);
	}
	if (shape->depth_test) {
		glEnable(GL_DEPTH_TEST);
	} else {
		glDisable(GL_DEPTH_TEST);
	}
glViewport(x, y, w, h);
	glUseProgram(shape->program);

	int uniform_proj = glGetUniformLocation(shape->program, "u_proj"); 
	int uniform_model = glGetUniformLocation(shape->program, "u_model");
	int uniform_view = glGetUniformLocation(shape->program, "u_view");

	int uniform_color = glGetUniformLocation(shape->program, "u_color");
	int uniform_light_color = glGetUniformLocation(shape->program, "u_light_color");

	int uniform_light_pos = glGetUniformLocation(shape->program, "u_light_pos");
	int uniform_view_pos = glGetUniformLocation(shape->program, "u_view_pos");
	int uniform_shininess = glGetUniformLocation(shape->program, "u_shininess");
	int uniform_reflectivity = glGetUniformLocation(shape->program, "u_reflectivity");

	glUniform4f(uniform_color, COLOR_UNPACK_F(shape->color));
	glUniform3f(uniform_light_pos, shape->light_pos.x, shape->light_pos.y, shape->light_pos.z);
	glUniform4f(uniform_light_color, COLOR_UNPACK_F(shape->light_color));

	glUniform3f(uniform_view_pos, shape->camera.x, shape->camera.y, shape->camera.z);
	glUniform1f(uniform_shininess, shape->shininess);
	glUniform1f(uniform_reflectivity, shape->reflectivity);

	Mat4 model = mat4_model(shape->pos, 0, vec4(0, 1, 0), vec4(1.f/shape->scale, 1.f/shape->scale, 1.f/shape->scale));
	Mat4 proj = mat4_perspective(60, (float)w / h, 1, 200);
	Mat4 view = mat4_look_at(shape->camera, vec4(0, 0, 0), vec4(0, 1, 0));

	glUniformMatrix4fv(uniform_proj, 1, GL_FALSE, proj.data);
	glUniformMatrix4fv(uniform_model, 1, GL_FALSE, model.data);
	glUniformMatrix4fv(uniform_view, 1, GL_FALSE, view.data);

	glBindVertexArray(shape->vao);
	glDrawElements(GL_TRIANGLES, shape->indicies_count, GL_UNSIGNED_INT, 0);

	glBindVertexArray(0);
	glUseProgram(0);
	glViewport(0, 0, screen_w, screen_h);
	if (shape->face_cull) {
		glDisable(GL_CULL_FACE);
	}
	if (shape->depth_test) {
		glDisable(GL_DEPTH_TEST);
	}
}
void shape3d_free(Shape3D* shape) {
	glDeleteVertexArrays(1, &shape->vao);
	glDeleteBuffers(1, &shape->vbo);
	glDeleteBuffers(1, &shape->ebo);
}
