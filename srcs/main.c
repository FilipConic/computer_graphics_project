#include <color.h>
#include <cube_marching.h>
#include <mat.h>
#include <obj_parse.h>

#define CYLIBX_ALLOC
#include <cylibx.h>
#include <immediate.h>

enum CubeMarchingState {
	NOTHING,
	CALCULATING,
	ERROR_HAPPEND,
	FINISHED,
};
enum FileState {
	FILE_NOTHING,
	FILE_SAVE,
	FILE_OPEN,
	SHOW_HELP,
	SHOW_ERROR,
};

void click_escape_overlay(Context* ctx, SceneShowable* showable) {
	(void)showable;
	grid_get_i(ctx, "file_overlay_on") = 0;
}

void click_red(Context* ctx, SceneShowable* showable) {
	(void)showable;
	grid_get_color(ctx, "shape_color") = COLOR_RED;
}
void click_orange(Context* ctx, SceneShowable* showable) {
	(void)showable;
	grid_get_color(ctx, "shape_color") = COLOR_ORANGE;
}
void click_yellow(Context* ctx, SceneShowable* showable) {
	(void)showable;
	grid_get_color(ctx, "shape_color") = COLOR_YELLOW;
}
void click_lime(Context* ctx, SceneShowable* showable) {
	(void)showable;
	grid_get_color(ctx, "shape_color") = COLOR_LIME;
}
void click_green(Context* ctx, SceneShowable* showable) {
	(void)showable;
	grid_get_color(ctx, "shape_color") = COLOR_GREEN;
}
void click_light_blue(Context* ctx, SceneShowable* showable) {
	(void)showable;
	grid_get_color(ctx, "shape_color") = COLOR_LIGHT_BLUE;
}
void click_blue(Context* ctx, SceneShowable* showable) {
	(void)showable;
	grid_get_color(ctx, "shape_color") = COLOR_BLUE;
}
void click_purple(Context* ctx, SceneShowable* showable) {
	(void)showable;
	grid_get_color(ctx, "shape_color") = COLOR_PURPLE;
}
void click_white(Context* ctx, SceneShowable* showable) {
	(void)showable;
	grid_get_color(ctx, "shape_color") = COLOR_WHITE;
}
void click_black(Context* ctx, SceneShowable* showable) {
	(void)showable;
	grid_get_color(ctx, "shape_color") = COLOR_BLACK;
}
void click_show_help(Context* ctx, SceneShowable* showable) {
	(void)showable;
	grid_get_i(ctx, "file_state") = SHOW_HELP;
	push_event(ctx, EVENT_TURN_OFF_INPUT);
	grid_get_i(ctx, "file_overlay_on") = 1;
}

