#include <immediate.h>

int main() {
	Context ctx = { 0 };
	// context_setup("./resources/fonts/UbuntuMonoNerdFontMono-Bold.ttf", 20);
	context_setup(&ctx);
	// context_setup("./resources/fonts/JetBrainsMonoNerdFont-Bold.ttf", 20);

	// glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	while (!RGFW_window_shouldClose(ctx.win)) {
		context_events(&ctx);

		context_show(&ctx);

		RGFW_window_swapBuffers_OpenGL(ctx.win);
		glFlush();
	}
	context_cleanup(&ctx);

	return 0;
}
