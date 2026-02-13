#ifndef __SHARED_UTILS_H__
#define __SHARED_UTILS_H__

#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>

#define CYLIBX_ALLOC
#include <cylibx.h>

#include <shapes.h>
#include <font.h>
#include <mat.h>

#define WIDTH 1600
#define HEIGHT 900
typedef struct Context Context;

// timer
typedef struct {
	double prev_time;
	double collection;
	double dt;

	size_t frames;
	char log;
	char fps_cap;
} Timer;
void updateTimer(Timer* timer);

// programs
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
uint32_t compile_program(EvoAllocator* alloc, const char* vert_file, const char* frag_file);

enum ProgramNames {
	PROGRAM_FONT,
	PROGRAM_RECT,
	PROGRAM_3D,
	PROGRAM_COUNT,
};

// events
typedef struct {
	enum {
		EVENT_SHOWABLE_CLICKED,
		EVENT_TURN_OFF_INPUT,
	} type;
	int value;
} CustomEvent;
#define CUSTOM_EVENT(t) ((CustomEvent){ .type = (t), .value = 0 })
#define CUSTOM_EVENT_V(t, v) ((CustomEvent){ .type = (t), .value = (v) })

// scene variables
typedef struct SceneShowable SceneShowable;
typedef struct GridList GridList;
typedef enum {
	DATA_ERROR, 

	SHOWABLE_TEXT_INPUT,
	SHOWABLE_STATIC_TEXT,
	SHOWABLE_RECT,
	SHOWABLE_3D,

	DATA_SEPARATOR,

	DATA_INT,
	DATA_FLOAT,
	DATA_VEC2I,
	DATA_VEC2F,
	DATA_LIST,
	DATA_COLOR,
	DATA_VEC4,
	DATA_PTR,
} SceneDataType;
struct SceneShowable {
	char* id;
	SceneDataType type;
	union {
		struct {
			char* text;
			Rectangle cursor;
			Color clr;
			int cursor_pos;
			uint8_t is_selected : 1;
			uint8_t center_x : 1;
			uint8_t center_y : 1;
			uint8_t text_wrap : 1;
		} dyn_text;
		struct {
			char* text;
			Color clr;
			uint8_t center_x : 1;
			uint8_t center_y : 1;
		} static_text;
		Rectangle rect;
		Shape3D shape;
	} as;
	void (*click)(Context* ctx, SceneShowable* showable);
};
typedef struct {
	SceneDataType type;
	union {
		int n;
		float x;
		Vec2i vi;
		Vec2f vf;
		GridList* list;
		Color clr;
		Vec4 v4;
		void* ptr;
	} as;
} SceneValue;

// scene variables kv
typedef struct {
	char* key;
	SceneShowable* value;
} SceneShowableKV;
typedef struct {
	char* key;
	SceneValue* value;
} SceneValueKV;

// grids
typedef struct {
	char* id;

	int x;
	int y;
	int w;
	int h;

	int z;
} ZBuffer;
enum GridType {
	GRID_ERROR,
	GRID_NONE,
	GRID_VERTICAL,
	GRID_HORIZONTAL,
};
#define GRID_SIZE 16
typedef struct {
	enum GridType type;

	int x, y, w, h;
	int len;
	int sizes[GRID_SIZE];
} Grid;

typedef struct {
	void* ptr;
	SceneDataType type;
} ScenePair;
struct __GridAddParams {
	Context* __ctx;
	int __pos;
	const char* __id;
	SceneDataType __type;
	const char* __str;

	int x;
	int y;
	int z;
	int width;
	int height;
	int padding;

	uint8_t text_wrap : 1;
	uint8_t center_x : 1;
	uint8_t center_y : 1;
	uint8_t center : 1;
	uint8_t cull_faces : 1;
	uint8_t depth_test : 1;

	Color color;
	Color border_color;
	int border_width;