void main_setup(Context* ctx) {
	grid_get_i(ctx, "show_name") = 1;

	grid_get_i(ctx, "calculating_cubes") = NOTHING;

	grid_get_ptr(ctx, "indices") = cyx_array_new(uint32_t, &ctx->perm);
	grid_get_ptr(ctx, "vertices") = cyx_array_new(float, &ctx->perm);
	grid_get_f(ctx, "yaw") = -45.0;
	grid_get_f(ctx, "pitch") = 45.0;
	grid_get_v4(ctx, "camera") = vec4(3, 0, 0);

	grid_get_f(ctx, "light_yaw") = -45.0;
	grid_get_f(ctx, "light_pitch") = 45.0;
	grid_get_v4(ctx, "light") = vec4(3, 0, 0);
	grid_get_color(ctx, "light_color") = COLOR_WHITE;

	grid_get_color(ctx, "shape_color") = COLOR_RED;

	grid_get_ptr(ctx, "error_msg") = cyx_str_new(&ctx->perm);
	grid_get_ptr(ctx, "graph_indices") = cyx_array_new(uint32_t, &ctx->perm);
	grid_get_ptr(ctx, "graph_vertices") = cyx_array_new(float, &ctx->perm);

	obj_parse("./resources/objs/graph3d.obj", (uint32_t**)&grid_get_ptr(ctx, "graph_indices"), (float**)&grid_get_ptr(ctx, "graph_vertices"));

	grid_get_ptr(ctx, "light_indices") = cyx_array_new(uint32_t, &ctx->perm, .reserve = 48);
	grid_get_ptr(ctx, "light_vertices") = cyx_array_new(float, &ctx->perm, .reserve = 36);

	float** light_vertices = (float**)&grid_get_ptr(ctx, "light_vertices");
	cyx_array_append_mult(*light_vertices,
		-1, -1, -1,  -0.577, -0.577, -0.577,
		 1, -1, -1,   0.577, -0.577, -0.577,
		 1,  1, -1,   0.577,  0.577, -0.577,
		-1,  1, -1,  -0.577,  0.577, -0.577,
		-1, -1,  1,  -0.577, -0.577,  0.577,
		 1, -1,  1,   0.577, -0.577,  0.577,
		 1,  1,  1,   0.577,  0.577,  0.577,
		-1,  1,  1,  -0.577,  0.577,  0.577
	);
	uint32_t** light_indices = (uint32_t**)&grid_get_ptr(ctx, "light_indices");
	cyx_array_append_mult(*light_indices, 
	    4, 5, 6,  4, 6, 7,
		1, 0, 3,  1, 3, 2,
		7, 6, 2,  7, 2, 3,
		0, 1, 5,  0, 5, 4,
		5, 1, 2,  5, 2, 6,
		0, 4, 7,  0, 7, 3
	);

	grid_get_i(ctx, "cull_faces") = 1;
	grid_get_i(ctx, "depth_test") = 1;

	grid_list_start(ctx, "colors1"); {
		grid_list_rect(ctx, 0, COLOR_RED, .border_color = COLOR_WHITE, .border_width = 5, .click_func = click_red);
		grid_list_rect(ctx, 1, COLOR_ORANGE, .border_color = COLOR_WHITE, .border_width = 5, .click_func = click_orange);
		grid_list_rect(ctx, 2, COLOR_YELLOW, .border_color = COLOR_WHITE, .border_width = 5, .click_func = click_yellow);
		grid_list_rect(ctx, 3, COLOR_LIME, .border_color = COLOR_WHITE, .border_width = 5, .click_func = click_lime);
		grid_list_rect(ctx, 4, COLOR_GREEN, .border_color = COLOR_WHITE, .border_width = 5, .click_func = click_green);
	} grid_list_end(ctx);
	grid_list_start(ctx, "colors2"); {
		grid_list_rect(ctx, 0, COLOR_LIGHT_BLUE, .border_color = COLOR_WHITE, .border_width = 5, .click_func = click_light_blue);
		grid_list_rect(ctx, 1, COLOR_BLUE, .border_color = COLOR_WHITE, .border_width = 5, .click_func = click_blue);
		grid_list_rect(ctx, 2, COLOR_PURPLE, .border_color = COLOR_WHITE, .border_width = 5, .click_func = click_purple);
		grid_list_rect(ctx, 3, COLOR_WHITE, .border_color = COLOR_WHITE, .border_width = 5, .click_func = click_white);
		grid_list_rect(ctx, 4, COLOR_BLACK, .border_color = COLOR_WHITE, .border_width = 5, .click_func = click_black);
	} grid_list_end(ctx);

	grid_dyn_text(ctx, 0, "overlay_text", .padding = 15);
}
void main_show(Context* ctx) {
	grid_start(ctx, 0, GRID_VERTICAL, 1, 3, 4); {
		grid_start(ctx, 0, GRID_HORIZONTAL, 10, 1); {
			grid_text(ctx, 0, "f", "f(x,y,z)=0", .x = 15, .y = 15);
			grid_rect(ctx, 1, "help_rect", COLOR_GRAY, .border_width = 10, .border_color = COLOR_WHITE, .padding = 5, .click_func = click_show_help);
			grid_text(ctx, 1, "help_text", "Help", .center = 1);
		} grid_end(ctx);

		grid_start(ctx, 1, GRID_HORIZONTAL, 15, 1, 2, 2); {
			grid_rect(ctx, 0, "terminal_rect", COLOR_GRAY, .border_color = COLOR_WHITE, .border_width = 10);
			grid_dyn_text(ctx, 0, "terminal_text", .text_wrap = 1, .padding = 15);

			grid_list_show(ctx, grid_get_list(ctx, "colors1"), 2, GRID_VERTICAL, 1, 1, 1, 1, 1);
			grid_list_show(ctx, grid_get_list(ctx, "colors2"), 3, GRID_VERTICAL, 1, 1, 1, 1, 1);
		} grid_end(ctx);

		Vec4 pitch_vec = mat4_mult_vec4(mat4_rotation(grid_get_f(ctx, "pitch"), vec4(0, 0, 1)), grid_get_v4(ctx, "camera"));
		Vec4 camera = mat4_mult_vec4(mat4_rotation(grid_get_f(ctx, "yaw"), vec4(0, 1, 0)), pitch_vec);

		Mat4 pitch_mat = mat4_rotation(grid_get_f(ctx, "light_pitch"), vec4(0, 0, 1));
		Mat4 yaw_mat = mat4_rotation(grid_get_f(ctx, "light_yaw"), vec4(0, 1, 0));

		Vec4 light_pos = mat4_mult_vec4(yaw_mat, mat4_mult_vec4(pitch_mat, grid_get_v4(ctx, "light")));
		Vec4 light_cube_pos = mat4_mult_vec4(yaw_mat, mat4_mult_vec4(pitch_mat, vec4_add(grid_get_v4(ctx, "light"), vec4(0.5, 0, 0))));
		grid_shape(ctx, 2, "graph3d",
			vec4(0, 0, 0),
			COLOR_WHITE,
			35.f,
			grid_get_ptr(ctx, "graph_indices"),
			grid_get_ptr(ctx, "graph_vertices"),
			camera,
			light_pos,
			grid_get_color(ctx, "light_color"),
			.padding = 20,
			.cull_faces = grid_get_i(ctx, "cull_faces"),
			.depth_test = grid_get_i(ctx, "depth_test"),
			.shininess = 128,
			.reflectivity = 1.f,
		);
		grid_shape(ctx, 2, "light_cube",
			light_cube_pos,
			COLOR_WHITE,
			35.f,
			grid_get_ptr(ctx, "light_indices"),
			grid_get_ptr(ctx, "light_vertices"),
			camera,
			light_pos,
			grid_get_color(ctx, "light_color"),
			.padding = 20,
			.cull_faces = grid_get_i(ctx, "cull_faces"),
			.depth_test = grid_get_i(ctx, "depth_test"),
			.shininess = 128,
			.reflectivity = 1.f,
		);
		if (grid_get_i(ctx, "calculating_cubes") == FINISHED) {
			grid_shape(ctx, 2, "shape3d",
				vec4(0, 0, 0),
				grid_get_color(ctx, "shape_color"),
				10.f,
				grid_get_ptr(ctx, "indices"),
				grid_get_ptr(ctx, "vertices"),
				camera,
				light_pos,
				grid_get_color(ctx, "light_color"),
				.padding = 20,
				.cull_faces = grid_get_i(ctx, "cull_faces"),
				.depth_test = grid_get_i(ctx, "depth_test"),
				.shininess = 128,
				.reflectivity = 1.0f,
			);
		}
		grid_rect(ctx, 2, "shape_rect", COLOR_NONE, .border_color = COLOR_WHITE, .border_width = 10, .padding = 10);
		if (grid_get_i(ctx, "show_name")){
			grid_text(ctx, 2, "name", "Filip Conic, RA126/2022", .x = -400, .y = -10, .padding = 20);
		}
	} grid_end(ctx);

	if (grid_get_i(ctx, "file_overlay_on")) {
		grid_start(ctx, 0, GRID_VERTICAL, 1); {
			grid_rect(ctx, 0, "overlay_bg", ((Color){ .a = 153 }), .click_func = click_escape_overlay);

			if (grid_get_i(ctx, "file_state") == FILE_OPEN || grid_get_i(ctx, "file_state") == FILE_SAVE) {
				grid_start(ctx, 0, GRID_VERTICAL, 1, 3, 1); {
					grid_start(ctx, 1, GRID_HORIZONTAL, 4, 1, 4); {
						grid_text(ctx, 0, "overlay_text_type", grid_get_i(ctx, "file_state") == FILE_SAVE ? "Save file:" : "Open file:", .y = -50);
						grid_rect(ctx, 1, "overlay_rect", COLOR_GRAY, .border_color = COLOR_WHITE, .border_width = 10);
						grid_dyn_text(ctx, 1, "overlay_text", .padding = 15);
					} grid_end(ctx);
				} grid_end(ctx);
			} else if (grid_get_i(ctx, "file_state") == SHOW_HELP) {
				grid_start(ctx, 0, GRID_VERTICAL, 1, 9, 1); {
					grid_start(ctx, 1, GRID_HORIZONTAL, 1, 15, 1); {
						grid_rect(ctx, 1, "overlay_help_rect", COLOR_GRAY, .border_color = COLOR_WHITE, .border_width = 10);
						grid_text(ctx, 1, "overlay_help_text", 
								"<C-h>      : Show this 'help' overlay\n"
								"<C-q>      : Close any overlay or escape the input box\n"
								"<C-f>      : Turn on/off fullscreen\n"
								"<C-n>      : Show/Hide author name\n"
								"<C-g>      : Turn on/off back face-culling\n"
								"<C-d>      : Turn on/off depth testing\n"
								"<C-l>      : Turn on/off FPS cap\n"
								"<ESC>      : Turn off the application\n\n"
								"<C-o>      : Show the 'open file' overlay where you can open a saved function\n"
								"<C-s>      : Show the 'save file' overlay where you can save a function you have\n"
								"             currently written\n"
								"<C-i>      : Select the main input box\n"
								"<C-r>      : Compile the function you've written\n\n"
								"WASD       : Move the camera around on a sphere\n"
								"<C-'+'>    : Move the camera closer to the (0, 0)\n"
								"<C-'-'>    : Move the camera away from (0, 0)\n"
								"<C-'='>    : Return the camera and the light to the normal viewing angle\n\n"
								"arrow keys : Move the light around on a sphere\n"
								"<C-'.'>    : Move the light closer to the (0, 0)\n"
								"<C-','>    : Move the ligth away from (0, 0)\n",
								.padding = 15
								);
					} grid_end(ctx);
				} grid_end(ctx);
			} else if (grid_get_i(ctx, "file_state") == SHOW_ERROR) {
				grid_start(ctx, 0, GRID_VERTICAL, 1, 9, 1); {
					grid_start(ctx, 1, GRID_HORIZONTAL, 4, 1, 4); {
						grid_rect(ctx, 1, "error_rect", COLOR_GRAY, .border_color = COLOR_WHITE, .border_width = 10);
						grid_text(ctx, 1, "error_text", grid_get_ptr(ctx, "error_msg"), .center = 1);
					} grid_end(ctx);
				} grid_end(ctx);
			}
		} grid_end(ctx);
	}
}
void main_key(Context* ctx, char key, int value) {
	if (key == 'r' && ctx->key_info.ctrl_held) {
		push_event(ctx, EVENT_TURN_OFF_INPUT);
		char* str = cyx_str_copy_a(&ctx->temp, grid_get_text(ctx, "terminal_text"));
		printf(CYX_STR_FMT"\n", CYX_STR_UNPACK(str));
		char** err_msg = (char**)&grid_get_ptr(ctx, "error_msg");

		uint32_t** indices = (uint32_t**)&grid_get_ptr(ctx, "indices");
		float** vertices = (float**)&grid_get_ptr(ctx, "vertices");

		CubeMarchDefintions defs = {
			.res = 50,
			.left = -20.0, .right = 20.0,
			.bottom = -20.0, .top = 20.0,
			.near = -20.0, .far = 20.0,
		};

		if (grid_get_i(ctx, "calculating_cubes") == NOTHING) {
			grid_get_i(ctx, "calculating_cubes") = CALCULATING;

			if (!cube_march(indices, vertices, str, NULL, defs, err_msg)) {
				cyx_str_append_char(err_msg, '\0');
				cyx_str_replace(*err_msg, '\t', ' ');
				grid_get_i(ctx, "calculating_cubes") = ERROR_HAPPEND;
				grid_get_i(ctx, "file_overlay_on") = 1;
				grid_get_i(ctx, "file_state") = SHOW_ERROR;
			} else {
				printf("here success!\n");
				grid_get_i(ctx, "calculating_cubes") = FINISHED;
			}
		} else if (grid_get_i(ctx, "calculating_cubes") == FINISHED || grid_get_i(ctx, "calculating_cubes") == ERROR_HAPPEND) {
			grid_get_i(ctx, "calculating_cubes") = CALCULATING;
		
			ScenePair ret = grid_get(ctx, "shape3d", SHOWABLE_3D);
			if (ret.ptr) { shape3d_free(ret.ptr); }
			cyx_array_clear(*indices);
			cyx_array_clear(*vertices);

			if (!cube_march(indices, vertices, str, NULL, defs, err_msg)) {
				cyx_str_append_char(err_msg, '\0');
				cyx_str_replace(*err_msg, '\t', ' ');
				grid_get_i(ctx, "calculating_cubes") = ERROR_HAPPEND;
				grid_get_i(ctx, "file_overlay_on") = 1;
				grid_get_i(ctx, "file_state") = SHOW_ERROR;
			} else {
				if (ret.ptr) {
					((SceneShowable*)ret.ptr)->as.shape = shape3d_create(
						ctx->programs[PROGRAM_3D],
						grid_get_color(ctx, "shape_color"),
						20.f, *indices, *vertices
					);
				}
				printf("success!\n");
				grid_get_i(ctx, "calculating_cubes") = FINISHED;
			}
		}
	}

	if (!ctx->curr_text_input && !grid_get_i(ctx, "file_overlay_on")) {
		float* yaw = &grid_get_f(ctx, "yaw");
		float* pitch = &grid_get_f(ctx, "pitch");

		float* light_yaw = &grid_get_f(ctx, "light_yaw");
		float* light_pitch = &grid_get_f(ctx, "light_pitch");

		if (key == 'a') {
			*yaw -= 1000 * ctx->timer.dt;
		} 
		if (key == 'd') {
			*yaw += 1000 * ctx->timer.dt;
		}
		if (key == 'w') {
			*pitch += 1000 * ctx->timer.dt;
		}
		if (key == 's') {
			*pitch -= 1000 * ctx->timer.dt;
		} else if (key == '+' && ctx->key_info.ctrl_held) {
			grid_get_v4(ctx, "camera").x -= 100 * ctx->timer.dt;
			if (grid_get_v4(ctx, "camera").x <= 0.35f) {
				grid_get_v4(ctx, "camera").x = 0.35f;
			}
		} else if (key == '-' && ctx->key_info.ctrl_held) {
			grid_get_v4(ctx, "camera").x += 100 * ctx->timer.dt;
		} else if (key == '=' && ctx->key_info.ctrl_held) {
			grid_get_v4(ctx, "camera").x = 3.0f;
			grid_get_v4(ctx, "light").x = 3.0f;
		} 
		if (value == 162) { // up
			*light_pitch += 1000 * ctx->timer.dt;
		} 
		if (value == 163) { // down
			*light_pitch -= 1000 * ctx->timer.dt;
		} 
		if (value == 164) { // left
			*light_yaw -= 1000 * ctx->timer.dt;
		} 
		if (value == 165) { // right
			*light_yaw += 1000 * ctx->timer.dt;
		} 
		if (key == '.' && ctx->key_info.ctrl_held) {
			grid_get_v4(ctx, "light").x -= 100 * ctx->timer.dt;
			if (grid_get_v4(ctx, "light").x <= 0.35f) {
				grid_get_v4(ctx, "light").x = 0.35f;
			}
		} else if (key == ',' && ctx->key_info.ctrl_held) {
			grid_get_v4(ctx, "light").x += 100 * ctx->timer.dt;
		}

		if (*yaw > 180.f) { *yaw -= 360.f; }
		else if (*yaw < -180.f) { *yaw += 360.f; }
		else if (*pitch > 89.f) { *pitch = 89.f; }
		else if (*pitch < -89.f) { *pitch = -89.f; }

		if (*light_yaw > 180.f) { *light_yaw -= 360.f; }
		else if (*light_yaw < -180.f) { *light_yaw += 360.f; }
		else if (*light_pitch > 89.f) { *light_pitch = 89.f; }
		else if (*light_pitch < -89.f) { *light_pitch = -89.f; }
	}

	if (!grid_get_i(ctx, "file_overlay_on")) {
		if (ctx->key_info.ctrl_held) {
			switch (key) {
				case 'd': {
					grid_get_i(ctx, "depth_test") = !grid_get_i(ctx, "depth_test");
				} break;
				case 'l': {
					ctx->timer.fps_cap = !ctx->timer.fps_cap;
				} break;
				case 'g': {
					grid_get_i(ctx, "cull_faces") = !grid_get_i(ctx, "cull_faces");
				} break;
				case 'q': {
					push_event(ctx, EVENT_TURN_OFF_INPUT);
				} break;
				case 'i': {
					ScenePair ret = grid_get(ctx, "terminal_text", SHOWABLE_TEXT_INPUT);
					assert(ret.ptr);
					ctx->showable_clicked = ret.ptr;
					push_event(ctx, EVENT_SHOWABLE_CLICKED);
				} break;
				case 's': {
					grid_get_i(ctx, "file_overlay_on") = 1;
					push_event(ctx, EVENT_TURN_OFF_INPUT);

					grid_get_i(ctx, "file_state") = FILE_SAVE;
					ScenePair ret = grid_get(ctx, "overlay_text", SHOWABLE_TEXT_INPUT);
					if (ret.ptr) {
						cyx_str_clear(((SceneShowable*)ret.ptr)->as.dyn_text.text);
					}
					ctx->showable_clicked = ret.ptr;
					push_event(ctx, EVENT_SHOWABLE_CLICKED);
			  	} break;
				case 'o': {
					grid_get_i(ctx, "file_overlay_on") = 1;
					push_event(ctx, EVENT_TURN_OFF_INPUT);

					grid_get_i(ctx, "file_state") = FILE_OPEN;
					ScenePair ret = grid_get(ctx, "overlay_text", SHOWABLE_TEXT_INPUT);
					if (ret.ptr) {
						cyx_str_clear(((SceneShowable*)ret.ptr)->as.dyn_text.text);
					}
					ctx->showable_clicked = ret.ptr;
					push_event(ctx, EVENT_SHOWABLE_CLICKED);
				} break;
				case 'h': {
					grid_get_i(ctx, "file_overlay_on") = 1;
					push_event(ctx, EVENT_TURN_OFF_INPUT);

					grid_get_i(ctx, "file_state") = SHOW_HELP;
				} break;
				default: break;
			}
		}
	} else {
		if (ctx->key_info.ctrl_held && key == 'q') {
			grid_get_i(ctx, "file_overlay_on") = 0;

			grid_get_i(ctx, "file_state") = FILE_NOTHING;
			push_event(ctx, EVENT_TURN_OFF_INPUT);
		}
		if (value == 10) {
			int file_state = grid_get_i(ctx, "file_state");
			if (file_state == FILE_SAVE) {
				ScenePair ret = grid_get(ctx, "overlay_text", SHOWABLE_TEXT_INPUT);
				if (ret.ptr) {
					char* file_path = cyx_str_from_lit(&ctx->temp, "./resources/saved/");
					cyx_str_append_str(&file_path, ((SceneShowable*)ret.ptr)->as.dyn_text.text);
					cyx_str_append_char(&file_path, '\0');
					FILE* file = fopen(file_path, "w+");
					if (!file) {
						fprintf(stderr, "ERROR:\tError happened while creating file!\n");
					} else {
						ScenePair ret = grid_get(ctx, "terminal_text", SHOWABLE_TEXT_INPUT);
						if (ret.ptr) {
							char* text = ((SceneShowable*)ret.ptr)->as.dyn_text.text;
							fwrite(text, sizeof(char), cyx_str_length(text), file);
						}

						grid_get_i(ctx, "file_overlay_on") = 0;
						grid_get_i(ctx, "file_state") = FILE_NOTHING;
						push_event(ctx, EVENT_TURN_OFF_INPUT);
						fclose(file);
					}
				}
			} else if (file_state == FILE_OPEN) {
				ScenePair ret = grid_get(ctx, "overlay_text", SHOWABLE_TEXT_INPUT);
				if (ret.ptr) {
					char* file_path = cyx_str_from_lit(&ctx->temp, "./resources/saved/");
					cyx_str_append_str(&file_path, ((SceneShowable*)ret.ptr)->as.dyn_text.text);
					--cyx_str_length(file_path);
					cyx_str_append_char(&file_path, '\0');

					ScenePair ret = grid_get(ctx, "terminal_text", SHOWABLE_TEXT_INPUT);
					if (ret.ptr) {
						cyx_str_clear(((SceneShowable*)ret.ptr)->as.dyn_text.text);
						char* file_text = cyx_str_from_file(&ctx->temp, file_path);
						cyx_str_append_str(&((SceneShowable*)ret.ptr)->as.dyn_text.text, file_text);
					}

					grid_get_i(ctx, "file_overlay_on") = 0;
					grid_get_i(ctx, "file_state") = FILE_NOTHING;
					push_event(ctx, EVENT_TURN_OFF_INPUT);
				}
			}
		}
	}

	if (key == 'n' && ctx->key_info.ctrl_held) {
		grid_get_i(ctx, "show_name") = !grid_get_i(ctx, "show_name");
	}
}

SceneDescription scenes[] = { 
	{
		.name = "main",
		.setup = main_setup,
		.show = main_show,
		.key_callback = main_key,
	},
};

int main() {
	Context ctx = { 0 };

	context_setup(&ctx, sizeof(scenes)/sizeof(*scenes), scenes);
	// glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	context_update(&ctx);
	context_cleanup(&ctx);

	return 0;
}
