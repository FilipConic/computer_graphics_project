#define CYLIBX_ALLOC
#define CYLIBX_IMPLEMENTATION
#include <cylibx.h>

#define RGFW_OPENGL
#define RGFW_IMPLEMENTATION
#include <RGFW.h>

#include <immediate.h>
#include <stdio.h>

static const char* programs[2 * PROGRAM_COUNT] = {
	"./resources/shaders/font.vert", "./resources/shaders/font.frag",
	"./resources/shaders/basic.vert", "./resources/shaders/basic.frag",
	"./resources/shaders/shader3d.vert", "./resources/shaders/shader3d.frag",
};

static ScenePair context_get(Context* ctx, char* id) {
	if (!id) { return (ScenePair){ .ptr = NULL, .type = DATA_ERROR }; }
	SceneShowable** showable = cyx_hashmap_get(ctx->showables, id);
	if (showable) {
		return (ScenePair) {
			.type = (*showable)->type,
			.ptr = *showable,
		};
	}

	SceneValue** value = cyx_hashmap_get(ctx->values, id);
	if (!value) {
		return (ScenePair) { .type = DATA_ERROR, .ptr = NULL, };
	}
	return (ScenePair) {
		.type = (*value)->type,
		.ptr = *value,
	};
}
static void context_add(Context* ctx, char* id, SceneDataType type, void* value) {
	ScenePair ret = context_get(ctx, id);
	if (ret.ptr) {
		if (ret.type != type) {
			fprintf(stderr, "ERROR:\tThe ID you've passed already exists but is off a different type! [" CYX_STR_FMT "]\n", CYX_STR_UNPACK(id));
		}
		return;
	}

	if (type < DATA_SEPARATOR) {
		cyx_hashmap_add_v(ctx->showables, cyx_str_copy_a(&ctx->perm, id), (SceneShowable*)(value));
	} else {
		cyx_hashmap_add_v(ctx->values, cyx_str_copy_a(&ctx->perm, id), (SceneValue*)(value));
	}
}
static ScenePair grid_add(Context* ctx, char* id, SceneDataType type) {
	if (type < DATA_SEPARATOR) { assert("UNREACHABLE"); }

	SceneValue* value = evo_alloc_malloc(&ctx->perm, sizeof(SceneValue));
	value->type = type;
	switch (type) {
		case DATA_INT: value->as.n = 0; break;
		case DATA_FLOAT: value->as.x = 0; break;
		case DATA_VEC2I: value->as.vi = vec2i(0, 0); break;
		case DATA_VEC2F: value->as.vf = vec2f(0, 0); break;
		case DATA_COLOR: value->as.clr = COLOR_NONE; break;
		case DATA_LIST: {
			GridList* ptr = evo_alloc_malloc(&ctx->perm, sizeof(GridList));;
			ptr->list_name = cyx_str_copy_a(&ctx->perm, id);
			ptr->params = NULL;
			ptr->done = 0;
			value->as.list = ptr;
		} break;
		case DATA_VEC4: value->as.v4 = vec4(0, 0, 0); break;
		case DATA_PTR: value->as.ptr = NULL; break;
		default: assert("UNREACHABLE");
	}
	context_add(ctx, id, type, value);
	return (ScenePair){ .ptr = value, .type = type };
}
ScenePair grid_get(Context* ctx, const char* id, SceneDataType type) {
	char* _id = cyx_str_from_lit(&ctx->temp, id);
	ScenePair p = context_get(ctx, _id);
	if (p.ptr && p.type != type) {
		fprintf(stderr, "ERROR:\tThe ID you've passed already exists but is off a different type! [" CYX_STR_FMT "]\n", CYX_STR_UNPACK(id));
		return (ScenePair){ .ptr = NULL, .type = DATA_ERROR };
	} else if (p.ptr) {
		assert(p.type == type);
		return p;
	} else if (!p.ptr && type < DATA_SEPARATOR) {
		// fprintf(stderr, "ERROR:\tYou can not add a showable element yourself! Use a different function!\n");
		return (ScenePair){ .ptr = NULL, .type = DATA_ERROR };
	}
	return grid_add(ctx, _id, type);
}
static FontTTF* context_get_font(Context* ctx) {
	FontTTF* font = cyx_hashmap_get(ctx->fonts, ctx->curr_font);
	assert(font != NULL);
	return font;
}
static int zbuf_cmp(const void* const a, const void* const b) { 
	const ZBuffer* const za = a;
	const ZBuffer* const zb = b;
	if (za->z < zb->z) {
		return -1;
	} else if (za->z > zb->z) {
		return 1;
	} else {
		return 0;
	}
}

