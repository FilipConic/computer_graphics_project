#include "vec2.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

struct {
	double prev_time;
	double collection;
	double dt;

	size_t frames;
	char log;
	char fps_cap;
} timer = { .log = 0, .fps_cap = 0 };
void updateTimer() {
	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC, &t);
	double curr_time = t.tv_sec + t.tv_nsec * 1e-9;
	timer.dt = curr_time - timer.prev_time;
	timer.prev_time = curr_time;
	timer.collection += timer.dt;

	timer.frames++;
	if (timer.fps_cap) {
		if (timer.frames >= 75) {
			usleep((1. - timer.collection) * 1e6);
			timer.collection = 1.1;
		}
	}
	if (timer.collection > 1.) {
		if (timer.log) {
			printf("FPS:\t%zu\n", timer.frames);
		}
		timer.frames = 0;
		timer.collection = 0.0;
	}
}


#define check_shader_compile_status(shader, shader_name) do { \
	int success; \
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success); \
	if (!success) { \
		char info_log[512]; \
		glGetShaderInfoLog(shader, 512, NULL, info_log);\
		fprintf(stderr, "ERROR:\tUnable to compile shader [\"%s\"]!\nLOG:\t%s\n", shader_name, info_log); \
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
uint32_t compile_program(const char* vert_file, const char* frag_file) {
	char* source = cyx_str_from_file(vert_file);
	uint32_t vert_shader = glCreateShader(GL_VERTEX_SHADER);
	int32_t len = cyx_str_length(source);
	glShaderSource(vert_shader, 1, (const char* const*)&source, &len);
	glCompileShader(vert_shader);
	check_shader_compile_status(vert_shader, vert_file);
	cyx_str_free(source);

	source = cyx_str_from_file(frag_file);
	uint32_t frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
	len = cyx_str_length(source);
	glShaderSource(frag_shader, 1, (const char* const*)&source, &len);
	glCompileShader(frag_shader);
	check_shader_compile_status(frag_shader, frag_file);
	cyx_str_free(source);

	uint32_t program = glCreateProgram();
	glAttachShader(program, vert_shader);
	glAttachShader(program, frag_shader);
	glLinkProgram(program);
	check_program_compile_status(program);

	glDeleteShader(vert_shader);
	glDeleteShader(frag_shader);
	return program;
}

enum ProgramNames {
	PROGRAM_FONT,
	PROGRAM_RECT,
	PROGRAM_COUNT,
};
uint32_t compiled_programs[PROGRAM_COUNT] = { 0 };
const char* programs[2 * PROGRAM_COUNT] = {
	"./resources/shaders/font.vert", "./resources/shaders/font.frag",
	"./resources/shaders/basic.vert", "./resources/shaders/basic.frag",
};


typedef struct {
	enum {
		EVENT_CLICK,
		EVENT_CLICKED_ID,
	} type;
	void* value;
} CustomEvent;
#define TO_VOID(val) (assert(sizeof(*(val)) <= 8), *(void**)(val))
#define FROM_VOID(type, val) (assert(sizeof(type) <= 8), *(type*)&(val))
#define CUSTOM_EVENT(t, v) ((CustomEvent){ .type = (t), .value = TO_VOID(v) })

typedef struct Context Context;
typedef struct SceneData SceneData;

typedef enum {
	DATA_TEXT,
	DATA_STATIC_TEXT,
	DATA_RECT,
	DATA_VALUES,
} SceneDataType;
typedef void(*ClickCallback)(Context*, SceneData*);
struct SceneData {
	int id;
	SceneDataType type;
	union {
		struct {
			char* text;
			StaticRectangle cursor;
			Color clr;
			char is_selected;
		} dyn_text;
		struct {
			char* text;
			Color clr;
		} static_text;
		StaticRectangle rect;
		void* values;
	} as;
	ClickCallback click;
};


#define SCENE_DATA_POOL_SIZE 1024
struct {
	int len;
	SceneData data[SCENE_DATA_POOL_SIZE];
} scene_data_pool;
void scene_pool_cleanup() {
	for (int i = 0; i < scene_data_pool.len; ++i) {
		SceneData* s = &scene_data_pool.data[i];
		switch (s->type) {
			case DATA_TEXT:
				cyx_str_free(s->as.dyn_text.text);
				break;
			case DATA_STATIC_TEXT:
				cyx_str_free(s->as.static_text.text);
				break;
			case DATA_RECT:
				rect_free(&s->as.rect);
				break;
			case DATA_VALUES: break;
			default: assert(0 && "UNREACHABLE");
		}
	}
}

typedef struct {
	char* key;
	size_t value;
} IdSceneData;

typedef enum {
	SCENE_MAIN,
	SCENE_COUNT,
} SceneName;

typedef struct Scene Scene;
struct Scene {
	int cleaned_up;
	IdSceneData* data;
	void (*setup)(Scene*, Context*);
	void (*show)(Scene*, Context*);
	void (*cleanup)(Scene*, Context*);
};

enum GridType {
	GRID_VERTICAL,
	GRID_HORIZONTAL,
	GRID_TYPE_COUNT,
};
struct ZBuffer {
	int x;
	int y;
	int w;
	int h;

	int z;
	int id;
};
int __zbuf_cmp(const void* const a, const void* const b) { 
	const struct ZBuffer* const za = a;
	const struct ZBuffer* const zb = b;
	if (za->z < zb->z) {
		return -1;
	} else if (za->z > zb->z) {
		return 1;
	} else {
		return 0;
	}
}

#define GRID_SIZE 16
struct Grid {
	Scene* scene;
	Context* ctx;

	enum GridType type;
	int x, y, w, h;
	int len;
	int sizes[GRID_SIZE];

	int max_z;
	struct ZBuffer* z_buffer;
};
#define GRID_COUNT 64
int __grid_len = 1;
struct Grid grids[GRID_COUNT] = {
	{
		.type = GRID_VERTICAL,
		.x = 0, .y = 0,
		.w = WIDTH, .h = HEIGHT,
		.len = 1, .sizes = { WIDTH },
		.max_z = 0,
		.z_buffer = NULL,
	},
};

typedef struct {
	char* name;
	int size;
} NameSizePair;
typedef struct {
	NameSizePair key;
	FontTTF value;
} FontKV;

int __name_size_eq(const void* const a, const void* const b) {
	const NameSizePair* const p1 = a;
	const NameSizePair* const p2 = b;
	return cyx_eq_str(p1->name, p2->name) && p1->size == p2->size;
}
size_t __name_size_hash(const void* const val) {
	const NameSizePair* const p = val;
	return cyx_hash_str(p->name) ^ cyx_hash_int32(&p->size);
}
void __name_size_free(void* val) {
	NameSizePair* p = val;
	cyx_str_free(p->name);
}
void __font_free(void* val) {
	font_free(val);
}

#define FONT_MAX_COUNT 10
struct Context {
	struct {
		enum {
			STATE_NONE,
			STATE_TEST_CLICK,
		} type;
		void* value;
	} state;

	int curr_input;
	struct {
		uint8_t shift_held;
		uint8_t ctrl_held;
		uint8_t alt_held;
	} key_info;

	Vec2i wh;

	RGFW_window* win;

	char* font_dir;
	NameSizePair curr_font;
	FontKV* fonts;

	SceneName curr_scene;
	CustomEvent* event_queue;
};
FontTTF* context_get_font(Context* ctx) {
	FontTTF* font = cyx_hashmap_get(ctx->fonts, ctx->curr_font);
	assert(font != NULL);
	return font;
}

void __change_per_unit(int goal, int count, int sizes[]) {
	assert(count > 0);
	int sum = 0;
	for (int i = 0; i < count; ++i) {
		sum += sizes[i];
	}

	float per_segment = (float)goal / sum;
	if (count > 1) {
		sizes[0] *= per_segment;
		for (int i = 1; i < count - 1; ++i) {
			sizes[i] = sizes[i - 1] + sizes[i] * per_segment;
		}
		sizes[count - 1] = goal;
	} else {
		sizes[0] = goal;
	}
}
void __get_xywh_from_grid(int pos, int* x, int* y, int* w, int* h) {
	struct Grid* g = &grids[__grid_len - 1];
	// printf("Grid (%d, %d, %d, %d): [ ", g->x, g->y, g->w, g->h);
	// for (size_t i = 0; i < g->len; ++i) {
	// 	if (i) printf(", ");
	// 	printf("%d", g->sizes[i]);
	// }
	// printf(" ]\n");

	assert(0 <= pos && pos < g->len);
	int less = (pos ? g->sizes[pos - 1] : 0);
	if (g->type == GRID_VERTICAL) {
		*x = less;
		*y = g->y;
		*w = g->sizes[pos] - less;
		*h = g->h;
	} else if (g->type == GRID_HORIZONTAL) {
		*x = g->x;
		*y = less;
		*w = g->w;
		*h = g->sizes[pos] - less;
	} else {
		assert(0 && "UNREACHABLE");
	}
}
void __grid_push(int pos, enum GridType type, int n, int sizes[]) {
	assert(n <= GRID_SIZE);
	struct Grid* g = &grids[__grid_len - 1];
	assert(g->scene && g->ctx);

	struct Grid grid = {
		.scene = g->scene,
		.ctx = g->ctx,
		.type = type,
		.len = n,
	};
	__get_xywh_from_grid(pos, &grid.x, &grid.y, &grid.w, &grid.h);

	if (type == GRID_VERTICAL) {
		__change_per_unit(grid.w, n, sizes);
	} else if (type == GRID_HORIZONTAL) {
		__change_per_unit(grid.h, n, sizes);
	} else {
		assert(0 && "UNREACHABLE");
	}
	memcpy(grid.sizes, sizes, n * sizeof(*sizes));
	grid.z_buffer = cyx_array_new(struct ZBuffer, .reserve = 4, .cmp_fn = __zbuf_cmp);

	memcpy(&grids[__grid_len++], &grid, sizeof(grid));
}
#define grid_set(scene, ctx) do { \
	grids[0].scene = scene; \
	grids[0].ctx = ctx; \
} while(0)
#define grid_start(pos, type, ...) do {\
	int grid_desc[] = { __VA_ARGS__ }; \
	__grid_push(pos, type, sizeof(grid_desc)/sizeof(*grid_desc), grid_desc); \
} while(0)
int scene_data_get_clicked(Context* ctx, SceneData* data, Vec2i other, Vec2i pos, Vec2i wh) {
	FontTTF* font = context_get_font(ctx);

	switch (data->type) {
		case DATA_STATIC_TEXT: {
			char* str = data->as.static_text.text;
			Vec2i letter_wh = { .x = 0, .y = font->size * 3 };
			for (size_t i = 0; i < cyx_str_length(str); ++i) {
				letter_wh.x = font_get_advance(font, str[i]);
				if (vec2i_rect_collision(other, pos, letter_wh)) {
					return 1;
				}
				pos.x += letter_wh.x;
			}
	   	} break;
		case DATA_TEXT:
			if (vec2i_rect_collision(other, pos, wh)) {
				return 1;
			}
			break;
		case DATA_RECT:
			if (vec2i_rect_collision(other, pos, wh)) {
				return 1;
			}
			break;
		case DATA_VALUES: break;
		default: assert(0 && "UNREACHABLE");
	}
	return 0;
}
void scene_data_show(Context* ctx, int id, int x, int y, int w, int h) {
	SceneData* data = scene_data_pool.data + id;
	FontTTF* font = context_get_font(ctx);

	switch (data->type) {
		case DATA_STATIC_TEXT: {
			char* str = data->as.static_text.text;
			for (size_t i = 0; i < cyx_str_length(str); ++i) {
				int advance = font_show(font, str[i], x, y);
				x += advance;
			}
	   	} break;
		case DATA_TEXT: {
			char* str = data->as.dyn_text.text;
			for (size_t i = 0; i < cyx_str_length(str); ++i) {
				int advance = font_show(font, str[i], x, y);
				x += advance;
			}
		} break;
		case DATA_RECT:
			rect_show(&data->as.rect, x, y, w, h, ctx->wh.x, ctx->wh.y);
			break;
		case DATA_VALUES: break;
		default: assert(0 && "UNREACHABLE");
	}
}
void __grid_show() {
	struct Grid* g = &grids[__grid_len - 1];
	Context* ctx = g->ctx;

	cyx_array_sort(g->z_buffer);
	for (size_t i = 0; i < cyx_array_length(g->z_buffer); ++i) {
		struct ZBuffer buf = g->z_buffer[i];
		scene_data_show(ctx, buf.id, buf.x, buf.y, buf.w, buf.h);
	}

	if (ctx->state.type == STATE_TEST_CLICK) {
		Vec2i pos = FROM_VOID(Vec2i, ctx->state.value);
		cyx_array_reverse(g->z_buffer);
		
		for (size_t i = 0; i < cyx_array_length(g->z_buffer); ++i) {
			struct ZBuffer buf = g->z_buffer[i];
			if (scene_data_get_clicked(ctx, scene_data_pool.data + buf.id, vec2i(pos.x, pos.y), vec2i(buf.x, buf.y), vec2i(buf.w, buf.h))) {
				cyx_ring_push(ctx->event_queue, CUSTOM_EVENT(EVENT_CLICKED_ID, &buf.id));
				goto found;
			}
		}
		int val = -1;
		cyx_ring_push(ctx->event_queue, CUSTOM_EVENT(EVENT_CLICKED_ID, &val));
found: { }
	}
}
#define grid_end() do { \
	__grid_show(); \
	assert(__grid_len != 1); \
	cyx_array_free(grids[--__grid_len].z_buffer); \
} while(0)
void scene_add(Scene* scene, const char* id, SceneData data) {
	char* _id = cyx_str_from_lit(id);
	size_t* ret = cyx_hashmap_get(scene->data, _id);
	assert(ret == NULL && "There already exists an element with the same name!");
	data.id = scene_data_pool.len;

	memcpy(scene_data_pool.data + scene_data_pool.len, &data, sizeof(data));

	cyx_hashmap_add_v(scene->data, _id, scene_data_pool.len);
	scene_data_pool.len++;
}
SceneData* scene_get(Scene* scene, const char* id) {
	char* _id = cyx_str_from_lit(id);
	size_t* ret = cyx_hashmap_get(scene->data, _id);
	assert(ret != NULL);
	return &scene_data_pool.data[*ret];
}
size_t scene_get_i(Scene* scene, const char* id) {
	char* _id = cyx_str_from_lit(id);
	size_t* ret = cyx_hashmap_get(scene->data, _id);
	assert(ret != NULL);
	return *ret;
}

