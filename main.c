#include <math.h>
#include <stdio.h>
#include <time.h>
#define RGFW_IMPLEMENTATION
#define RGFW_OPENGL
#include <RGFW.h>
#include <glew.h>

#define CYLIBX_IMPLEMENTATION
#include <cylibx.h>

#define WIDTH 800
#define HEIGHT 600

void esc_callback(RGFW_window* win, u8 key, u8 keyChar, RGFW_keymod keyMod, RGFW_bool repeat, RGFW_bool pressed) {
	RGFW_UNUSED(keyChar);
	RGFW_UNUSED(keyMod);
	RGFW_UNUSED(repeat);
	if ((key == RGFW_escape || key == RGFW_q) && pressed) { RGFW_window_setShouldClose(win, 1); }
}

#define check_shader_compile_status(shader) do { \
	int success; \
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success); \
	if (!success) { \
		char info_log[512]; \
		glGetShaderInfoLog(shader, 512, NULL, info_log);\
		fprintf(stderr, "ERROR:\tUnable to compile shader!\nLOG:\t%s\n", info_log); \
	} \
} while(0)
#define check_program_compile_status(program) do { \
	int success; \
	glGetProgramiv(program, GL_COMPILE_STATUS, &success); \
	if (!success) { \
		char info_log[512]; \
		glGetProgramInfoLog(program, 512, NULL, info_log);\
		fprintf(stderr, "ERROR:\tUnable to compile program!\nLOG:\t%s\n", info_log); \
	} \
} while(0)

#define rand0_1 ((double)rand() / RAND_MAX)

typedef struct { float r, g, b, a; } fColor;
#define rand_color_f() ((fColor){ .r = rand0_1, .g = rand0_1, .b = rand0_1, .a = rand0_1 })
#define set_uniform_color(uniform, color) glUniform4f(uniform, color.r, color.g, color.b, color.a)

typedef struct {
	u32 vao, vbo, ebo;
	u32 program;
	u32 count;

	float x, y, r;
	float vx, vy;
	fColor clr;
} DrawnObject;

DrawnObject gen_circle(int pos_x, int pos_y, int radius, size_t step_count) {
	assert(step_count > 2);
	float* vertices = cyx_array_new(float);
	float a = 0;
	float step = 2 * M_PI / step_count;
	float rx = (float)radius / WIDTH * 2;
	float ry = (float)radius / HEIGHT * 2;

	cyx_array_append_mult(vertices, 0.0f, 0.0f, 0.0f);
	for (size_t i = 0; i < step_count; ++i, a += step) {
		cyx_array_append_mult(vertices, rx * cosf(a), ry * sinf(a), 0.0f);
	}

	u32* indicies = cyx_array_new(u32);
	cyx_array_append_mult(indicies, 0, step_count, 1);
	for (size_t i = 2; i <= step_count; ++i) {
		cyx_array_append_mult(indicies, 0, i - 1, i);
	}

	DrawnObject obj = { .count = cyx_array_length(indicies) };
	obj.x = pos_x;
	obj.y = pos_y;
	obj.vx = rand0_1;
	obj.vy = rand0_1;
	obj.r = radius;

	glGenVertexArrays(1, &obj.vao);
	glBindVertexArray(obj.vao);

	glGenBuffers(1, &obj.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, obj.vbo);
	glBufferData(GL_ARRAY_BUFFER, cyx_array_length(vertices) * sizeof(float), vertices, GL_STATIC_DRAW);

	glGenBuffers(1, &obj.ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj.ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, cyx_array_length(indicies) * sizeof(u32), indicies, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), NULL);
	glEnableVertexAttribArray(0);

	cyx_array_free(indicies);
	cyx_array_free(vertices);
	return obj;
}
u32 compile_program(const char* vert_file, const char* frag_file) {
	char* source = cyx_str_from_file(vert_file);
	u32 vert_shader = glCreateShader(GL_VERTEX_SHADER);
	i32 len = cyx_str_length(source);
	glShaderSource(vert_shader, 1, (const char* const*)&source, &len);
	glCompileShader(vert_shader);
	check_shader_compile_status(vert_shader);
	cyx_str_free(source);

	source = cyx_str_from_file(frag_file);
	u32 frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
	len = cyx_str_length(source);
	glShaderSource(frag_shader, 1, (const char* const*)&source, &len);
	glCompileShader(frag_shader);
	check_shader_compile_status(frag_shader);
	cyx_str_free(source);

	u32 program = glCreateProgram();
	glAttachShader(program, vert_shader);
	glAttachShader(program, frag_shader);
	glLinkProgram(program);
	check_program_compile_status(program);

	glDeleteShader(vert_shader);
	glDeleteShader(frag_shader);
	return program;
}

