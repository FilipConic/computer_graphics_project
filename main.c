#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#define RGFW_IMPLEMENTATION
#define RGFW_OPENGL
#include <RGFW.h>
#include <glew.h>

#define CYLIBX_IMPLEMENTATION
#include <cylibx.h>

#define WIDTH 800
#define HEIGHT 600

void esc_callback(RGFW_window* win, u8 key, u8 keyChar, RGFW_keymod keyMod, RGFW_bool repeat, RGFW_bool pressed) {
	RGFW_UNUSED(keyChar);
	RGFW_UNUSED(keyMod);
	RGFW_UNUSED(repeat);
	if ((key == RGFW_escape || key == RGFW_q) && pressed) { RGFW_window_setShouldClose(win, 1); }
}

#define check_shader_compile_status(shader) do { \
	int success; \
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success); \
	if (!success) { \
		char info_log[512]; \
		glGetShaderInfoLog(shader, 512, NULL, info_log);\
		fprintf(stderr, "ERROR:\tUnable to compile shader!\nLOG:\t%s\n", info_log); \
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

#define rand0_1 ((double)rand() / RAND_MAX)

typedef struct { float r, g, b, a; } fColor;
#define rand_color_f() ((fColor){ .r = rand0_1, .g = rand0_1, .b = rand0_1, .a = rand0_1 })
#define set_uniform_color(uniform, color) glUniform4f(uniform, color.r, color.g, color.b, color.a)

typedef struct {
	u32 vao, vbo, ebo;
	u32 program;
	u32 count;

	float x, y, r;
	float vx, vy;
	fColor clr;
} DrawnObject;

DrawnObject gen_circle(int pos_x, int pos_y, int radius, size_t step_count) {
	assert(step_count > 2);
	float* vertices = cyx_array_new(float);
	float a = 0;
	float step = 2 * M_PI / step_count;
	float rx = (float)radius / WIDTH * 2;
	float ry = (float)radius / HEIGHT * 2;

	cyx_array_append_mult(vertices, 0.0f, 0.0f, 0.0f);
	for (size_t i = 0; i < step_count; ++i, a += step) {
		cyx_array_append_mult(vertices, rx * cosf(a), ry * sinf(a), 0.0f);
	}

	u32* indicies = cyx_array_new(u32);
	cyx_array_append_mult(indicies, 0, step_count, 1);
	for (size_t i = 2; i <= step_count; ++i) {
		cyx_array_append_mult(indicies, 0, i - 1, i);
	}

	DrawnObject obj = { .count = cyx_array_length(indicies) };
	obj.x = pos_x;
	obj.y = pos_y;
	obj.vx = rand0_1;
	obj.vy = rand0_1;
	obj.r = radius;

	glGenVertexArrays(1, &obj.vao);
	glBindVertexArray(obj.vao);

	glGenBuffers(1, &obj.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, obj.vbo);
	glBufferData(GL_ARRAY_BUFFER, cyx_array_length(vertices) * sizeof(float), vertices, GL_STATIC_DRAW);

	glGenBuffers(1, &obj.ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj.ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, cyx_array_length(indicies) * sizeof(u32), indicies, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), NULL);
	glEnableVertexAttribArray(0);

	cyx_array_free(indicies);
	cyx_array_free(vertices);
	return obj;
}
u32 compile_program(const char* vert_file, const char* frag_file) {
	char* source = cyx_str_from_file(vert_file);
	u32 vert_shader = glCreateShader(GL_VERTEX_SHADER);
	i32 len = cyx_str_length(source);
	glShaderSource(vert_shader, 1, (const char* const*)&source, &len);
	glCompileShader(vert_shader);
	check_shader_compile_status(vert_shader);
	cyx_str_free(source);

	source = cyx_str_from_file(frag_file);
	u32 frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
	len = cyx_str_length(source);
	glShaderSource(frag_shader, 1, (const char* const*)&source, &len);
	glCompileShader(frag_shader);
	check_shader_compile_status(frag_shader);
	cyx_str_free(source);

	u32 program = glCreateProgram();
	glAttachShader(program, vert_shader);
	glAttachShader(program, frag_shader);
	glLinkProgram(program);
	check_program_compile_status(program);

	glDeleteShader(vert_shader);
	glDeleteShader(frag_shader);
	return program;
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

#define from_big_endian16(val) do { val = (((val & 0xff00) >> 8) | ((val & 0xff) << 8)); } while(0)
#define from_big_endian32(val) do { val = (((val & 0xff000000) >> 24) | ((val & 0xff0000) >> 8) | ((val & 0xff00) << 8) | ((val & 0xff) << 24)); } while(0)
#define from_big_endian64(val) do { val = \
	(((val & 0xff00000000000000ull) >> 56) | \
	 ((val & 0xff000000000000ull) >> 40) | \
	 ((val & 0xff0000000000ull) >> 24) | \
	 ((val & 0xff00000000ull) >> 8) | \
	 ((val & 0xff000000ull) << 8) | \
	 ((val & 0xff0000ull) << 24) | \
	 ((val & 0xff00ull) << 40) | \
	 ((val & 0xffull) << 56)); \
} while(0)