#define scene_setup(scene, ctx) do { \
	basic_scene_setup((scene), (ctx));\
	if ((scene)->setup) { \
		(scene)->setup((scene), (ctx));\
	} \
} while(0)
#define scene_show(scene, ctx) do { \
	basic_scene_show((scene), (ctx));\
	if ((scene)->show) { \
		(scene)->show((scene), (ctx));\
	} \
} while(0)
#define scene_cleanup(scene, ctx) do { \
	if (!(scene)->cleaned_up) { \
		basic_scene_cleanup((scene), (ctx));\
		if ((scene)->cleanup) { \
			(scene)->cleanup((scene), (ctx));\
		} \
	} \
} while(0)

struct GridShowParams {
	int __pos;
	const char* __id;

	int x;
	int y;
	int z;
	int padding;
};
void __grid_add(struct GridShowParams params) {
	int pos = params.__pos;

	struct Grid* g = &grids[__grid_len - 1];
	assert(0 <= pos && pos < g->len);

	int i = scene_get_i(g->scene, params.__id);
	int x = 0, y = 0, w = 0, h = 0;
	__get_xywh_from_grid(pos, &x, &y, &w, &h);

	struct ZBuffer buf = {
		.id = i,
		.x = x + params.padding + params.x,
		.y = y + params.padding + params.y,
		.w = w - 2 * params.padding,
		.h = h - 2 * params.padding
	};
	if (!params.z) {
		g->max_z++;
		buf.z = g->max_z;
	} else {
		if (params.z > g->max_z) {
			g->max_z = params.z;
		}
		buf.z = params.z;
	}
	cyx_array_append(g->z_buffer, buf);
}
#define grid_add(pos, id, ...) (__grid_add((struct GridShowParams){ .__pos = pos, .__id = id, __VA_ARGS__ }))

