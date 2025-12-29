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
#include <color.h>

#include <shapes.h>

#define WIDTH 1600
#define HEIGHT 900

enum ProgramNames {
	PROGRAM_RECT,
	PROGRAM_COUNT,
};
const char* programs[2 * PROGRAM_COUNT] = {
	"./resources/shaders/basic.vert", "./resources/shaders/basic.frag",
};

typedef enum {
	SCENE_DATA_STRING,
} SceneDataType;
typedef struct {
	SceneDataType type;
	union {
		char* str;
	} as;
} SceneData;

typedef enum {
	SCENE_MAIN,
	SCENE_COUNT,
} SceneName;

struct {
	SceneData* data;	
	void (*show)();
} scenes[SCENE_COUNT] = { 0 };

typedef struct {
	enum {
		EVENT_ELEMENT_CLICK,
	} type;
	int value;
} CustomEvent;

#define custom_event(t, v) ((CustomEvent){ .type = (t), .value = (v) })

struct {
	RGFW_window* win;
	FontTTF* fonts;
	uint32_t programs[PROGRAM_COUNT];

	SceneName curr_scene;
	CustomEvent* event_queue;
} context;

void __font_free(void* val) { font_free(val); }
void context_compile_programs() {
	for (size_t i = 0; i < PROGRAM_COUNT; ++i) {
		context.programs[i] = compile_program(programs[2 * i], programs[2 * i + 1]);
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
	// RGFW_setKeyCallback(key_callback);
	// RGFW_setMouseButtonCallback(mouse_callback);

	context.fonts = cyx_array_new(FontTTF, .defer_fn = __font_free);
	cyx_array_append(context.fonts, font_compile(file_path, size, WIDTH, HEIGHT));

	context.curr_scene = SCENE_MAIN;
	context_compile_programs();
	context.event_queue = cyx_ring_new(CustomEvent);
}
void context_events() {
	RGFW_event event;
	while (RGFW_window_checkEvent(context.win, &event)) {
		if (event.type == RGFW_quit) { break; }  
	}

	cyx_ring_drain(val, context.event_queue) {
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

	scenes[context.curr_scene].show();
}
void context_cleanup() {
	cyx_ring_free(context.event_queue);

	for (size_t i = 0; i < ELEMENT_COUNT; ++i) {
		element_free(&context.elements[i]);
	}

	for (size_t i = 0; i < PROGRAM_COUNT; ++i) {
		glDeleteProgram(context.programs[i]);
	}

	cyx_array_free(context.fonts);
	RGFW_window_close(context.win);
}