typedef struct {
	u32 tag;
	u32 checksum;
	u32 offset;
	u32 length;
} Table; 
typedef struct {
	i32 key;
	Table value;
} TableKV;
typedef i16 FWord;
typedef struct {
	u16* end_points;
	u8* instrs;
	u8* flags;
	i16* x_coords;
	i16* y_coords;
} SimpleGlyph;
typedef struct {
	char place_holder;
} CompoundGlyph;
typedef struct {
	struct {
		i16 number_of_contours;
		FWord x_min;
		FWord y_min;
		FWord x_max;
		FWord y_max;
	} metadata;
	union {
		SimpleGlyph simple;
		CompoundGlyph compound;
	} as;

	enum {
		GLYPH_ERROR,
		GLYPH_SIMPLE,
		GLYPH_COMPOUND,
	} type;
} FontGlyph;

void font_glyph_free(void* glyph_ptr) {
	FontGlyph* glyph = glyph_ptr;
	switch (glyph->type) {
		case GLYPH_SIMPLE: {
			SimpleGlyph* simple = &glyph->as.simple;
			cyx_array_free(simple->end_points); 
			cyx_array_free(simple->instrs);
			cyx_array_free(simple->flags);
			cyx_array_free(simple->x_coords);
			cyx_array_free(simple->y_coords);
		} break;
		case GLYPH_COMPOUND: {
		} break;
		default:
			assert(0 && "UNREACHABLE");
	}
}

typedef struct {
	char* file_path;
	FILE* file;

	struct {
		u32 scaler_type;
		u16 num_tables;
		u16 search_range;
		u16 entry_selector;
		u16 range_shift;
	} offset_subtable;
	struct {
		u32 version;
		u16 num_glyphs;
		u16 max_points;
		u16 max_contours;
		u16 max_component_points;
		u16 max_component_contours;
		u16 max_zones;
		u16 max_twilightpoints;
		u16 max_storage;
		u16 max_function_defs;
		u16 max_instruction_defs;
		u16 max_stack_elements;
		u16 max_size_of_instructions;
		u16 max_component_elements;
		u16 max_component_depth;
	} maxp;
	struct {
		u32	version;
		u32 font_revision;
		u32 check_sum_adjustment;
		u32 magic_number;
		u16	flags;
		u16 units_per_em;
		i64 created;
		i64 modified;
		FWord x_min;
		FWord y_min;
		FWord x_max;
		FWord y_max;
		u16	mac_style;
		u16	lowest_rec_ppem;
		i16	font_direction_hint;
		i16	index_to_loc_format;
		i16	glyph_data_format;
	} head;

	TableKV* table_metadata;
	FontGlyph* glyphs;
} TTF;

#define get_table(map, key) (assert(strlen(key) == 4 && "ERROR: Key has to be of length 4!"), cyx_hashmap_get(map, *(i32*)(key)))
void ttf_free(TTF* ttf) {
	if (ttf->file) { fclose(ttf->file); }
	if (ttf->file_path) { cyx_str_free(ttf->file_path); }
	if (ttf->table_metadata) { cyx_hashmap_free(ttf->table_metadata); }
	if (ttf->glyphs) { cyx_array_free(ttf->glyphs); }
}
#define big_endian_glyph_metadata(glyph) do { \
	from_big_endian16(glyph.metadata.number_of_contours); \
	from_big_endian16(glyph.metadata.x_min); \
	from_big_endian16(glyph.metadata.y_min); \
	from_big_endian16(glyph.metadata.x_max); \
	from_big_endian16(glyph.metadata.y_max); \
} while(0) 
#define flag_read_at(flag, pos) (\
	assert(0 <= (pos) && (pos) < 6 && "ASSERT: Position has to be between 0 and 6"), \
	assert(sizeof(flag) == sizeof(u8) && "ASSERT: Flag can only be of size 1 (byte)!"), \
	(((flag) >> (pos)) & 0b1) \
)

