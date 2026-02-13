#include <ttf.h>

#include <stdio.h>
#include <limits.h>
#include <stddef.h>

#define CYLIBX_ALLOC
#include <cylibx.h>

typedef struct {
	uint32_t glyph_index;
	uint32_t unicode_code; 
} GlyphMap;
typedef struct {
	uint32_t key;
	uint32_t value;
} TagKV;

#define mmax(a, b) ((a) > (b) ? (a) : (b))
#define mmin(a, b) ((a) < (b) ? (a) : (b))

typedef enum {
	FLAG_ON_CURVE = 0,
	FLAG_IS_SINGLE_BYTE_X = 1,
	FLAG_IS_SINGLE_BYTE_Y = 2,
	FLAG_REPEAT = 3,
	FLAG_INSTRUCTION_X = 4,
	FLAG_INSTRUCTION_Y = 5,
} SimpleGlyphFlag;
static int get_simple_flag(uint8_t flag, SimpleGlyphFlag type) {
	return (flag >> type) & 1;
}

typedef enum {
	FLAG_ARGS_ARE_2_BYTES = 0,
	FLAG_ARGS_ARE_XY_VALUES = 1,
	FLAG_ROUND_XY_TO_GRID = 2,
	FLAG_IS_SINGLE_SCALE_VALUE = 3,

	FLAG_IS_MORE_COMPONENTS_AFTER_THIS = 5,
	FLAG_IS_X_AND_Y_SCALE = 6,
	FLAG_IS_2X2_MATRIX = 7,
	FLAG_HAS_INSTRUCTIONS = 8,

	FLAG_USE_THIS_COMPONENT_METRICS = 9,
	FLAG_COMPONENTS_OVERLAP = 10,
} CompoundGlyphFlag;
static int get_compound_flag(uint16_t flag, CompoundGlyphFlag type) {
	return (flag >> type) & 1;
}

static int16_t to_little_endian_i16(int16_t val) {
	return (((val & 0xff00) >> 8) | ((val & 0xff) << 8));
}
static uint16_t to_little_endian_u16(uint16_t val) {
	return (((val & 0xff00) >> 8) | ((val & 0xff) << 8));
}
static uint32_t to_little_endian_u32(uint32_t val) {
	return (((val & 0xff000000) >> 24) | ((val & 0xff0000) >> 8) | ((val & 0xff00) << 8) | ((val & 0xff) << 24));
}

static uint8_t read_u8(FILE* file) {
	uint8_t res = 0;
	fread(&res, sizeof(res), 1, file);
	return res;
}
static uint16_t read_u16(FILE* file) {
	uint16_t res = 0;
	fread(&res, sizeof(res), 1, file);
	return to_little_endian_u16(res);
}
static int16_t read_i16(FILE* file) {
	int16_t res = 0;
	fread(&res, sizeof(res), 1, file);
	return to_little_endian_i16(res);
}
static uint32_t read_u32(FILE* file) {
	uint32_t res = 0;
	fread(&res, sizeof(res), 1, file);
	return to_little_endian_u32(res);
}
static double read_fixed_point(FILE* file) {
	uint16_t raw = 0;
	fread(&raw, sizeof(uint16_t), 1, file);
	raw = to_little_endian_u16(raw);
	return (int16_t)(raw) / (double)(1 << 14);
}

static void skip_bytes(FILE* file, size_t num_bytes) {
	fseek(file, num_bytes, SEEK_CUR);
}