static void change_per_unit(int start_val, int goal, int count, int sizes[]) {
	assert(count > 0);
	int sum = 0;
	for (int i = 0; i < count; ++i) {
		sum += sizes[i];
	}

	float per_segment = (float)goal / sum;
	if (count > 1) {
		sizes[0] = sizes[0] * per_segment + start_val;
		for (int i = 1; i < count - 1; ++i) {
			sizes[i] = sizes[i - 1] + sizes[i] * per_segment;
		}
		sizes[count - 1] = start_val + goal;
	} else {
		sizes[0] = goal + start_val;
	}
}
static void get_xywh_from_grid(Context* ctx, int pos, int* x, int* y, int* w, int* h) {
	Grid* g = cyx_array_top(ctx->grids);
	// printf("Grid (%d, %d, %d, %d): [ ", g->x, g->y, g->w, g->h);
	// for (size_t i = 0; i < g->len; ++i) {
	// 	if (i) printf(", ");
	// 	printf("%d", g->sizes[i]);
	// }
	// printf(" ]\n");
	assert(0 <= pos && pos < g->len);

	int less = (pos ? g->sizes[pos - 1] :  (g->type == GRID_VERTICAL ? g->x : (g->type == GRID_HORIZONTAL ? g->y : 0)));
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
	} else if (g->type == GRID_NONE) {
		*x = g->x;
		*y = g->y;
		*w = g->w;
		*h = g->h;
	} else {
		assert(0 && "UNREACHABLE");
	}
}
static void grid_init(Context* ctx) {
	ctx->max_z = -1;
	ctx->z_buffer = cyx_array_new(ZBuffer, &ctx->temp, .cmp_fn = zbuf_cmp);
	cyx_array_append(ctx->grids, ((Grid) {
		.type = GRID_NONE,

		.x = 0,
		.y = 0,
		.w = ctx->wh.x,
		.h = ctx->wh.y,

		.len = 1,
		.sizes = { ctx->wh.x },
	}));
}
void __grid_start(Context* ctx, int pos, enum GridType type, size_t n, int* desc) {
	assert(n <= GRID_SIZE);

	Grid grid = {
		.type = type,
		.len = n,
	};
	get_xywh_from_grid(ctx, pos, &grid.x, &grid.y, &grid.w, &grid.h);
	// printf("%d : %d, %d, %d, %d -> ", pos, grid.x, grid.y, grid.w, grid.h);

	if (type == GRID_VERTICAL) {
		change_per_unit(grid.x, grid.w, n, desc);
	} else if (type == GRID_HORIZONTAL) {
		change_per_unit(grid.y, grid.h, n, desc);
	} else {
		assert(0 && "UNREACHABLE");
	}
	// printf("[ ");
	// for (size_t i = 0; i < n; ++i) {
	// 	if (i) printf(", "); 
	// 	printf("%d", desc[i]);
	// }
	// printf(" ]\n");
	memcpy(grid.sizes, desc, n * sizeof(*desc));

	cyx_array_append(ctx->grids, grid);
}
void __grid_add(struct __GridAddParams params) {
	Context* ctx = params.__ctx;
	int pos = params.__pos;
	assert(params.__id);
	char* id = cyx_str_from_lit(&ctx->temp, params.__id);
	SceneDataType type = params.__type;

	Color color = params.color.r || params.color.g || params.color.b || params.color.a ? params.color : COLOR_WHITE;

	ScenePair ret = context_get(ctx, id);
	if (!ret.ptr) {
		assert(type < DATA_SEPARATOR);
		SceneShowable* showable = evo_pool_malloc(&ctx->scene_pool, 0);
		showable->type = type;
		showable->click = params.click_func;
		showable->id = cyx_str_copy_a(&ctx->perm, id);
		switch (type) {
			case SHOWABLE_RECT: {
				showable->as.rect = rect_create(
					ctx->programs[PROGRAM_RECT],
					params.color,
					.border_color = params.border_color,
					.border_width = params.border_width
				);
			} break;
			case SHOWABLE_STATIC_TEXT: {
				const char* str = params.__str;
				if (!str) {
					fprintf(stderr, "ASSERT:\tYou can't have an empty text block! [" CYX_STR_FMT "]\n", CYX_STR_UNPACK(id));
					assert(0);
				}
				showable->as.static_text.text = cyx_str_from_lit(&ctx->perm, str);
				showable->as.static_text.clr = color;

				showable->as.static_text.center_x = params.center ? 1 : params.center_x;
				showable->as.static_text.center_y = params.center ? 1 : params.center_y;
			} break;
			case SHOWABLE_TEXT_INPUT: {
				if (params.__str) {
					showable->as.dyn_text.text = cyx_str_copy_a(&ctx->perm, params.__str);
				} else {
					showable->as.dyn_text.text = cyx_str_new(&ctx->perm);
				}
				showable->as.dyn_text.is_selected = 0;
				showable->as.dyn_text.clr = color;
				showable->as.dyn_text.cursor = rect_create(ctx->programs[PROGRAM_RECT], color);

				showable->as.dyn_text.center_x = params.center ? 1 : params.center_x;
				showable->as.dyn_text.center_y = params.center ? 1 : params.center_y;

				showable->as.dyn_text.text_wrap = params.text_wrap;
				showable->as.dyn_text.cursor_pos = -1;
			} break;
			case SHOWABLE_3D: {
				assert(params.indices && params.vertices);
				showable->as.shape = shape3d_create(ctx->programs[PROGRAM_3D], color, params.scale, params.indices, params.vertices);
				showable->as.shape.camera = params.camera;
				showable->as.shape.light_color = params.light_color;
				showable->as.shape.light_pos = params.light_pos;
				showable->as.shape.pos = params.world_pos;
				showable->as.shape.depth_test = params.depth_test;
				showable->as.shape.face_cull = params.cull_faces;
			} break;
			default: assert(0 && "UNREACHABLE");
		}
		ret.ptr = showable;
		ret.type = type;
		cyx_hashmap_add_v(ctx->showables, cyx_str_copy_a(&ctx->perm, id), showable);
	} else {
		assert(type < DATA_SEPARATOR);
		SceneShowable* showable = ret.ptr;
		switch (type) {
			case SHOWABLE_RECT: {
				showable->as.rect.color = params.color;
				showable->as.rect.border_color = params.border_color;
				showable->as.rect.border_width = params.border_width;
			} break;
			case SHOWABLE_STATIC_TEXT: {
				const char* str = params.__str;
				if (!str) {
					fprintf(stderr, "ASSERT:\tYou can't have an empty text block! [" CYX_STR_FMT "]\n", CYX_STR_UNPACK(id));
					assert(0);
				}
				if (!cyx_str_eq(cyx_str_from_lit(&ctx->temp, str), showable->as.static_text.text)) {
					showable->as.static_text.text = cyx_str_from_lit(&ctx->perm, str);
				}
				showable->as.static_text.clr = color;

				showable->as.static_text.center_x = params.center ? 1 : params.center_x;
				showable->as.static_text.center_y = params.center ? 1 : params.center_y;
			} break;
			case SHOWABLE_TEXT_INPUT: {
				showable->as.dyn_text.clr = color;
				showable->as.dyn_text.cursor.color = color;

				showable->as.dyn_text.center_x = params.center ? 1 : params.center_x;
				showable->as.dyn_text.center_y = params.center ? 1 : params.center_y;

				showable->as.dyn_text.text_wrap = params.text_wrap;
			} break;
			case SHOWABLE_3D: {
				showable->as.shape.color = color;
				showable->as.shape.camera = params.camera;
				showable->as.shape.light_color = params.light_color;
				showable->as.shape.light_pos = params.light_pos;
				showable->as.shape.scale = params.scale;
				showable->as.shape.shininess = params.shininess;
				showable->as.shape.reflectivity = params.reflectivity;
				showable->as.shape.pos = params.world_pos;

				showable->as.shape.depth_test = params.depth_test;
				showable->as.shape.face_cull = params.cull_faces;
			} break;
			default: assert(0 && "UNREACHABLE");
		}
	}
	
	Grid* grid = cyx_array_top(ctx->grids);
	assert(grid);
	ZBuffer buf = {
		.id = id,
		.z = params.z ? (ctx->max_z = (params.z >= ctx->max_z ? params.z + 1 : ctx->max_z), params.z) : ++ctx->max_z,
	};
	get_xywh_from_grid(ctx, pos, &buf.x, &buf.y, &buf.w, &buf.h);
	buf.w -= params.padding << 1;
	buf.h -= params.padding << 1;
	buf.x += (params.x >= 0 ? params.x : buf.w + params.x - (params.padding << 1)) + params.padding;
	buf.y += (params.y >= 0 ? params.y : buf.h + params.y - (params.padding << 1)) + params.padding;
	cyx_array_append(ctx->z_buffer, buf);
}
void __grid_list_add(struct __GridAddParams params) {
	assert(params.__ctx);
	if (!params.__ctx->curr_list) { return; }
	GridList* list = params.__ctx->curr_list;

	if (!list->params) {
		list->params = cyx_array_new(struct __GridAddParams, &params.__ctx->perm, .reserve = 16);
	}

	char* id = cyx_str_copy(list->list_name);
	cyx_str_append_uint(&id, cyx_array_length(list->params));
	cyx_str_append_char(&id, '\0');
	params.__id = id;

	cyx_array_append(list->params, params);
}
static int scene_data_get_clicked(Context* ctx, SceneShowable* data, Vec2i other, Vec2i pos, Vec2i wh) {
	FontTTF* font = context_get_font(ctx);

	// printf("type: %d\nmouse_x = %d, mouse_y = %d\nx = %d, y = %d\nw = %d, h = %d\n", data->type, other.x, other.y, pos.x, pos.y, wh.x, wh.y);
	switch (data->type) {
		case SHOWABLE_STATIC_TEXT: {
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
		case SHOWABLE_TEXT_INPUT:
			if (vec2i_rect_collision(other, pos, wh)) {
				return 1;
			}
			break;
		case SHOWABLE_RECT:
			if (vec2i_rect_collision(other, pos, wh)) {
				return 1;
			}
			break;
		case SHOWABLE_3D: break;
		default: assert(0 && "UNREACHABLE");
	}
	return 0;
}
static void scene_data_show(Context* ctx, char* id, int x, int y, int w, int h) {
	FontTTF* font = context_get_font(ctx);

	ScenePair ret = context_get(ctx, id);
	if (ret.type == DATA_ERROR || ret.type > DATA_SEPARATOR) { return; }

	SceneShowable* showable = ret.ptr;
	switch (showable->type) {
		case SHOWABLE_STATIC_TEXT: {
			char* str = showable->as.static_text.text;
			if (showable->as.static_text.center_x) {
				int font_w = font_get_text_width(font, cyx_str_length(str), str);
				x += w / 2 - font_w / 2;
			}
			if (showable->as.static_text.center_y) {
				int font_h = 1.2 * font->size;
				y += h / 2 - font_h / 2;
			}
			int xx = x;
			for (size_t i = 0; i < cyx_str_length(str); ++i) {
				if (str[i] != '\n') {
					int advance = font_show(font, str[i], x, y, ctx->wh.x, ctx->wh.y);
					x += advance;
				} else {
					x = xx;
					y += 1.2 * font->size;
				}
			}
	   	} break;
		case SHOWABLE_TEXT_INPUT: {
			char* str = showable->as.dyn_text.text;
			int xx = x;
			int yy = y;
			if (showable->as.dyn_text.cursor_pos >= (int)cyx_str_length(str)) {
				showable->as.dyn_text.cursor_pos = -1;
			}
			if (showable->as.dyn_text.text_wrap) {
				for (size_t i = 0; i < cyx_str_length(str); ++i) {
					if (str[i] != '\n') {
						int advance = font_show(font, str[i], x, y, ctx->wh.x, ctx->wh.y);
						x += advance;
						if (x + advance >= xx + w) {
							x = xx;
							y += 1.2 * font->size;
						}
					} else {
						x = xx;
						y += 1.2 * font->size;
					}
				}
			} else {
				for (size_t i = 0; i < cyx_str_length(str); ++i) {
					int advance = font_show(font, str[i], x, y, ctx->wh.x, ctx->wh.y);
					x += advance;
				}
			}
			if (showable->as.dyn_text.is_selected) {
				if (showable->as.dyn_text.cursor_pos == -1) {
					int cursor_w = font_get_advance(font, ' ');
					rect_show(&showable->as.dyn_text.cursor, x, y, cursor_w, 1.2f * font->size, ctx->wh.x, ctx->wh.y);
				} else {
					x = xx;
					y = yy;
					for (int i = 0; i < showable->as.dyn_text.cursor_pos; ++i) {
						if (str[i] != '\n') {
							int advance = font_get_advance(font, str[i]);
							x += advance;
							if (x + advance >= xx + w) {
								x = xx;
								y += 1.2 * font->size;
							}
						} else {
							x = xx;
							y += 1.2 * font->size;
						}
					}
					int cursor_w = font_get_advance(font, ' ');
					rect_show(&showable->as.dyn_text.cursor, x, y, cursor_w, 1.2f * font->size, ctx->wh.x, ctx->wh.y);
				}
			}
		} break;
		case SHOWABLE_RECT:
			rect_show(&showable->as.rect, x, y, w, h, ctx->wh.x, ctx->wh.y);
			break;
		case SHOWABLE_3D:
			shape3d_show(&showable->as.shape, x, y, w, h, ctx->wh.x, ctx->wh.y);
			break;
		default: assert(0 && "UNREACHABLE");
	}
}

void updateTimer(Timer* timer) {
	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC, &t);
	double curr_time = t.tv_sec + t.tv_nsec * 1e-9;
	timer->dt = curr_time - timer->prev_time;
	timer->prev_time = curr_time;
	timer->collection += timer->dt;

	timer->frames++;
	if (timer->fps_cap && timer->frames >= 75) {
		usleep((1. - timer->collection) * 1e6);
		timer->collection = 1.1;
	}
	if (timer->collection > 1.) {
		if (timer->log) {
			printf("FPS:\t%zu\n", timer->frames);
		}
		timer->frames = 0;
		timer->collection = 0.0;
	}
}
uint32_t compile_program(EvoAllocator* alloc, const char* vert_file, const char* frag_file) {
	evo_temp_set_mark(alloc->ctx);
	char* source = cyx_str_from_file(alloc, vert_file);
	uint32_t vert_shader = glCreateShader(GL_VERTEX_SHADER);
	int32_t len = cyx_str_length(source);
	glShaderSource(vert_shader, 1, (const char* const*)&source, &len);
	glCompileShader(vert_shader);
	check_shader_compile_status(vert_shader, vert_file);

	source = cyx_str_from_file(alloc, frag_file);
	uint32_t frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
	len = cyx_str_length(source);
	glShaderSource(frag_shader, 1, (const char* const*)&source, &len);
	glCompileShader(frag_shader);
	check_shader_compile_status(frag_shader, frag_file);

	uint32_t program = glCreateProgram();
	glAttachShader(program, vert_shader);
	glAttachShader(program, frag_shader);
	glLinkProgram(program);
	check_program_compile_status(program);

	glDeleteShader(vert_shader);
	glDeleteShader(frag_shader);
	evo_temp_reset_mark(alloc->ctx);
	return program;
}
static int name_size_eq(const void* const a, const void* const b) {
	const NameSizePair* const p1 = a;
	const NameSizePair* const p2 = b;
	return cyx_eq_str(p1->name, p2->name) && p1->size == p2->size;
}
static size_t name_size_hash(const void* const val) {
	const NameSizePair* const p = val;
	return cyx_hash_str(p->name) ^ cyx_hash_int32(&p->size);
}