void parse_glyph_simple(SimpleGlyph* simple, TTF* ttf, i16 contour_count) {
	// end points of contours
	simple->end_points = cyx_array_new(u16);
	for (i16 j = 0; j < contour_count; ++j) {
		u16 end_point = 0;
		fread(&end_point, sizeof(u16), 1, ttf->file);
		from_big_endian16(end_point);
		cyx_array_append(simple->end_points, end_point);
	}

	// calculating point count
	u16* end_point = cyx_array_at(simple->end_points, -1);
	assert(end_point && "ASSERT: If this happens there is some error with contours. Problem with TTF!");
	u16 point_count = *end_point + 1;

	// instruction length
	u16 instr_len = 0;
	fread(&instr_len, sizeof(u16), 1, ttf->file);
	from_big_endian16(instr_len);

	// instructions
	simple->instrs = cyx_array_new(u8, .reserve = instr_len);
	for (size_t j = 0; j < instr_len; ++j) {
		u8 instr = 0;
		fread(&instr, sizeof(u8), 1, ttf->file);
		cyx_array_append(simple->instrs, instr);
	}

	// flags
	simple->flags = cyx_array_new(u8, .reserve = point_count);
	for (size_t j = 0; j < point_count; ++j) {
		u8 flag = 0;
		fread(&flag, sizeof(u8), 1, ttf->file);
		cyx_array_append(simple->flags, flag);

		if (flag_read_at(flag, 3)) {
			u8 len = 0;
			fread(&len, sizeof(u8), 1, ttf->file);
			for(u8 k = 0; k < len; ++k) { cyx_array_append(simple->flags, flag); }
			j += len;
		}
	}

	// x coordinates
	simple->x_coords = cyx_array_new(i16, .reserve = point_count);
	for (size_t j = 0; j < point_count; ++j) {
		if (j) {
			i16* prev = cyx_array_at(simple->x_coords, -1);
			assert(prev && "ASSERT: Should never happen!");
			cyx_array_append(simple->x_coords, *prev);
		} else {
			cyx_array_append(simple->x_coords, 0);
		}
		i16* curr = cyx_array_at(simple->x_coords, -1);
		assert(curr && "ASSERT: Should never happen!");

		u8 flag = simple->flags[j];
		if (flag_read_at(flag, 1)) {
			u8 x = 0;
			fread(&x, sizeof(u8), 1, ttf->file);
			*curr += (i16)x * (flag_read_at(flag, 4) ? 1 : -1);
		} else {
			if (!flag_read_at(flag, 4)) {
				i16 x = 0;
				fread(&x, sizeof(i16), 1, ttf->file);
				from_big_endian16(x);
				*curr += x;
			}
		}
	}

	// y coordinates
	simple->y_coords = cyx_array_new(i16, .reserve = point_count);
	for (size_t j = 0; j < point_count; ++j) {
		if (j) {
			i16* prev = cyx_array_at(simple->y_coords, -1);
			assert(prev && "ASSERT: Should never happen!");
			cyx_array_append(simple->y_coords, *prev);
		} else {
			cyx_array_append(simple->y_coords, 0);
		}
		i16* curr = cyx_array_at(simple->y_coords, -1);
		assert(curr && "ASSERT: Should never happen!");

		u8 flag = simple->flags[j];
		if (flag_read_at(flag, 2)) {
			u8 y = 0;
			fread(&y, sizeof(u8), 1, ttf->file);
			*curr += (i16)y * (flag_read_at(flag, 5) ? 1 : -1);
		} else {
			if (!flag_read_at(flag, 5)) {
				i16 y = 0;
				fread(&y, sizeof(i16), 1, ttf->file);
				from_big_endian16(y);
				*curr += y;
			}
		}
	}
}
void parse_glyph_compound(CompoundGlyph* comp, TTF* ttf, size_t contour_count) {
	(void)comp;
	(void)ttf;
	(void)contour_count;
	assert(0 && "TODO: Compound Glyph parsing");
}
FontGlyph read_glyph(TTF* ttf) {
	FontGlyph glyph;
	fread(&glyph.metadata, sizeof(glyph.metadata), 1, ttf->file);
	big_endian_glyph_metadata(glyph);

	if (glyph.metadata.number_of_contours >= 0) {
		glyph.type = GLYPH_SIMPLE;
		parse_glyph_simple(&glyph.as.simple, ttf, glyph.metadata.number_of_contours);
	} else {
		glyph.type = GLYPH_COMPOUND;
		parse_glyph_compound(&glyph.as.compound, ttf, glyph.metadata.number_of_contours);
	}
	return glyph;
}
int ttf_parse_head(TTF* ttf) {
	Table* head_table = cyx_hashmap_get(ttf->table_metadata, *(i32*)"head");
	if (!head_table) {
		fprintf(stderr, "ERROR:\tCan not find 'head' table in ["CYX_STR_FMT"]\n", CYX_STR_UNPACK(ttf->file_path));
		return 0;
	}

	fseek(ttf->file, head_table->offset, SEEK_SET);

	fread(&ttf->head, sizeof(ttf->head), 1, ttf->file);
	from_big_endian32(ttf->head.version);
	from_big_endian32(ttf->head.font_revision);
	from_big_endian32(ttf->head.check_sum_adjustment);
	from_big_endian32(ttf->head.magic_number);
	from_big_endian16(ttf->head.flags);
	from_big_endian16(ttf->head.units_per_em);
	from_big_endian64(ttf->head.created);
	from_big_endian64(ttf->head.modified);
	from_big_endian16(ttf->head.x_min);
	from_big_endian16(ttf->head.y_min);
	from_big_endian16(ttf->head.x_max);
	from_big_endian16(ttf->head.y_max);
	from_big_endian16(ttf->head.mac_style);
	from_big_endian16(ttf->head.lowest_rec_ppem);
	from_big_endian16(ttf->head.font_direction_hint);
	from_big_endian16(ttf->head.index_to_loc_format);
	from_big_endian16(ttf->head.glyph_data_format);
		
	return 1;
}
int ttf_parse_maxp(TTF* ttf) {
	Table* maxp_table = get_table(ttf->table_metadata, "maxp");
	if (!maxp_table) {
		fprintf(stderr, "ERROR:\tCan not find 'maxp' table in ["CYX_STR_FMT"]\n", CYX_STR_UNPACK(ttf->file_path));
		return 0;
	}

	fseek(ttf->file, maxp_table->offset, SEEK_SET);

	fread(&ttf->maxp, sizeof(ttf->maxp), 1, ttf->file);
	from_big_endian32(ttf->maxp.version);
	from_big_endian16(ttf->maxp.num_glyphs);
	from_big_endian16(ttf->maxp.max_points);
	from_big_endian16(ttf->maxp.max_contours);
	from_big_endian16(ttf->maxp.max_component_points);
	from_big_endian16(ttf->maxp.max_component_contours);
	from_big_endian16(ttf->maxp.max_zones);
	from_big_endian16(ttf->maxp.max_twilightpoints);
	from_big_endian16(ttf->maxp.max_storage);
	from_big_endian16(ttf->maxp.max_function_defs);
	from_big_endian16(ttf->maxp.max_instruction_defs);
	from_big_endian16(ttf->maxp.max_stack_elements);
	from_big_endian16(ttf->maxp.max_size_of_instructions);
	from_big_endian16(ttf->maxp.max_component_elements);
	from_big_endian16(ttf->maxp.max_component_depth);

	return 1;
}
int ttf_parse_glyf(TTF* ttf) {
	Table* glyph_table = get_table(ttf->table_metadata, "glyf");
	if (!glyph_table) {
		fprintf(stderr, "ERROR:\tCan not find 'glyf' table in ["CYX_STR_FMT"] font file!\n", CYX_STR_UNPACK(ttf->file_path));
		return 0;
	}

	fseek(ttf->file, glyph_table->offset, SEEK_SET);

	ttf->glyphs = cyx_array_new(FontGlyph, .reserve = ttf->maxp.num_glyphs, .defer_fn = font_glyph_free);
	for (size_t i = 0; i < 1; ++i) { //ttf->maxp.num_glyphs; ++i) {
		cyx_array_append(ttf->glyphs, read_glyph(ttf));
	}
	return 1;
}
void ttf_print(TTF* ttf) {
	printf("TTF: {\n");
	printf("  offset_subtable: {\n"); {
		printf("    scaler_type:  0x%x\n", ttf->offset_subtable.scaler_type);
		printf("    num_tables:  %hu\n", ttf->offset_subtable.num_tables);
		printf("    search_range:  %hu\n", ttf->offset_subtable.search_range);
		printf("    entry_selector:  %hu\n", ttf->offset_subtable.entry_selector);
		printf("    range_shift:  %hu\n", ttf->offset_subtable.range_shift);
	
	} printf("  },\n");
	printf("  Tables: {\n"); {
		cyx_hashmap_foreach(val, ttf->table_metadata) {
			printf("    \"%.*s\": {\n", 4, (char*)&val->key); {
				printf("      tag:  \"%.*s\",\n", 4, (char*)&val->value.tag);
				printf("      checksum:  %u,\n", val->value.offset);
				printf("      offset:  %u,\n", val->value.offset);
				printf("      length:  %u,\n", val->value.length);
			} printf("    },\n");
		}
	} printf("  },\n");
	printf("  maxp: {\n"); {
		printf("    version: 0x%x,\n", (i32)ttf->maxp.version);
		printf("    num_glyphs: %d,\n", (i32)ttf->maxp.num_glyphs);
		printf("    max_points: %d,\n", (i32)ttf->maxp.max_points);
		printf("    max_contours: %d,\n", (i32)ttf->maxp.max_contours);
		printf("    max_component_points: %d,\n", (i32)ttf->maxp.max_component_points);
		printf("    max_component_contours: %d,\n", (i32)ttf->maxp.max_component_contours);
		printf("    max_zones: %d,\n", (i32)ttf->maxp.max_zones);
		printf("    max_twilightpoints: %d,\n", (i32)ttf->maxp.max_twilightpoints);
		printf("    max_storage: %d,\n", (i32)ttf->maxp.max_storage);
		printf("    max_function_defs: %d,\n", (i32)ttf->maxp.max_function_defs);
		printf("    max_instruction_defs: %d,\n", (i32)ttf->maxp.max_instruction_defs);
		printf("    max_stack_elements: %d,\n", (i32)ttf->maxp.max_stack_elements);
		printf("    max_size_of_instructions: %d,\n", (i32)ttf->maxp.max_size_of_instructions);
		printf("    max_component_elements: %d,\n", (i32)ttf->maxp.max_component_elements);
		printf("    max_component_depth: %d,\n", (i32)ttf->maxp.max_component_depth);
	} printf("  },\n");
	printf("}\n");
}
TTF ttf_parse(const char* file_path) {
	TTF res = { .file_path = cyx_str_from_lit(file_path), };

	res.file = fopen(file_path, "rb");
	if (!res.file) {
		fprintf(stderr, "ERROR:\tUnable to open file [%s]!\n", file_path);
		ttf_free(&res);
		return (TTF){ 0 };
	}

	if (fread(&res.offset_subtable, sizeof(res.offset_subtable), 1, res.file) != 1) {
		fprintf(stderr, "ERROR:\tUnable to parse the first subtable of [%s]!\n", file_path);
		ttf_free(&res);
		return (TTF){ 0 };
	}
	{
		from_big_endian32(res.offset_subtable.scaler_type);
		from_big_endian16(res.offset_subtable.num_tables);
		from_big_endian16(res.offset_subtable.search_range);
		from_big_endian16(res.offset_subtable.entry_selector);
		from_big_endian16(res.offset_subtable.range_shift);
	}

	if (res.offset_subtable.scaler_type != 0x00010000 && res.offset_subtable.scaler_type != 0x74727565) {
		fprintf(stderr, "ERROR:\tScaler type of [%s] font is not valid for True Type!\n", res.file_path);
		ttf_free(&res);
		return (TTF){ 0 };
	}

	res.table_metadata = cyx_hashmap_new(TableKV, cyx_hash_int32, cyx_eq_int32);
	Table table;
	for (size_t i = 0; i < res.offset_subtable.num_tables; ++i) {
		fread(&table, sizeof(Table), 1, res.file);
		from_big_endian32(table.checksum);
		from_big_endian32(table.offset);
		from_big_endian32(table.length);
		cyx_hashmap_add_v(res.table_metadata, table.tag, table);
	}

	if (!ttf_parse_maxp(&res) || !ttf_parse_glyf(&res) || !ttf_parse_head(&res)) {
		ttf_free(&res);
		return (TTF){ 0 };
	}
	ttf_print(&res);

	return res;
}

