#include "color.h"
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

#include <shapes.h>

#define WIDTH 1600
#define HEIGHT 900

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

typedef enum {
	BUTTON_PRESS,
	TEXT_INPUT_CLICKED,
	FONT_CHANGE_POPUP,
} CustomEvent;

typedef enum {
	SCENE_ACTIVE_ALWAYS,
	SCENE_FONT_CHANGE,
} Scene;

typedef enum {
	INPUT_NOTHING,
	INPUT_TEXT_MAIN,
} InputState;

typedef enum {
	TEXT_INPUT_ELEMENT,
	TEXT_ELEMENT,
	BUTTON_ELEMENT,
} ElementType;

typedef struct {
	ElementType type;
	Scene scene_needed;
	union {
		struct {
			InputState state_needed;
			int border_width;
			int x, y, w, h;
			Color color;
			Color border_color;
		} text_input;
		struct {
			int x, y;
			const char* str;
		} text;
		struct {
			int x, y, w, h;
			const char* str;
		} button;
	} as;
} ElementDescription;

const ElementDescription elements[] = {
	{
		.type = TEXT_INPUT_ELEMENT,
		.scene_needed = SCENE_ACTIVE_ALWAYS,
		.as.text_input = {
			.x = 180, .y = 100,
			.w = WIDTH / 2 - 180, .h = HEIGHT - 100,
			.state_needed = INPUT_TEXT_MAIN, 
			.color = COLOR_GRAY,
			.border_width = 5, .border_color = COLOR_WHITE,
		}
	},
	{
		.type = TEXT_ELEMENT,
		.scene_needed = SCENE_ACTIVE_ALWAYS,
		.as.text = { .x = 0, .y = 100, .str = "f(x,y)=" }
	},
	{
		.type = TEXT_ELEMENT,
		.scene_needed = SCENE_ACTIVE_ALWAYS,
		.as.text = { .x = 0, .y = 0, .str = "Hello World!" }
	},
	{
		.type = TEXT_ELEMENT,
		.scene_needed = SCENE_FONT_CHANGE,
		.as.text = { .x = WIDTH / 2, .y = HEIGHT / 2, .str = "Changing font!" }
	},
};
#define ELEMENT_COUNT (sizeof(elements)/sizeof(*elements))

typedef struct {
	ElementType type;
	Scene scene_needed;
	union {
		struct {
			StaticRectangle rect;
			StaticRectangle cursor;
			Text main;
			InputState state_needed;
		} text_input;
		Text text;
		struct {
			int empty;
		} button;
	} as;
} Element;

const char* programs[] = {
	"./resources/shaders/basic.vert", "./resources/shaders/basic.frag",
};
#define PROGRAM_COUNT (sizeof(programs)/(2 * sizeof(*programs)))

struct {
	RGFW_window* win;
	FontTTF font;
	Element elements[ELEMENT_COUNT];
	Text* curr_text_input;

	InputState input_state;
	Scene curr_scene;

	uint32_t programs[PROGRAM_COUNT];

	CustomEvent* custom_events_queue;
	uint8_t ctrl_held;
} context;

void element_parse_rgfw_event(Element* el, RGFW_event* ev) {
	if (el->type == TEXT_INPUT_ELEMENT && (
		el->scene_needed == SCENE_ACTIVE_ALWAYS ||
		el->scene_needed == context.curr_scene) &&
		el->as.text_input.state_needed == context.input_state) {
		if (ev->type != RGFW_keyPressed) { return; }

		char c = RGFW_rgfwToKeyChar(ev->key.value);
		// printf("Keycode: %d\n", event.key.value);
		if (ev->key.value == 8) {
			text_pop(&el->as.text_input.main);
		} else if (ev->key.value == 10) {
			text_push(&el->as.text_input.main, '\n');
		} else {
			if (c != 0) {
				text_push(&el->as.text_input.main, c);
			}
		}
	}
}
#define CURSOR_PADDING vec2i((int)(context.font.size * 0.15), (int)(context.font.size * 0.3))
void element_show(Element* el) {
	if (el->scene_needed == context.curr_scene || el->scene_needed == SCENE_ACTIVE_ALWAYS) {
		switch (el->type) {
			case TEXT_ELEMENT: text_show(&el->as.text); break;
			case TEXT_INPUT_ELEMENT: {
				if (context.input_state == INPUT_TEXT_MAIN) {
					Vec2i new_cursor_pos = text_cursor_pos(&el->as.text_input.main, CURSOR_PADDING);
					if (!vec2i_eq(el->as.text_input.cursor.pos, new_cursor_pos)) {
						s_rect_set_pos(&el->as.text_input.cursor, new_cursor_pos);
					}
					s_rect_show(&el->as.text_input.cursor);
				}
				s_rect_show(&el->as.text_input.rect);
				text_show(&el->as.text_input.main);
			 } break;
			case BUTTON_ELEMENT:
			default: assert(0 && "UNREACHABLE");
		}
	}
}
void element_free(Element* el) {
	switch (el->type) {
		case TEXT_ELEMENT:
			text_free(&el->as.text);
			break;
		case TEXT_INPUT_ELEMENT:
			text_free(&el->as.text_input.main);
			s_rect_free(&el->as.text_input.rect);
			s_rect_free(&el->as.text_input.cursor);
			break;
		case BUTTON_ELEMENT:
		default: assert(0 && "UNREACHABLE");
	}
}
void element_parse_mouse(Element* el, Vec2i pos) {
	if (el->type == TEXT_INPUT_ELEMENT) {
		Text* text = &el->as.text_input.main;
		if (text->pos.x < pos.x &&
			pos.x < text->pos.x + text->width &&
			text->pos.y < pos.y &&
			pos.y < text->pos.y + text->height) {
			cyx_ring_push(context.custom_events_queue, TEXT_INPUT_CLICKED);
		}
	}
}

