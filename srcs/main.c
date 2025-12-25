#include <X11/extensions/render.h>

#include <stddef.h>
#include <stdio.h>
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

void esc_callback(RGFW_window* win, u8 key, u8 keyChar, RGFW_keymod keyMod, RGFW_bool repeat, RGFW_bool pressed) {
	RGFW_UNUSED(keyChar);
	RGFW_UNUSED(keyMod);
	RGFW_UNUSED(repeat);
	if (key == RGFW_escape && pressed) { RGFW_window_setShouldClose(win, 1); }
}

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

int main() {
	srand(time(NULL));

	{ // Setting up OpenGL hints
		RGFW_glHints* hints = RGFW_getGlobalHints_OpenGL();
		hints->major = 3;
		hints->minor = 3;
		hints->profile = RGFW_glCore;
		RGFW_setGlobalHints_OpenGL(hints);
	}

	RGFW_window* win = RGFW_createWindow("Terminal", 0, 0, WIDTH, HEIGHT, RGFW_windowCenter | RGFW_windowOpenGL | RGFW_windowNoResize);
	RGFW_window_swapInterval_OpenGL(win, 0);

	GLenum err = glewInit();
	if (err != GLEW_OK) {
		fprintf(stderr, "ERROR:\tUnable to initialize GLEW!\nLOG: %s\n", glewGetErrorString(err));
		return 1;
	}
	printf("LOG:\tOpenGL version supported: %s\n", glGetString(GL_VERSION));

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	RGFW_setKeyCallback(esc_callback);

	// FontTTF font = font_compile("./resources/fonts/UbuntuMonoNerdFontMono-Regular.ttf", 20, WIDTH, HEIGHT);
	FontTTF font = font_compile("./resources/fonts/JetBrainsMonoNerdFont-Bold.ttf", 20, WIDTH, HEIGHT);
	Sentence input = sentence_create(WIDTH, HEIGHT, .y = 600);

	// glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	while (!RGFW_window_shouldClose(win)) {
		updateTimer();

		RGFW_event event;
		while (RGFW_window_checkEvent(win, &event)) {
			if (event.type == RGFW_quit) {
				break;
			} else if (event.type == RGFW_keyPressed) {
				char c = RGFW_rgfwToKeyChar(event.key.value);
				if (event.key.value != 8) {
					if (c != 0) {
						sentence_push(&input, &font, c);
					}
				} else {
					sentence_pop(&input);
				}
			}
		}
		
		glClearColor(0x18/255.0f, 0x18/255.0f, 0x18/255.0f, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);

		sentence_show(&input);

		RGFW_window_swapBuffers_OpenGL(win);
		glFlush();
	}
	sentence_free(&input);

	RGFW_window_close(win);

	font_free(&font);
	return 0;
}