typedef struct {
	u32 vao, vbo, ebo;
	u32 program;
	float x, y;
	size_t count;
	fColor color;
	size_t indicies_count;
} GlyphObject;

GlyphObject create_glyph_obj(FontGlyph* glyph, u32 program) {
	GlyphObject obj = {
		.x = -1,
		.y = 0,
		.program = program,
		.color = rand_color_f(),
	};

	SimpleGlyph* simple = &glyph->as.simple;
	float* vertices = cyx_array_new(float);
	for (size_t i = 0; i < cyx_array_length(simple->x_coords); ++i) {
		cyx_array_append_mult(vertices,
			(float)simple->x_coords[i] / WIDTH / 2,
			(float)simple->y_coords[i] / HEIGHT / 2,
			0.0
		);
	}
	u32* indicies = cyx_array_new(u32);
	u32 prev = 0;
	for (size_t i = 0; i < cyx_array_length(simple->end_points); ++i) {
		for (size_t j = prev; j < simple->end_points[i]; ++j) {
			cyx_array_append_mult(indicies, j, j + 1);
		}
		cyx_array_append_mult(indicies, simple->end_points[i], prev);
		prev = simple->end_points[i] + 1;
	}

	glGenVertexArrays(1, &obj.vao);
	glBindVertexArray(obj.vao);

	glGenBuffers(1, &obj.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, obj.vbo);
	glBufferData(GL_ARRAY_BUFFER, cyx_array_length(vertices) * sizeof(float), vertices, GL_STATIC_DRAW);

	glGenBuffers(1, &obj.ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj.ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, cyx_array_length(indicies) * sizeof(u32), indicies, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), NULL);
	glEnableVertexAttribArray(0);

	obj.count = cyx_array_length(vertices) / 3;
	obj.indicies_count = cyx_array_length(indicies);

	cyx_array_free(vertices);
	cyx_array_free(indicies);
	return obj;
}
void draw_glyph(GlyphObject* obj) {
	glUseProgram(obj->program);
	int uniformColor = glGetUniformLocation(obj->program, "inColor");
	int uniformPos = glGetUniformLocation(obj->program, "screenPos");
	glUniform4f(uniformColor, obj->color.r, obj->color.g, obj->color.b, obj->color.a);
	glUniform2f(uniformPos, obj->x, obj->y);

	glBindVertexArray(obj->vao);
	glDrawElements(GL_LINES, obj->indicies_count, GL_UNSIGNED_INT, 0);

	glBindVertexArray(0);
	glUseProgram(0);
}