static TagKV* read_table_locations(EvoAllocator* allocator, FILE* file) {
	TagKV* tables = cyx_hashmap_new(TagKV, allocator, cyx_hash_int32, cyx_eq_int32);

	skip_bytes(file, 4);
	int num_tables = read_u16(file);
	skip_bytes(file, 6);

	for (int i = 0; i < num_tables; ++i) {
		char tag[4] = { 0 };
		fread(&tag, sizeof(char), 4, file);
		skip_bytes(file, 4); // checksum
		uint32_t offset = read_u32(file);
		skip_bytes(file, 4); // length

		cyx_hashmap_add_v(tables, *(uint32_t*)tag, offset);
	}

	return tables;
}
static uint32_t* get_all_glyph_locations(EvoAllocator* allocator, FILE* file, int num_glyphs, int num_bytes, uint32_t loca_location, uint32_t glyf_location) {
	uint32_t* all_glyph_locs = cyx_array_new(uint32_t, allocator, .reserve = num_glyphs);

	for (int i = 0; i < num_glyphs; ++i) {
		fseek(file, loca_location + i * num_bytes, SEEK_SET);
		uint32_t glyph_data_offset = num_bytes == 2 ? ((uint32_t)read_u16(file) << 1) : read_u32(file);
		cyx_array_append(all_glyph_locs, glyph_data_offset + glyf_location);
	}

	return all_glyph_locs;
}
static GlyphMap* get_unicode_to_glyph_index_mappings(EvoAllocator* allocator, FILE* file, uint32_t cmap_location) {
	fseek(file, cmap_location, SEEK_SET);

	skip_bytes(file, 2); // version
	uint32_t num_subtables = read_u16(file);

	uint32_t cmap_subtable_offset = 0;
	int selected_unicode_version_id = -1;

	for (size_t i = 0; i < num_subtables; ++i) {
		int platform_id = read_u16(file);
		int platform_specific_id = read_u16(file);
		uint32_t offset = read_u32(file);

		if (platform_id == 0) {
			if ((platform_specific_id == 0 ||
				platform_specific_id == 1 ||
				platform_specific_id == 3 ||
				platform_specific_id == 4) &&
				platform_specific_id > selected_unicode_version_id) {
				cmap_subtable_offset = offset;
				selected_unicode_version_id = platform_specific_id;
			}
		} else if (platform_id == 3 && selected_unicode_version_id == -1) {
			if (platform_specific_id == 1 || platform_specific_id == 10) {
				cmap_subtable_offset = offset;
			}
		}
	}

	if (!cmap_subtable_offset) {
		fprintf(stderr, "ERROR:\tFont file doesn't contain a supported character map type!\n");
		return NULL;
	}

	fseek(file, cmap_location + cmap_subtable_offset, SEEK_SET);
	int format = read_u16(file);
	if (format != 4 && format != 12) {
		fprintf(stderr, "ERROR:\tFont file contains an unsuported character mapping format [%d]!\n", format);
		return NULL;
	}

	GlyphMap* mappings = cyx_array_new(GlyphMap, allocator);
	char has_read_missing_glyph = 0;
	if (format == 4) {
		skip_bytes(file, 4); // length, language_code
		int segment_count = read_u16(file) >> 1;
		skip_bytes(file, 6);

		EvoTempStack temp = { 0 };
		EvoAllocator temp_allocator = evo_allocator_temp(&temp);
		int* end_codes = cyx_array_new(int, &temp_allocator, .reserve = segment_count);
		for (int i = 0; i < segment_count; ++i) {
			cyx_array_append(end_codes, read_u16(file));
		}
		skip_bytes(file, 2);

		int* start_codes = cyx_array_new(int, &temp_allocator, .reserve = segment_count);
		for (int i = 0; i < segment_count; ++i) {
			cyx_array_append(start_codes, read_u16(file));
		}

		int* id_deltas = cyx_array_new(int, &temp_allocator, .reserve = segment_count);
		for (int i = 0; i < segment_count; ++i) {
			cyx_array_append(id_deltas, read_u16(file));
		}

		int* id_range_offsets = cyx_array_new(int, &temp_allocator, .reserve = 2 * segment_count);
		for (int i = 0; i < segment_count; ++i) {
			cyx_array_append(id_deltas, ftell(file)); // read location
			cyx_array_append(id_deltas, read_u16(file)); // offset
		}

		for (int i = 0; i < segment_count; ++i) {
			int end_code = end_codes[i];
			int curr_code = start_codes[i];

			if (curr_code == 65535) { break; }

			while (curr_code <= end_code) {
				int glyph_index = 0;

				if (!id_range_offsets[2 * i + 1]) {
					glyph_index = (curr_code + id_deltas[i]) % 65536;
				} else {
					uint32_t reader_location_old = ftell(file);
					int range_offset_location = id_range_offsets[2 * i] + id_range_offsets[2 * i + 1];
					int glyph_index_array_location = 2 * (curr_code - start_codes[i]) + range_offset_location;

					fseek(file, glyph_index_array_location, SEEK_SET);
					glyph_index = read_u16(file);

					if (glyph_index) { glyph_index = (glyph_index + id_deltas[i]) % 65536; }

					fseek(file, reader_location_old, SEEK_SET);
				}

				cyx_array_append(mappings, ((GlyphMap){ .glyph_index = glyph_index, .unicode_code = curr_code }));
				if (!curr_code) { has_read_missing_glyph = 1; }
				++curr_code;
			}
		}
	} else if (format == 12) {
		skip_bytes(file, 10);
		uint32_t num_groups = read_u32(file);

		for (size_t i = 0; i < num_groups; ++i) {
			uint32_t start_char_code = read_u32(file);
			uint32_t end_char_code = read_u32(file);
			uint32_t start_glyph_index = read_u32(file);

			uint32_t num_chars = end_char_code - start_char_code + 1;
			for (size_t j = 0; j < num_chars; ++j) {
				cyx_array_append(mappings, ((GlyphMap){
					.glyph_index = start_glyph_index + j,
					.unicode_code = start_char_code + j
				}));
				if (!(start_char_code + j)) { has_read_missing_glyph = 1; }
			}
		}
	}

	if (!has_read_missing_glyph) {
		cyx_array_append(mappings, ((GlyphMap){
			.glyph_index = 0,
			.unicode_code = 0,
		}));
	}

	return mappings;
}
static GlyphData read_glyph(EvoAllocator* temp, EvoAllocator* perm, FILE* file, uint32_t* all_glyph_locations, uint32_t glyph_index);
static GlyphData read_next_component_glyph(EvoAllocator* temp, FILE* file, uint32_t* glyph_locations, uint32_t glyph_loc, uint8_t* repeat) {
	uint16_t flag = read_u16(file);
	uint32_t glyph_index = read_u16(file);

	uint32_t comp_glyph_loc = glyph_locations[glyph_index];

	if (comp_glyph_loc == glyph_loc) {
		*repeat = 0;
		return (GlyphData){
			.points = cyx_array_new(Point, temp),
			.contour_end_indicies = cyx_array_new(int, temp),
		};
	}

	int arg1 = get_compound_flag(flag, FLAG_ARGS_ARE_2_BYTES) ? read_i16(file) : read_u8(file);
	int arg2 = get_compound_flag(flag, FLAG_ARGS_ARE_2_BYTES) ? read_i16(file) : read_u8(file);

	if (!get_compound_flag(flag, FLAG_ARGS_ARE_XY_VALUES)) {
		assert("TODO: Args 1 and 2 are point indicies to be matched, rather than offsets!");
	}

	double offset_x = arg1;
	double offset_y = arg2;

	double i_hat_x = 1;
	double i_hat_y = 0;
	double j_hat_x = 0;
	double j_hat_y = 1;

	if (get_compound_flag(flag, FLAG_IS_SINGLE_SCALE_VALUE)) {
		i_hat_x = read_fixed_point(file);
		j_hat_y = i_hat_x;
	} else if (get_compound_flag(flag, FLAG_IS_X_AND_Y_SCALE)) {
		i_hat_x = read_fixed_point(file);
		j_hat_y = read_fixed_point(file);
	} else if (get_compound_flag(flag, FLAG_IS_2X2_MATRIX)) {
		i_hat_x = read_fixed_point(file);
		i_hat_y = read_fixed_point(file);
		j_hat_x = read_fixed_point(file);
		j_hat_y = read_fixed_point(file);
	}

	uint32_t curr_pos = ftell(file);
	GlyphData simple_glyph = read_glyph(temp, temp, file, glyph_locations, glyph_index);
	fseek(file, curr_pos, SEEK_SET);

	for (size_t i = 0; i < cyx_array_length(simple_glyph.points); ++i) {
		int* x = &simple_glyph.points[i].x;
		int* y = &simple_glyph.points[i].y;

		double x_prime = i_hat_x * (*x) + j_hat_x * (*y) + offset_x;
		double y_prime = i_hat_y * (*x) + j_hat_y * (*y) + offset_y;

		*x = (int)x_prime;
		*y = (int)y_prime;
	}

	*repeat = get_compound_flag(flag, FLAG_IS_MORE_COMPONENTS_AFTER_THIS);
	return simple_glyph;
}
static GlyphData read_simple_glyph(EvoAllocator* temp, EvoAllocator* perm, FILE* file, uint32_t* glyph_locations, uint32_t glyph_index) {
	fseek(file, glyph_locations[glyph_index], SEEK_SET);

	GlyphData glyph = { 0 };
	glyph.glyph_index = glyph_index;

	int contour_count = read_i16(file);
	if (contour_count < 0) {
		fprintf(stderr, "ERROR:\tExpected a simple glyph, but found a compound glyph! This should be unreachable!\n");
		return (GlyphData){ 0 };
	}

	glyph.min_x = read_i16(file);
	glyph.min_y = read_i16(file);
	glyph.max_x = read_i16(file);
	glyph.max_y = read_i16(file);

	int num_points = 0;
	int* contour_end_indices = cyx_array_new(int, perm, .reserve = contour_count);
	for (int i = 0; i < contour_count; ++i) {
		int contour_end_idx = read_u16(file);
		num_points = mmax(num_points, contour_end_idx + 1);
		cyx_array_append(contour_end_indices, contour_end_idx);
	}

	int instr_len = read_i16(file);
	skip_bytes(file, instr_len);

	uint8_t* flags = cyx_array_new(uint8_t, temp, .reserve = num_points);
	Point* points = cyx_array_new(Point, perm, .reserve = num_points);

	for (int i = 0; i < num_points; ++i) {
		uint8_t flag = read_u8(file);
		cyx_array_append(flags, flag);
		if (get_simple_flag(flag, FLAG_REPEAT)) {
			int repeat_count = read_u8(file);
			for (int r = 0; r < repeat_count; ++r) {
				cyx_array_append(flags, flag);
				++i;
			}
		}
	}

	int x = 0;
	for (int i = 0; i < num_points; ++i) {
		uint8_t flag = flags[i];
		
		if (get_simple_flag(flag, FLAG_IS_SINGLE_BYTE_X)) {
			int x_offset = read_u8(file);
			x += get_simple_flag(flag, FLAG_INSTRUCTION_X) ? x_offset : -x_offset;
		} else if (!get_simple_flag(flag, FLAG_INSTRUCTION_X)) {
			x += read_i16(file);
		}
		cyx_array_append(points, ((Point){ .x = x, .on_curve = get_simple_flag(flag, FLAG_ON_CURVE) }));
	}

	int y = 0;
	for (int i = 0; i < num_points; ++i) {
		uint8_t flag = flags[i];
		
		if (get_simple_flag(flag, FLAG_IS_SINGLE_BYTE_Y)) {
			int y_offset = read_u8(file);
			y += get_simple_flag(flag, FLAG_INSTRUCTION_Y) ? y_offset : -y_offset;
		} else if (!get_simple_flag(flag, FLAG_INSTRUCTION_Y)) {
			y += read_i16(file);
		}
		points[i].y = y;
	}

	glyph.points = points;
	glyph.contour_end_indicies = contour_end_indices;
	return glyph;
}
static GlyphData read_compound_glyph(EvoAllocator* temp, EvoAllocator* perm, FILE* file, uint32_t* glyph_locations, uint32_t glyph_index) {
	GlyphData glyph = { 0 };
	glyph.glyph_index = glyph_index;

	uint32_t glyph_loc = glyph_locations[glyph_index];
	fseek(file, glyph_loc, SEEK_SET);
	skip_bytes(file, 2);

	glyph.min_x = read_i16(file);
	glyph.min_y = read_i16(file);
	glyph.max_x = read_i16(file);
	glyph.max_y = read_i16(file);

	Point* all_points = cyx_array_new(Point, perm);
	int* all_contour_end_indicies = cyx_array_new(int, perm);

	uint8_t repeat = 1;
	while (repeat) {
		GlyphData component_glyph = read_next_component_glyph(temp, file, glyph_locations, glyph_loc, &repeat);

		for (size_t i = 0; i < cyx_array_length(component_glyph.contour_end_indicies); ++i) {
			cyx_array_append(all_contour_end_indicies, component_glyph.contour_end_indicies[i] + cyx_array_length(all_points));
		}
		cyx_array_append_mult_n(all_points, cyx_array_length(component_glyph.points), component_glyph.points);
	}

	glyph.points = all_points;
	glyph.contour_end_indicies = all_contour_end_indicies;
	return glyph;
}
static GlyphData read_glyph(EvoAllocator* temp, EvoAllocator* perm, FILE* file, uint32_t* all_glyph_locations, uint32_t glyph_index) {
	uint32_t glyph_loc = all_glyph_locations[glyph_index];

	fseek(file, glyph_loc, SEEK_SET);
	int contour_count = read_i16(file);

	if (contour_count >= 0) {
		return read_simple_glyph(temp, perm, file, all_glyph_locations, glyph_index);
	} else {
		return read_compound_glyph(temp, perm, file, all_glyph_locations, glyph_index);
	}
}
static GlyphData* read_all_glyphs(EvoAllocator* temp, EvoAllocator* perm, FILE* file, uint32_t* all_glyph_locations, GlyphMap* mappings) {
	GlyphData* glyphs = cyx_array_new(GlyphData, perm, .reserve = 128);
	memset(glyphs, 0, 128 * sizeof(GlyphData));
	cyx_array_length(glyphs) = 128;

	for (size_t i = 0; i < cyx_array_length(mappings); ++i) {
		GlyphMap mapping = mappings[i];

		if (mapping.unicode_code < 128) {
			int ascii_val = mapping.unicode_code % 128;

			GlyphData glyph = read_glyph(temp, perm, file, all_glyph_locations, mapping.glyph_index);
			glyph.ascii_val = ascii_val;
			glyph.found = 1;
			glyphs[ascii_val] = glyph;
		}
	}

	return glyphs;
}

