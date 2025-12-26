#include <context.h>

int main() {
	// context_setup("./resources/fonts/UbuntuMonoNerdFontMono-Regular.ttf", 20);
	context_setup("./resources/fonts/JetBrainsMonoNerdFont-Bold.ttf", 20);

	// glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	while (!RGFW_window_shouldClose(context.win)) {
		updateTimer();
		context_events();

		context_show();

		RGFW_window_swapBuffers_OpenGL(context.win);
		glFlush();
	}
	context_cleanup();

	return 0;
}