	Vec4 world_pos;
	Vec4 camera;
	float scale;
	uint32_t* indices;
	float* vertices;
	float shininess;
	float reflectivity;
	Vec4 light_pos;
	Color light_color;

	void (*click_func)(Context* ctx, SceneShowable* showable);
};
struct GridList {
	char* list_name;
	struct __GridAddParams* params;
	uint8_t done : 1;
};
void __grid_start(Context* ctx, int pos, enum GridType type, size_t n, int* desc);

void __grid_add(struct __GridAddParams params);
#define grid_start(ctx, pos, type, ...) do { \
	int grid_desc[] = { __VA_ARGS__ }; \
	__grid_start(ctx, pos, type, sizeof(grid_desc)/sizeof(*grid_desc), grid_desc); \
} while(0)
#define grid_dyn_text(ctx, pos, id, ...) (__grid_add((struct __GridAddParams){ \
	.__ctx = (ctx), \
	.__pos = (pos), \
	.__id = (id), \
	.__type = SHOWABLE_TEXT_INPUT, \
	__VA_ARGS__ \
}))
#define grid_text(ctx, pos, id, str, ...) (__grid_add((struct __GridAddParams){ \
	.__ctx = (ctx), \
	.__pos = (pos), \
	.__id = (id), \
	.__type = SHOWABLE_STATIC_TEXT, \
	.__str = (str), \
	__VA_ARGS__ \
}))
#define grid_rect(ctx, pos, id, clr, ...) (__grid_add((struct __GridAddParams){ \
	.__ctx = (ctx), \
	.__pos = (pos), \
	.__id = (id), \
	.__type = SHOWABLE_RECT, \
	.color = (clr), \
	__VA_ARGS__ \
}))
#define grid_shape(ctx, pos, id, wrld_pos, clr, scl, inds, vert, cam, l_pos, l_clr, ...) (__grid_add((struct __GridAddParams){ \
	.__ctx = (ctx), \
	.__pos = (pos), \
	.__id = (id), \
	.__type = SHOWABLE_3D, \
	.color = (clr), \
	.scale = (scl), \
	.indices = (inds), \
	.vertices = (vert), \
	.camera = (cam), \
	.light_pos = (l_pos), \
	.light_color = (l_clr), \
	.world_pos = (wrld_pos), \
	 __VA_ARGS__ \
}))
#define grid_end(ctx) cyx_array_pop((ctx)->grids)

ScenePair grid_get(Context* ctx, const char* id, SceneDataType type);
#define grid_get_i(ctx, id) (((SceneValue*)grid_get((ctx), (id), DATA_INT).ptr)->as.n)
#define grid_get_f(ctx, id) (((SceneValue*)grid_get((ctx), (id), DATA_FLOAT).ptr)->as.x)
#define grid_get_vi(ctx, id) (((SceneValue*)grid_get((ctx), (id), DATA_VEC2I).ptr)->as.vi)
#define grid_get_vf(ctx, id) (((SceneValue*)grid_get((ctx), (id), DATA_VEC2F).ptr)->as.vf)
#define grid_get_color(ctx, id) (((SceneValue*)grid_get((ctx), (id), DATA_COLOR).ptr)->as.clr)
#define grid_get_v4(ctx, id) (((SceneValue*)grid_get((ctx), (id), DATA_VEC4).ptr)->as.v4)
#define grid_get_ptr(ctx, id) (((SceneValue*)grid_get((ctx), (id), DATA_PTR).ptr)->as.ptr)
#define grid_get_text(ctx, id) (((SceneShowable*)grid_get((ctx), (id), SHOWABLE_TEXT_INPUT).ptr)->as.dyn_text.text)