int main() {
	// TTF font = ttf_parse("./resources/fonts/UbuntuMonoNerdFontMono-Regular.ttf");
	TTF font = ttf_parse("./resources/fonts/JetBrainsMonoNerdFont-Bold.ttf");

	RGFW_glHints* hints = RGFW_getGlobalHints_OpenGL();
	hints->major = 3;
	hints->minor = 3;
	hints->profile = RGFW_glCore;
	RGFW_setGlobalHints_OpenGL(hints);

	RGFW_window* win = RGFW_createWindow("Terminal", 0, 0, WIDTH, HEIGHT, RGFW_windowCenter | RGFW_windowOpenGL | RGFW_windowNoResize);
	RGFW_window_swapInterval_OpenGL(win, 0);

	GLenum err = glewInit();
	if (err != GLEW_OK) {
		fprintf(stderr, "ERROR:\tUnable to initialize GLEW!\nLOG: %s\n", glewGetErrorString(err));
		return 1;
	}
	printf("LOG:\tOpenGL version: %s\n", glGetString(GL_VERSION));

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	RGFW_setKeyCallback(esc_callback);

	u32 basic_program = compile_program("./resources/shaders/basic.vert", "./resources/shaders/basic.frag");
	GlyphObject c_null = create_glyph_obj(&font.glyphs[0], basic_program);

	srand(time(NULL));
	while (!RGFW_window_shouldClose(win)) {
		updateTimer();

		RGFW_event event;
		while (RGFW_window_checkEvent(win, &event)) {
			if (event.type == RGFW_quit) { break; }
		}
		
		glClearColor(0x18/255.0f, 0x18/255.0f, 0x18/255.0f, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);

		draw_glyph(&c_null);

		RGFW_window_swapBuffers_OpenGL(win);
		glFlush();
	}

	RGFW_window_close(win);
	ttf_free(&font);
	return 0;
}