void textbox_click_callback(Context* ctx, SceneData* data) {
	ctx->curr_input = data->id;
}
#define SCENE_DATA_TEXT(str, color) ((SceneData){ \
	.type = DATA_STATIC_TEXT, \
	.as.static_text = { \
		.text = cyx_str_from_lit(str), \
		.clr = (color), \
	} \
})
#define SCENE_DATA_DYN_TEXT(color) ((SceneData){ \
	.type = DATA_TEXT, \
	.click = textbox_click_callback, \
	.as.dyn_text = { \
		.text = cyx_str_new(), \
		.cursor = rect_create(compiled_programs[PROGRAM_RECT], (color)), \
		.clr = (color), \
	} \
})
#define SCENE_DATA_RECT(color, ...) ((SceneData){ \
	.type = DATA_RECT, \
	.as.rect = rect_create(compiled_programs[PROGRAM_RECT], (color), __VA_ARGS__), \
})

void basic_scene_setup(Scene* scene, Context* ctx) {
	(void)ctx;
	scene->data = cyx_hashmap_new(IdSceneData, cyx_hash_str, cyx_eq_str, .defer_key_fn = cyx_str_free, .is_key_ptr = 1);
	scene->cleaned_up = 0;
}
void basic_scene_show(Scene* scene, Context* ctx) {
	grid_set(scene, ctx);
}
void basic_scene_cleanup(Scene* scene, Context* ctx) {
	(void)ctx;
	cyx_hashmap_free(scene->data);
	scene->cleaned_up = 1;
}