TTF ttf_parse(const char* font_file_path) {
	EvoAllocator allocator = evo_allocator_arena_heap(EVO_KB(16));

	TTF ttf = { 0 };

	FILE* file = fopen(font_file_path, "r");
	if (!file) {
		fprintf(stderr, "ERROR:\tUnable to open font file with file path [\"%s\"]\n", font_file_path);
		return ttf;
	}

	TagKV* table_locations = read_table_locations(&allocator, file);

	// cyx_hashmap_foreach(val, table_locations) {
	// 	printf("Tag: %.4s\tValue: %u\n", (char*)&val->key, val->value);
	// }
	uint32_t* glyf_location = cyx_hashmap_get(table_locations, *(uint32_t*)"glyf");
	uint32_t* head_location = cyx_hashmap_get(table_locations, *(uint32_t*)"head");
	uint32_t* loca_location = cyx_hashmap_get(table_locations, *(uint32_t*)"loca");
	uint32_t* cmap_location = cyx_hashmap_get(table_locations, *(uint32_t*)"cmap");
	uint32_t* maxp_location = cyx_hashmap_get(table_locations, *(uint32_t*)"maxp");
	uint32_t* hhea_location = cyx_hashmap_get(table_locations, *(uint32_t*)"hhea");
	uint32_t* hmtx_location = cyx_hashmap_get(table_locations, *(uint32_t*)"hmtx");
	if (!glyf_location || !loca_location || !cmap_location || !head_location || !maxp_location || !hhea_location || !hmtx_location) {
		fprintf(stderr, "ERROR:\tOne of the required tables not provided in the TTF file [\"%s\"]\n", font_file_path);
		return ttf;
	}

	// Parse 'head' table
	fseek(file, *head_location, SEEK_SET);
	skip_bytes(file, 18);
	ttf.units_per_em = read_u16(file);
	skip_bytes(file, 30);
	int num_bytes_per_loc_lookup = (!read_i16(file) ? 2 : 4);

	// Parse 'maxp' table
	fseek(file, *maxp_location, SEEK_SET);
	skip_bytes(file, 4);
	int num_glyphs = read_u16(file);

	uint32_t* all_glyph_locations = get_all_glyph_locations(&allocator, file, num_glyphs, num_bytes_per_loc_lookup, *loca_location, *glyf_location);

	GlyphMap* mappings = get_unicode_to_glyph_index_mappings(&allocator, file, *cmap_location);
	if (!mappings) {
		return ttf;
	}
	ttf.arena = evo_arena_new(EVO_KB(1));
	ttf.alloc = evo_allocator_arena(&ttf.arena);
	GlyphData* glyphs = read_all_glyphs(&allocator, &ttf.alloc, file, all_glyph_locations, mappings);

	// Parse 'hhea' table
	fseek(file, *hhea_location, SEEK_SET);
	skip_bytes(file, 34);
	int num_advance_width_metrics = read_i16(file);

	// Parse 'hmtx' table
	fseek(file, *hmtx_location, SEEK_SET);
	int last_advance_width = 0;

	int* layout_data = cyx_array_new(int, &allocator, .reserve = 2 * num_glyphs);
	for (int i = 0; i < num_advance_width_metrics; ++i) {
		int advance_width = read_u16(file);
		int left_side_bearing = read_i16(file);

		last_advance_width = advance_width;
		cyx_array_append_mult(layout_data, advance_width, left_side_bearing);
	}

	int num_rem = num_glyphs - num_advance_width_metrics;
	for (int i = 0; i < num_rem; ++i) {
		int left_side_bearing = read_i16(file);
		cyx_array_append_mult(layout_data, last_advance_width, left_side_bearing);
	}

	for (size_t i = 0; i < cyx_array_length(glyphs); ++i) {
		if (glyphs[i].glyph_index >= 65535) { continue; }

		glyphs[i].advance_width = layout_data[2 * glyphs[i].glyph_index];
		glyphs[i].left_side_bearing = layout_data[2 * glyphs[i].glyph_index + 1];
	}

	memcpy(ttf.glyphs, glyphs, 128 * sizeof(GlyphData));

	fclose(file);
	evo_allocator_free(&allocator);
	return ttf;
}
GlyphData* ttf_get(TTF* ttf, char c) {
	GlyphData* data = &ttf->glyphs[(int)c];
	if (c < 0 || !data->found) {
		return ttf->glyphs;
	}
	return data;
}
void ttf_free(TTF* ttf) {
	evo_arena_destroy(&ttf->arena);
}