struct {
	double prev_time;
	double collection;
	double dt;
	size_t frames;
	char log;
} timer = { .log = 1 };
void updateTimer() {
	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC, &t);
	double curr_time = t.tv_sec + t.tv_nsec * 1e-9;
	timer.dt = curr_time - timer.prev_time;
	timer.prev_time = curr_time;
	timer.collection += timer.dt;

	timer.frames++;
	if (timer.collection > 1.) {
		if (timer.log) {
			printf("FPS:\t%zu\n", timer.frames);
		}
		timer.frames = 0;
		timer.collection = 0.0;
	}
}

#define SPEED 1000
void circle_update_pos(DrawnObject* circle) {
	float next_x = circle->x + SPEED * circle->vx * timer.dt;
	float next_y = circle->y + SPEED * circle->vy * timer.dt;
	if (next_x < circle->r) {
		circle->vx *= -1;
		next_x = circle->r;
	} else if (next_x > WIDTH - circle->r) {
		circle->vx *= -1;
		next_x = WIDTH - circle->r;
	}
	if (next_y < circle->r) {
		circle->vy *= -1;
		next_y = circle->r;
	} else if (next_y > HEIGHT - circle->r) {
		circle->vy *= -1;
		next_y = HEIGHT - circle->r;
	}
	circle->x = next_x;
	circle->y = next_y;
}

int main() {
	RGFW_glHints* hints = RGFW_getGlobalHints_OpenGL();
	hints->major = 3;
	hints->minor = 3;
	hints->profile = RGFW_glCore;
	RGFW_setGlobalHints_OpenGL(hints);

	RGFW_window* win = RGFW_createWindow("Terminal", 0, 0, WIDTH, HEIGHT, RGFW_windowCenter | RGFW_windowOpenGL | RGFW_windowNoResize);
	RGFW_window_swapInterval_OpenGL(win, 0);

	GLenum err = glewInit();
	if (err != GLEW_OK) {
		fprintf(stderr, "ERROR:\tUnable to initialize GLEW!\nLOG: %s\n", glewGetErrorString(err));
		return 1;
	}
	printf("LOG:\tOpenGL version: %s\n", glGetString(GL_VERSION));

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	RGFW_setKeyCallback(esc_callback);

	srand(time(NULL));
	u32 program = compile_program("./resources/shaders/basic.vert", "./resources/shaders/basic.frag");
	DrawnObject* objs = cyx_array_new(DrawnObject);

	for (size_t i = 0; i < 50; ++i) {
		cyx_array_append(objs, gen_circle(rand0_1 * WIDTH, rand0_1 * HEIGHT, rand0_1 * 75 + 25, 20));
		objs[i].program = program;
		fColor clr = rand_color_f();
		clr.a = 0.5;
		objs[i].clr = clr;
	}

	while (!RGFW_window_shouldClose(win)) {
		RGFW_event event;
		while (RGFW_window_checkEvent(win, &event)) {
			if (event.type == RGFW_quit) { break; }
		}
		
		glClearColor(0x18/255.0f, 0x18/255.0f, 0x18/255.0f, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);

		cyx_array_foreach(circle, objs) {
			circle_update_pos(circle.value);

			int uniformColor = glGetUniformLocation(circle.value->program, "inColor");
			int uniformPos = glGetUniformLocation(circle.value->program, "screenPos");
			glUseProgram(circle.value->program);

			set_uniform_color(uniformColor, circle.value->clr);
			glUniform2f(uniformPos, circle.value->x / WIDTH * 2 - 1, circle.value->y / HEIGHT * 2 - 1);

			glBindVertexArray(circle.value->vao);
			glDrawElements(GL_TRIANGLES, circle.value->count, GL_UNSIGNED_INT, 0);
		}

		RGFW_window_swapBuffers_OpenGL(win);

		glUseProgram(0);
		glBindVertexArray(0);
		glFlush();

		updateTimer();
	}

	RGFW_window_close(win);
	return 0;
}