void main_setup(Scene* scene, Context* ctx) {
	(void)ctx;

	scene_add(scene, "f",	 		  SCENE_DATA_TEXT("f(x,y,z)=", COLOR_WHITE));
	scene_add(scene, "terminal_rect", SCENE_DATA_RECT(COLOR_GRAY, .border_width = 10, .border_color = COLOR_WHITE));
	scene_add(scene, "terminal_text", SCENE_DATA_DYN_TEXT(COLOR_WHITE));
}
void main_show(Scene* scene, Context* ctx) {
	(void)scene;
	(void)ctx;

	grid_start(0, GRID_VERTICAL, 1, 2, 3); {
		grid_add(0, "f");
		grid_add(1, "terminal_rect");
		grid_add(1, "terminal_text", .padding = 10);
	} grid_end();
}

Scene scenes[SCENE_COUNT] = { 
	{
		.setup = main_setup,
		.show = main_show,
	},
};

void compile_programs() {
	for (size_t i = 0; i < PROGRAM_COUNT; ++i) {
		compiled_programs[i] = compile_program(programs[2 * i], programs[2 * i + 1]);
	}
}
void context_add_font(Context* ctx, const char* font_name, size_t size) {
	char* file_path = cyx_str_copy(ctx->font_dir);
	char* file_name = cyx_str_from_lit(font_name);
	cyx_str_append_str(&file_path, file_name);
	cyx_str_append_lit(&file_path, ".ttf");
	cyx_str_append_char(&file_path, '\0');

	cyx_hashmap_add_v(ctx->fonts, ((NameSizePair){ .name = file_name, .size = size}), font_compile(compiled_programs[PROGRAM_FONT], file_path, size, WIDTH, HEIGHT));

	ctx->curr_font = (NameSizePair){
		.name = cyx_str_copy(file_name),
		.size = size,
	};
}
void context_setup(Context* ctx) {
	srand(time(NULL));

	{ // Setting up OpenGL hints
		RGFW_glHints* hints = RGFW_getGlobalHints_OpenGL();
		hints->major = 3;
		hints->minor = 3;
		hints->profile = RGFW_glCore;
		RGFW_setGlobalHints_OpenGL(hints);
	}

	ctx->curr_input = -1;

	ctx->wh = vec2i(WIDTH, HEIGHT);
	ctx->win = RGFW_createWindow("Grphing Calculator", 0, 0, WIDTH, HEIGHT, RGFW_windowCenter | RGFW_windowOpenGL | RGFW_windowNoResize);
	RGFW_window_swapInterval_OpenGL(ctx->win, 0);

	GLenum err = glewInit();
	if (err != GLEW_OK) {
		fprintf(stderr, "ERROR:\tUnable to initialize GLEW!\nLOG: %s\n", glewGetErrorString(err));
		exit(1);
	}
	printf("LOG:\tOpenGL version supported: %s\n", glGetString(GL_VERSION));

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	compile_programs();
	// RGFW_setKeyCallback(key_callback);

	ctx->font_dir = cyx_str_from_lit("./resources/fonts/");
	ctx->fonts = cyx_hashmap_new(FontKV, __name_size_hash, __name_size_eq, .defer_key_fn = __name_size_free, .defer_value_fn = __font_free);
	context_add_font(ctx, "JetBrainsMonoNerdFont-Bold", 20);

	ctx->curr_scene = SCENE_MAIN;
	ctx->event_queue = cyx_ring_new(CustomEvent);

	scene_setup(scenes + SCENE_MAIN, ctx);
}
void context_events(Context* ctx) {
	updateTimer();

	RGFW_event event;
	while (RGFW_window_checkEvent(ctx->win, &event)) {
		if (event.type == RGFW_quit) {
			break;
		} else if (event.type == RGFW_mouseButtonPressed) {
			Vec2i pos = { 0 };
			RGFW_window_getMouse(ctx->win, &pos.x, &pos.y);
			cyx_ring_push(ctx->event_queue, CUSTOM_EVENT(EVENT_CLICK, &pos));
		} else if (event.type == RGFW_keyPressed) {
			// printf("Keycode: %d\n", event.key.value);
			// printf("Mod: %d\n", event.key.mod);
			// printf("Type: %d\n", event.key.type);
			// printf("Char: %d, '%c'\n", event.key.sym, event.key.sym);
			if (event.key.value == 27) {
				event.type = RGFW_quit;
				RGFW_eventQueuePush(&event);
			} else if (event.key.value == 154) {
				ctx->key_info.shift_held = 1;
			} else if (event.key.value == 155) {
				ctx->key_info.ctrl_held = 1;
			} else if (event.key.value == 156) {
				ctx->key_info.alt_held = 1;
			}

			if (ctx->curr_input != -1 && scene_data_pool.data[ctx->curr_input].type == DATA_TEXT && event.key.sym >= ' ') {
				cyx_str_append_char(&scene_data_pool.data[ctx->curr_input].as.dyn_text.text, event.key.sym);
			}
		} else if (event.type == RGFW_keyReleased) {
			if (event.key.value == 154) {
				ctx->key_info.shift_held = 0;
			} else if (event.key.value == 155) {
				ctx->key_info.ctrl_held = 0;
			} else if (event.key.value == 156) {
				ctx->key_info.alt_held = 0;
			}
		}
	}

	cyx_ring_drain(val, ctx->event_queue) {
		switch (val->type) {
			case EVENT_CLICK:
				ctx->state.type = STATE_TEST_CLICK;
				ctx->state.value = val->value;
				break;
			case EVENT_CLICKED_ID: {
				ctx->state.type = STATE_NONE;
				int id = FROM_VOID(int, val->value);
				if (id != -1 && scene_data_pool.data[id].click) {
					scene_data_pool.data[id].click(ctx, scene_data_pool.data + id);
				}
				ctx->state.value = NULL;
			} break;
			default: assert(0 && "UNREACHABLE");
		}
	}
}
void context_show(Context* ctx) {
	glClearColor(0x18/255.0f, 0x18/255.0f, 0x18/255.0f, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	Scene* curr = &scenes[ctx->curr_scene];

	scene_show(curr, ctx);
}
void context_cleanup(Context* ctx) {
	cyx_ring_free(ctx->event_queue);
	cyx_hashmap_free(ctx->fonts);

	scene_pool_cleanup();

	for (size_t i = 0; i < SCENE_COUNT; ++i) {
		scene_cleanup(scenes + i, ctx);
	}

	for (size_t i = 0; i < PROGRAM_COUNT; ++i) {
		glDeleteProgram(compiled_programs[i]);
	}

	RGFW_window_close(ctx->win);
}