#define grid_get_list(ctx, id) (((SceneValue*)grid_get((ctx), (id), DATA_LIST).ptr)->as.list)
#define grid_list_start(ctx, id) do { \
	GridList* list = grid_get_list(ctx, id); \
	if (list->done) { break; } \
	(ctx)->curr_list = list; \
} while(0)
void __grid_list_add(struct __GridAddParams params);
#define grid_list_dyn_text(ctx, pos, ...) (__grid_list_add((struct __GridAddParams){ \
	.__ctx = (ctx), \
	.__pos = (pos), \
	.__id = "", \
	.__type = SHOWABLE_TEXT_INPUT, \
	__VA_ARGS__ \
}))
#define grid_list_text(ctx, pos, str, ...) (__grid_list_add((struct __GridAddParams){ \
	.__ctx = (ctx), \
	.__pos = (pos), \
	.__id = "", \
	.__type = SHOWABLE_STATIC_TEXT, \
	.__str = (str), \
	__VA_ARGS__ \
}))
#define grid_list_rect(ctx, pos, clr, ...) (__grid_list_add((struct __GridAddParams){ \
	.__ctx = (ctx), \
	.__pos = (pos), \
	.__id = "", \
	.__type = SHOWABLE_RECT, \
	.color = (clr), \
	__VA_ARGS__ \
}))
#define grid_list_end(ctx) do { \
	if ((ctx)->curr_list) { \
		(ctx)->curr_list->done = 1; \
		(ctx)->curr_list = NULL; \
	} \
} while(0)
#define grid_list_show(ctx, list, pos, type, ...) do { \
	GridList* l = (list); \
	assert(l->done); \
	grid_start(ctx, pos, type, __VA_ARGS__); { \
		for (size_t i = 0; i < cyx_array_length(l->params); ++i) { \
			__grid_add(l->params[i]); \
		} \
	} grid_end(ctx); \
} while(0)

// font kv
typedef struct {
	char* name;
	int size;
} NameSizePair;

typedef struct {
	NameSizePair key;
	FontTTF value;
} FontKV;

// scene
typedef struct {
	const char* name;
	void (*setup)(Context* ctx);
	void (*show)(Context*);
	void (*key_callback)(Context*, char key, int value);
} SceneDescription;
typedef struct {
	void (*setup)(Context* ctx);
	void (*show)(Context*);
	void (*key_callback)(Context*, char key, int value);
	uint8_t setup_done : 1;
} Scene;
typedef struct {
	char* key;
	Scene value;
} SceneKV;

// context
#define FONT_MAX_COUNT 10
struct Context {
// timer
	Timer timer;

// window data
	Vec2i wh;
	void* win;
	uint8_t fullscreen : 1;

// opengl programs
	uint32_t programs[PROGRAM_COUNT];

// font data
	char* font_dir;
	NameSizePair curr_font;
	FontKV* fonts;

// event queue, FIFO
	CustomEvent* event_queue;

// arena for stuff that needs to be saved over the lifetime of the app
	EvoArena perm_arena;
	EvoAllocator perm;

// arena for per frame resets
	EvoArena temp_arena;
	EvoAllocator temp;
	
// scene values part
	EvoPool scene_pool;
	EvoAllocator scene_alloc;

	SceneShowableKV* showables;
	SceneValueKV* values;

// grids
	Grid* grids; // grid stack
	GridList* curr_list;
	
	int max_z;
	ZBuffer* z_buffer;

// text input data
	char* curr_text_input;
	struct {
		uint8_t key;
		uint8_t shift_held : 1;
		uint8_t ctrl_held : 1;
		uint8_t alt_held : 1;
	} key_info;

// mouse flag awaiting processing
	SceneShowable* showable_clicked;
	struct {
		Vec2i pos;
		uint8_t pressed : 1;
		uint8_t left : 1;
		uint8_t middle : 1;
		uint8_t right : 1;
	} mouse_info;

// scene
	char* curr_scene;
	SceneKV* scenes;
};

void context_setup(Context* ctx, size_t scene_count, SceneDescription* scenes);
void context_update(Context* ctx);
void context_cleanup(Context* ctx);

#define push_event(ctx, event) cyx_ring_push((ctx)->event_queue, CUSTOM_EVENT(event))

#endif // __SHARED_UTILS_H__