void key_callback(RGFW_window* win, u8 key, u8 keyChar, RGFW_keymod keyMod, RGFW_bool repeat, RGFW_bool pressed) {
	RGFW_UNUSED(keyChar);
	RGFW_UNUSED(repeat);

	if (pressed) {
		if (key == RGFW_escape) {
			RGFW_window_setShouldClose(win, 1);
		} else if (keyMod == RGFW_modControl && keyChar == 'c') {
			cyx_ring_push(context.custom_events_queue, FONT_CHANGE_POPUP);
		}
		if (context.curr_text_input) {
			if (key == RGFW_backSpace) {
				text_pop(context.curr_text_input);
			} else if (key == RGFW_enter) {
				text_push(context.curr_text_input, '\n');
			} else {
				if (keyChar != 0) { text_push(context.curr_text_input, keyChar); }
			}
		}
	}
}
void mouse_callback(RGFW_window* win, RGFW_mouseButton button, RGFW_bool scrolled){
	RGFW_UNUSED(scrolled);
	Vec2i pos = { 0 };
	RGFW_window_getMouse(win, &pos.x, &pos.y);
	if (button == RGFW_mouseLeft) {
		for (size_t i = 0; i < ELEMENT_COUNT; ++i) {
			element_parse_mouse(&context.elements[i], pos);
		}
	}
}

void context_setup_programs() {
	for (size_t i = 0; i < PROGRAM_COUNT; ++i) {
		printf("LOG:\tCompiling: [%s], [%s]\n", programs[2 * i], programs[2 * i + 1]);
		context.programs[i] = compile_program(programs[2 * i], programs[2 * i + 1]);
	}
}
void context_setup_elements() {
	for (size_t i = 0; i < ELEMENT_COUNT; ++i) {
		Element* new_element = &context.elements[i];
		new_element->type = elements[i].type;
		new_element->scene_needed = elements[i].scene_needed;
		switch (elements[i].type) {
			case TEXT_ELEMENT: {
				new_element->as.text = text_create(&context.font, WIDTH, HEIGHT,
					.x = elements[i].as.text.x, .y = elements[i].as.text.y, .is_static = 1, .text = elements[i].as.text.str);
			} break;
			case TEXT_INPUT_ELEMENT: {
				new_element->as.text_input.state_needed = elements[i].as.text_input.state_needed;
				new_element->as.text_input.main = text_create(&context.font, WIDTH, HEIGHT,
					.width = elements[i].as.text_input.w, .height = elements[i].as.text_input.h,
					.word_wrapping = 1,
					.x = elements[i].as.text_input.x + elements[i].as.text_input.border_width,
					.y = elements[i].as.text_input.y + elements[i].as.text_input.border_width);
				new_element->as.text_input.rect = s_rect_create(context.programs[0], WIDTH, HEIGHT,
					elements[i].as.text_input.x,
					elements[i].as.text_input.y,
					elements[i].as.text_input.w,
					elements[i].as.text_input.h,
					elements[i].as.text_input.color,
					.border_color = elements[i].as.text_input.border_color,
					.border_width = elements[i].as.text_input.border_width);
				new_element->as.text_input.cursor = s_rect_create(context.programs[0], WIDTH, HEIGHT,
					elements[i].as.text_input.x, elements[i].as.text_input.y,
					(int)(context.font.size * 0.8), (int)(context.font.size * 2.3),
					COLOR_WHITE);
			} break;
			case BUTTON_ELEMENT: { } break;
			default: assert(0 && "UNREACHABLE");
		}
	}
}
void context_setup(const char* file_path, int size) {
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
	RGFW_setKeyCallback(key_callback);
	RGFW_setMouseButtonCallback(mouse_callback);

	context.font = font_compile(file_path, size, WIDTH, HEIGHT);
	context.input_state = INPUT_NOTHING;
	context.curr_scene = SCENE_ACTIVE_ALWAYS;
	context_setup_programs();
	context_setup_elements();
	context.custom_events_queue = cyx_ring_new(CustomEvent);
}
void context_events() {
	RGFW_event event;
	while (RGFW_window_checkEvent(context.win, &event)) {
		if (event.type == RGFW_quit) { break; }  
		// for (size_t i = 0; i < ELEMENT_COUNT; ++i) {
		// 	element_parse_rgfw_event(&context.elements[i], &event);
		// }
	}

	cyx_ring_drain(val, context.custom_events_queue) {
		if (*val == TEXT_INPUT_CLICKED) {
			context.input_state = INPUT_TEXT_MAIN;
			context.curr_text_input = &context.elements[0].as.text_input.main;
		} else if (*val == FONT_CHANGE_POPUP) {
			context.curr_scene = SCENE_FONT_CHANGE;
		}
	}
}
void context_show() {
	glClearColor(0x18/255.0f, 0x18/255.0f, 0x18/255.0f, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	for (size_t i = 0; i < ELEMENT_COUNT; ++i) {
		element_show(&context.elements[i]);
	}
}
void context_cleanup() {
	cyx_ring_free(context.custom_events_queue);

	for (size_t i = 0; i < ELEMENT_COUNT; ++i) {
		element_free(&context.elements[i]);
	}

	for (size_t i = 0; i < PROGRAM_COUNT; ++i) {
		glDeleteProgram(context.programs[i]);
	}

	font_free(&context.font);
	RGFW_window_close(context.win);
}
