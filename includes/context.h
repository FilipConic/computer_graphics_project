#include <X11/extensions/render.h>

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <float.h>

#define RGFW_IMPLEMENTATION
#define RGFW_OPENGL
#include <RGFW.h>
#include <glew.h>

#define CYLIBX_IMPLEMENTATION
#include <cylibx.h>
#include <font.h>

#define WIDTH 1600
#define HEIGHT 900

typedef enum {
	BUTTON_PRESS,
	TEXT_INPUT_CLICKED,
} CustomEvents;

typedef enum {
	STATE_ACTIVE_ALWAYS,
} CurrentVisibilityState;

typedef enum {
	INPUT_NOTHING,
	INPUT_TEXT_MAIN,
} CurrentInputState;

struct {
	double prev_time;
	double collection;
	double dt;

	size_t frames;
	char log;
} timer = { .log = 0 };
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

typedef struct {
	int x, y;
	const char* text;
} TextElement;
const TextElement words[] = {
	{ 0, 0, "Hello World!" },
	{ 500, 0, "How are you doing today?" },
};
#define TEXT_ELEMENT_COUNT (sizeof(words)/sizeof(*words))

struct {
	RGFW_window* win;
	FontTTF font;
	Text input;
	Text words[TEXT_ELEMENT_COUNT];

	CurrentInputState input_state;
	CurrentVisibilityState visibility_state;

	CustomEvents* custom_events_queue;
} context;

void esc_callback(RGFW_window* win, u8 key, u8 keyChar, RGFW_keymod keyMod, RGFW_bool repeat, RGFW_bool pressed) {
	RGFW_UNUSED(keyChar);
	RGFW_UNUSED(keyMod);
	RGFW_UNUSED(repeat);
	if (key == RGFW_escape && pressed) { RGFW_window_setShouldClose(win, 1); }
}
void mouse_callback(RGFW_window* win, RGFW_mouseButton button, RGFW_bool scrolled){
	RGFW_UNUSED(scrolled);
	Vec2i pos = { 0 };
	RGFW_window_getMouse(win, &pos.x, &pos.y);
	if (button == RGFW_mouseLeft) {
		if (context.input.pos.x < pos.x &&
			pos.x < context.input.pos.x + context.input.width &&
			context.input.pos.y < pos.y &&
			pos.y < context.input.pos.y + context.input.height) {
			cyx_ring_push(context.custom_events_queue, TEXT_INPUT_CLICKED);
		}
	}
}

void context_setup(const char* file_path, int size, int input_x, int input_y) {
	assert(size > 0);

	srand(time(NULL));

	{ // Setting up OpenGL hints
		RGFW_glHints* hints = RGFW_getGlobalHints_OpenGL();
		hints->major = 3;
		hints->minor = 3;
		hints->profile = RGFW_glCore;
		RGFW_setGlobalHints_OpenGL(hints);
	}

	context.win = RGFW_createWindow("Terminal", 0, 0, WIDTH, HEIGHT, RGFW_windowCenter | RGFW_windowOpenGL | RGFW_windowNoResize);
	RGFW_window_swapInterval_OpenGL(context.win, 0);

	GLenum err = glewInit();
	if (err != GLEW_OK) {
		fprintf(stderr, "ERROR:\tUnable to initialize GLEW!\nLOG: %s\n", glewGetErrorString(err));
		exit(1);
	}
	printf("LOG:\tOpenGL version supported: %s\n", glGetString(GL_VERSION));

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	RGFW_setKeyCallback(esc_callback);
	RGFW_setMouseButtonCallback(mouse_callback);

	context.font = font_compile(file_path, size, WIDTH, HEIGHT);
	context.input = text_create(&context.font, WIDTH, HEIGHT, WIDTH / 2, HEIGHT,
		.word_wrapping = 1, .x = input_x, .y = input_y);
	for (size_t i = 0; i < TEXT_ELEMENT_COUNT; ++i) {
		context.words[i] = text_create(&context.font, WIDTH, HEIGHT, WIDTH, HEIGHT,
			.x = words[i].x, .y = words[i].y, .is_static = 1, .text = words[i].text);
	}
	context.custom_events_queue = cyx_ring_new(CustomEvents);
}
void context_events() {
	RGFW_event event;
	while (RGFW_window_checkEvent(context.win, &event)) {
		if (event.type == RGFW_quit) {
			break;
		} else if (event.type == RGFW_keyPressed) {
			char c = RGFW_rgfwToKeyChar(event.key.value);
			// printf("Keycode: %d\n", event.key.value);
			if (event.key.value == 8) {
				text_pop(&context.input);
			} else if (event.key.value == 10) {
				text_push(&context.input, '\n');
			} else {
				if (c != 0) {
					text_push(&context.input, c);
				}
			}
		} 
	}

	cyx_ring_drain(val, context.custom_events_queue) {
		printf("Custom event! %d\n", *val);
	}
}
void context_show() {
	glClearColor(0x18/255.0f, 0x18/255.0f, 0x18/255.0f, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	for (size_t i = 0; i < TEXT_ELEMENT_COUNT; ++i) {
		text_show(&context.words[i]);
	}
	text_show(&context.input);
}
void context_cleanup() {
	cyx_ring_free(context.custom_events_queue);

	for (size_t i = 0; i < TEXT_ELEMENT_COUNT; ++i) {
		text_free(&context.words[i]);
	}
	text_free(&context.input);
	font_free(&context.font);
	RGFW_window_close(context.win);
}
