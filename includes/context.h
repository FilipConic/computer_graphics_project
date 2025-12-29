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
#include <win_element.h>

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

void text_in_click_callback(Element* el, void* out) {
	*(Text**)out = &el->as.text_input.main;
}

ElementDescription elements[] = {
	{
		.type = TEXT_INPUT_ELEMENT,
		.scene_needed = SCENE_ACTIVE_ALWAYS,
		.click_callback = text_in_click_callback,
		.x = 180, .y = 100,
		.as.text_input = {
			.w = WIDTH / 2 - 180, .h = HEIGHT - 100,
			.color = COLOR_GRAY,
			.border_width = 5, .border_color = COLOR_WHITE,
		}
	},
	{
		.type = TEXT_ELEMENT,
		.scene_needed = SCENE_ACTIVE_ALWAYS,
		.x = 0, .y = 100, 
		.as.text = { .str = "f(x,y)=" }
	},
	{
		.type = TEXT_ELEMENT,
		.scene_needed = SCENE_ACTIVE_ALWAYS,
		.x = 0, .y = 0,
		.as.text = { .str = "The quick brown fox jumps over the lazy dog?!" }
	},
	{
		.type = TEXT_ELEMENT,
		.scene_needed = SCENE_FONT_CHANGE,
		.x = WIDTH / 2, .y = HEIGHT / 2, 
		.as.text = { .str = "Changing font!" }
	},
};
#define ELEMENT_COUNT (sizeof(elements)/sizeof(*elements))

typedef enum {
	PRORGAM_STATIC_RECT,
	PROGRAM_COUNT,
} ProgramType;
const char* programs[PROGRAM_COUNT * 2] = {
	"./resources/shaders/basic.vert", "./resources/shaders/basic.frag",
};

struct {
	RGFW_window* win;
	FontTTF font;
	Element elements[ELEMENT_COUNT];
	Text* curr_text_input;

	Scene curr_scene;

	uint32_t programs[PROGRAM_COUNT];

	CustomEvent* custom_events_queue;
	uint8_t ctrl_held;

	uint16_t curr_id;
	uint16_t curr_z;
	uint16_t click_field[WIDTH * HEIGHT];
} context;

void key_callback(RGFW_window* win, u8 key, u8 keyChar, RGFW_keymod keyMod, RGFW_bool repeat, RGFW_bool pressed) {
	RGFW_UNUSED(repeat);

	if (pressed) {
		if (key == RGFW_escape) {
			RGFW_window_setShouldClose(win, 1);
		} else if (keyMod == RGFW_modControl && keyChar == 'c') {
			cyx_ring_push(context.custom_events_queue, event_create(EVENT_SCENE_CHANGE, SCENE_FONT_CHANGE));
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
		uint16_t id = context.click_field[pos.x + pos.y * WIDTH];
		if (!id) {
			cyx_ring_push(context.custom_events_queue, event_create(EVENT_CLICK_EMTPY, 0));
		} else {
			cyx_ring_push(context.custom_events_queue, event_create(EVENT_ELEMENT_CLICK, id));
		}
	}
}

void context_setup_programs() {
	for (size_t i = 0; i < PROGRAM_COUNT; ++i) {
		printf("LOG:\tCompiling: [%s], [%s]\n", programs[2 * i], programs[2 * i + 1]);
		context.programs[i] = compile_program(programs[2 * i], programs[2 * i + 1]);
	}
}
int find_smallest_z(Element* els, uint16_t* ids) {
	int id = -1;
	for (size_t i = 0; i < ELEMENT_COUNT; ++i) {
		if (els[i].click_callback && cyx_array_find(ids, &els[i].id) == -1) {
			if (id == -1) {
				id = els[i].id;
			} else {
				if (els[id - 1].z > els[i].z) {
					id = els[i].id;
				}
			}
		}
	}
	// printf("{ ");
	// for (size_t i = 0; i < cyx_array_length(ids); ++i) {
	// 	if (i) printf(", ");
	// 	printf("%d", ids[i]);
	// }
	// printf("}\n");
	return id;
}
void context_setup_elements() {
	for (size_t i = 0; i < ELEMENT_COUNT; ++i) {
		assert(context.curr_id != UINT16_MAX);

		elements[i].id = ++context.curr_id;
		if (elements[i].z <= 0) {
			elements[i].z = ++context.curr_z;
		}

		if (elements[i].type == TEXT_INPUT_ELEMENT) {
			elements[i].as.text_input.program = context.programs[PRORGAM_STATIC_RECT];
			elements[i].as.text_input.font = &context.font;
		} else if (elements[i].type == TEXT_ELEMENT) {
			elements[i].as.text_input.font = &context.font;
		}

		context.elements[i] = element_create(&elements[i]);
	}

	uint16_t* sorted_z = cyx_array_new(uint16_t, .reserve = ELEMENT_COUNT, .cmp_fn = cyx_cmp_int16);
	int found = -1;
	while ((found = find_smallest_z(context.elements, sorted_z)) > 0) {
		cyx_array_append(sorted_z, found);
	}
	for (size_t i = 0; i < cyx_array_length(sorted_z); ++i) {
		element_fill_click_space(&context.elements[sorted_z[i] - 1], context.click_field);
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
	context.curr_scene = SCENE_ACTIVE_ALWAYS;
	context.curr_text_input = NULL;
	context_setup_programs();
	context_setup_elements();
	context.custom_events_queue = cyx_ring_new(CustomEvent);
}
void context_events() {
	RGFW_event event;
	while (RGFW_window_checkEvent(context.win, &event)) {
		if (event.type == RGFW_quit) { break; }  
	}

	cyx_ring_drain(val, context.custom_events_queue) {
		if (val->type == EVENT_ELEMENT_CLICK) {
			assert(val->value != 0);
			Element* el = &context.elements[val->value - 1];
			el->click_callback(el, &context.curr_text_input);
		} else if (val->type == EVENT_SCENE_CHANGE) {
			context.curr_scene = val->value;
		} else if (val->type == EVENT_CLICK_EMTPY) {
			context.curr_text_input = NULL;
		}
	}
}
void context_show() {
	glClearColor(0x18/255.0f, 0x18/255.0f, 0x18/255.0f, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	for (size_t i = 0; i < ELEMENT_COUNT; ++i) {
		element_show(&context.elements[i], context.curr_text_input, context.curr_scene);
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