static void context_compile_programs(Context* ctx) {
	EvoTempStack temp = { 0 };
	EvoAllocator alloc = evo_allocator_temp(&temp);
	for (size_t i = 0; i < PROGRAM_COUNT; ++i) {
		ctx->programs[i] = compile_program(&alloc, programs[2 * i], programs[2 * i + 1]);
	}
}
static void context_add_font(Context* ctx, const char* font_name, size_t size) {
	char* file_path = cyx_str_copy(ctx->font_dir);
	char* file_name = cyx_str_from_lit(&ctx->perm, font_name);
	cyx_str_append_str(&file_path, file_name);
	cyx_str_append_lit(&file_path, ".ttf");
	cyx_str_append_char(&file_path, '\0');

	cyx_hashmap_add_v(
		ctx->fonts,
		((NameSizePair){
			.name = file_name,
			.size = size
		}),
		font_compile(ctx->programs[PROGRAM_FONT],
			file_path,
			size,
			ctx->wh.x,
			ctx->wh.y
		)
	);

	ctx->curr_font = (NameSizePair){
		.name = cyx_str_copy(file_name),
		.size = size,
	};
}
static void context_key_events(Context* ctx, char key, int value) {
	if (key == 'f' && ctx->key_info.ctrl_held) {
		RGFW_window_setFullscreen(ctx->win, ctx->fullscreen = !ctx->fullscreen);
	}
	if (ctx->curr_text_input) {
		if (value == 164) {
			ScenePair ret = grid_get(ctx, ctx->curr_text_input, SHOWABLE_TEXT_INPUT);
			assert(ret.ptr);

			SceneShowable* showable = ret.ptr;
			if (showable->as.dyn_text.cursor_pos == -1) {
				showable->as.dyn_text.cursor_pos = (int)cyx_str_length(showable->as.dyn_text.text) - 1;
			} else if (showable->as.dyn_text.cursor_pos > 0) {
				--showable->as.dyn_text.cursor_pos;
			}
		} else if (value == 165) {
			ScenePair ret = grid_get(ctx, ctx->curr_text_input, SHOWABLE_TEXT_INPUT);
			assert(ret.ptr);

			SceneShowable* showable = ret.ptr;
			if (showable->as.dyn_text.cursor_pos == (int)cyx_str_length(showable->as.dyn_text.text) - 1) {
				showable->as.dyn_text.cursor_pos = -1;
			} else if (showable->as.dyn_text.cursor_pos != -1) {
				++showable->as.dyn_text.cursor_pos;
			}
		}
	}
}
static void context_events(Context* ctx) {
	updateTimer(&ctx->timer);
	RGFW_event event;
	while (RGFW_window_checkEvent(ctx->win, &event)) {
		if (event.type == RGFW_quit) {
			break;
		} else if (event.type == RGFW_mouseButtonPressed) {
			RGFW_window_getMouse(ctx->win, &ctx->mouse_info.pos.x, &ctx->mouse_info.pos.y);
			if (event.mouse.type == RGFW_mouseRight) {
				ctx->mouse_info.right = 1;
			} else if (event.mouse.type == RGFW_mouseLeft) {
				ctx->mouse_info.left = 1;
			} else if (event.mouse.type == RGFW_mouseMiddle) {
				ctx->mouse_info.middle = 1;
			}
			ctx->mouse_info.pressed = 1;
		} else if (event.type == RGFW_mouseButtonReleased) {
			if (event.mouse.type == RGFW_mouseRight) {
				ctx->mouse_info.right = 0;
			} else if (event.mouse.type == RGFW_mouseLeft) {
				ctx->mouse_info.left = 0;
			} else if (event.mouse.type == RGFW_mouseMiddle) {
				ctx->mouse_info.middle = 0;
			}
		} else if (event.type == RGFW_keyPressed) {
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
			// printf("Key pressed: (sym: %d, value: %d)\n", event.key.sym, event.key.value);

			ScenePair ret = context_get(ctx, ctx->curr_text_input);
			if (ret.ptr &&
				ret.type == SHOWABLE_TEXT_INPUT &&
				(event.key.sym >= ' ' || event.key.value == '\b' || event.key.value == '\n') &&
				!ctx->key_info.ctrl_held
			) {
				SceneShowable* text_input = ret.ptr;
				if (event.key.value != '\b' && event.key.value != '\n') {
					if (text_input->as.dyn_text.cursor_pos == -1) {
						cyx_str_append_char(&text_input->as.dyn_text.text, event.key.sym);
					} else {
						cyx_str_insert_char(&text_input->as.dyn_text.text, event.key.sym, text_input->as.dyn_text.cursor_pos);
						++text_input->as.dyn_text.cursor_pos;
					}
				} else if (event.key.value == '\n') {
					if (text_input->as.dyn_text.cursor_pos == -1) {
						cyx_str_append_char(&text_input->as.dyn_text.text, '\n');
					} else {
						cyx_str_insert_char(&text_input->as.dyn_text.text, '\n', text_input->as.dyn_text.cursor_pos);
						++text_input->as.dyn_text.cursor_pos;
					}
				} else {
					if (cyx_str_length(text_input->as.dyn_text.text)) {
						if (text_input->as.dyn_text.cursor_pos == -1) {
							cyx_str_pop(&text_input->as.dyn_text.text);
						} else if (text_input->as.dyn_text.cursor_pos != 0) {
							cyx_str_remove(&text_input->as.dyn_text.text, text_input->as.dyn_text.cursor_pos - 1);
							--text_input->as.dyn_text.cursor_pos;
						}
					}
				}
			}

			Scene* scene = cyx_hashmap_get(ctx->scenes, ctx->curr_scene);
			if (scene && scene->key_callback) {
				scene->key_callback(ctx, event.key.sym, event.key.value);
			}

			context_key_events(ctx, event.key.sym, event.key.value);
		} else if (event.type == RGFW_keyReleased) {
			if (event.key.value == 154) {
				ctx->key_info.shift_held = 0;
			} else if (event.key.value == 155) {
				ctx->key_info.ctrl_held = 0;
			} else if (event.key.value == 156) {
				ctx->key_info.alt_held = 0;
			}
		} else if (event.type == RGFW_windowResized) {
			RGFW_window_getSize(ctx->win, &ctx->wh.x, &ctx->wh.y);
			glViewport(0, 0, ctx->wh.x, ctx->wh.y);
		}
	}

	cyx_ring_drain(val, ctx->event_queue) {
		switch (val->type) {
			case EVENT_SHOWABLE_CLICKED:
				if (ctx->showable_clicked && ctx->showable_clicked->type == SHOWABLE_TEXT_INPUT) {
					ctx->curr_text_input = ctx->showable_clicked->id;
					ctx->showable_clicked->as.dyn_text.is_selected = 1;
				} else {
					if (ctx->curr_text_input) {
						ScenePair ret = context_get(ctx, ctx->curr_text_input);
						if (ret.type == SHOWABLE_TEXT_INPUT) {
							((SceneShowable*)ret.ptr)->as.dyn_text.is_selected = 0;
						}
					}
					ctx->curr_text_input = NULL;
				}
				if (ctx->showable_clicked) {
					assert(ctx->showable_clicked->type < DATA_SEPARATOR);
					if (ctx->showable_clicked->click) {
						ctx->showable_clicked->click(ctx, ctx->showable_clicked);
					}
				}
				break;
			case EVENT_TURN_OFF_INPUT: {
				if (ctx->curr_text_input) {
					ScenePair ret = context_get(ctx, ctx->curr_text_input);
					if (ret.type == SHOWABLE_TEXT_INPUT) {
						((SceneShowable*)ret.ptr)->as.dyn_text.is_selected = 0;
					}
				}
				ctx->curr_text_input = NULL;
			} break;
			default: assert(0 && "UNREACHABLE");
		}
	}
}
static void context_show(Context* ctx) {
	glClearColor(0x18/255.0f, 0x18/255.0f, 0x18/255.0f, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	grid_init(ctx);
	if (ctx->scenes && ctx->curr_scene) {
		Scene* s = cyx_hashmap_get(ctx->scenes, ctx->curr_scene);
		if (s) {
			if (!s->setup_done) {
				if (s->setup) { s->setup(ctx); }
				s->setup_done = 1;
			}
			s->show(ctx); 

			cyx_array_sort(ctx->z_buffer);
			for (size_t i = 0; i < cyx_array_length(ctx->z_buffer); ++i) {
				ZBuffer buf = ctx->z_buffer[i];
				scene_data_show(ctx, buf.id, buf.x, buf.y, buf.w, buf.h);
			}

			if (ctx->mouse_info.pressed) {
				cyx_array_reverse(ctx->z_buffer);

				for (size_t i = 0; i < cyx_array_length(ctx->z_buffer); ++i) {
					ZBuffer buf = ctx->z_buffer[i];
					ScenePair ret = context_get(ctx, buf.id);
					assert(ret.type < DATA_SEPARATOR);
					if (scene_data_get_clicked(ctx, ret.ptr, ctx->mouse_info.pos, vec2i(buf.x, buf.y), vec2i(buf.w, buf.h))) {
						ctx->showable_clicked = ret.ptr;
						goto found;
					}
				}
				ctx->showable_clicked = NULL;
found: 
				ctx->mouse_info.pressed = 0;
				cyx_ring_push(ctx->event_queue, CUSTOM_EVENT(EVENT_SHOWABLE_CLICKED));
			}
		}
	}
}
void context_setup(Context* ctx, size_t scene_count, SceneDescription* scenes) {
	srand(time(NULL));
// timer
	ctx->timer = (Timer){ .log = 1, .fps_cap = 0 };

// window data
	{ // Setting up OpenGL hints
		RGFW_glHints* hints = RGFW_getGlobalHints_OpenGL();
		hints->major = 3;
		hints->minor = 3;
		hints->profile = RGFW_glCore;
		RGFW_setGlobalHints_OpenGL(hints);
	}
	ctx->wh = vec2i(WIDTH, HEIGHT);
	ctx->win = RGFW_createWindow("Grphing Calculator", 0, 0, WIDTH, HEIGHT, RGFW_windowCenter | RGFW_windowOpenGL);
	ctx->fullscreen = 0;
	RGFW_window_swapInterval_OpenGL(ctx->win, 0);

	GLenum err = glewInit();
	if (err != GLEW_OK) {
		fprintf(stderr, "ERROR:\tUnable to initialize GLEW!\nLOG: %s\n", glewGetErrorString(err));
		exit(1);
	}
	printf("LOG:\tOpenGL version supported: %s\n", glGetString(GL_VERSION));

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glViewport(0, 0, WIDTH, HEIGHT);

// opengl programs
	context_compile_programs(ctx);

// arena for stuff that needs to be saved over the lifetime of the app
	ctx->perm_arena = evo_arena_new(EVO_KB(64));
	ctx->perm = evo_allocator_arena(&ctx->perm_arena);

// arena for per frame resets
	ctx->temp_arena = evo_arena_new(EVO_KB(8));
	ctx->temp = evo_allocator_arena(&ctx->temp_arena);
	
// scene values part
	ctx->scene_pool = evo_pool_new(sizeof(SceneShowable));
	ctx->scene_alloc = evo_allocator_pool(&ctx->scene_pool);

	ctx->showables = cyx_hashmap_new(SceneShowableKV, &ctx->perm, cyx_hash_str, cyx_eq_str, .is_key_ptr = 1);
	ctx->values = cyx_hashmap_new(SceneValueKV, &ctx->perm, cyx_hash_str, cyx_eq_str, .is_key_ptr = 1);

	ctx->grids = cyx_array_new(Grid, &ctx->perm, .reserve = 64);

// font data
	ctx->font_dir = cyx_str_from_lit(&ctx->perm, "./resources/fonts/");
	ctx->fonts = cyx_hashmap_new(FontKV, &ctx->perm, name_size_hash, name_size_eq);
	context_add_font(ctx, "JetBrainsMonoNerdFont-Bold", 30);

// event queue, FIFO
	ctx->event_queue = cyx_ring_new(CustomEvent, &ctx->perm);

// text input data
	ctx->curr_text_input = NULL;
	ctx->key_info.alt_held = 0;
	ctx->key_info.ctrl_held = 0;
	ctx->key_info.shift_held = 0;

// scene
	if (!scene_count) { return; }
	ctx->curr_scene = cyx_str_from_lit(&ctx->perm, scenes[0].name);
	ctx->scenes = cyx_hashmap_new(SceneKV, &ctx->perm, cyx_hash_str, cyx_eq_str, .is_key_ptr = 1);
	for (size_t i = 0; i < scene_count; ++i) {
		cyx_hashmap_add_v(ctx->scenes, cyx_str_from_lit(&ctx->perm, scenes[i].name), ((Scene){
			.setup_done = 0,
			.setup = scenes[i].setup,
			.show = scenes[i].show,
			.key_callback = scenes[i].key_callback,
		}));
	}
}
void context_update(Context* ctx) {
	while (!RGFW_window_shouldClose(ctx->win)) {
		evo_alloc_reset(&ctx->temp);
		ctx->grids = cyx_array_new(Grid, &ctx->temp);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		context_show(ctx);
		context_events(ctx);

		RGFW_window_swapBuffers_OpenGL(ctx->win);
		glFlush();
		// printf("Perm filled: %zu\tTemp filled: %zu\n", evo_arena_size(&ctx->perm_arena), evo_arena_size(&ctx->temp_arena));
	}
}
void context_cleanup(Context* ctx) {
	evo_alloc_destroy(&ctx->perm);
	evo_alloc_destroy(&ctx->temp);
	evo_alloc_destroy(&ctx->scene_alloc);

	for (size_t i = 0; i < PROGRAM_COUNT; ++i) {
		glDeleteProgram(ctx->programs[i]);
	}
	RGFW_window_close(ctx->win);
}

