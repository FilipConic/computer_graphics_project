#include "obj_parse.h"

#include <stdlib.h>
#include <stdio.h>

#define CYLIBX_ALLOC
#include "cylibx.h"
#include "evoco.h"

#define STRING_SLICE_CONTAIN 8
typedef union {
	size_t len;
	struct {
		size_t len;
		char buffer[STRING_SLICE_CONTAIN];
	} in;
	struct {
		size_t len;
		char* ptr;
	} out;
} StringSlice;

static StringSlice slice_from_lit_n(size_t n, char* str) {
	if (n <= STRING_SLICE_CONTAIN) {
		StringSlice slice = { .len = n };
		memcpy(slice.in.buffer, str, n * sizeof(char));
		if (n + 1 < STRING_SLICE_CONTAIN) {
			slice.in.buffer[n + 1] = '\0';
		}
		return slice;
	} else {
		return (StringSlice){ .out.ptr = str, .out.len = n };
	}
}
#define slice_from_lit(str) slice_from_lit_n(strlen(str), str)
#define slice_from_str(str) (__CYX_STRING_TYPECHECK(str), slice_from_lit_n(cyx_str_length(str), str))
#define slice_buffer_by_type(slice) ((char*)((slice)->len <= STRING_SLICE_CONTAIN ? (slice)->in.buffer : (slice)->out.ptr))
#define SLICE_UNPACK(slice) (int)(slice)->len, slice_buffer_by_type(slice)

static StringSlice str_get_slice(char* str, size_t start, char cmp_char) {
	size_t counter = 0;
	for (size_t i = start; i < cyx_str_length(str); ++i, ++counter) {
		if (str[i] == cmp_char || str[i] == '\0') { break; }
	}
	return slice_from_lit_n(counter, str + start);
}
#define str_split_by_char(slice, str, c) size_t __CYX_UNIQUE_VAL__(counter) = 0; \
	for (StringSlice slice = str_get_slice(str, __CYX_UNIQUE_VAL__(counter), c); \
			__CYX_UNIQUE_VAL__(counter) < cyx_str_length(str); \
			__CYX_UNIQUE_VAL__(counter) += (slice).len + 1, \
			slice = str_get_slice(str, __CYX_UNIQUE_VAL__(counter), c))

static int slice_eq(const void* a, const void* b) {
	const StringSlice* s1 = a;
	const StringSlice* s2 = b;
	if (s1->len != s2->len) { return 0; }
	return strncmp(slice_buffer_by_type(s1), slice_buffer_by_type(s2), s1->len) == 0;
}

typedef struct {
	char* file;
	size_t curr;
} ObjLexer;

static double slice_read_double(StringSlice slice) {
	char* str = slice_buffer_by_type(&slice);

	double ret = strtold(str, NULL);
	return ret;
}
static int64_t slice_read_long(StringSlice slice) {
	char is_neg = 0;
	char* str = slice_buffer_by_type(&slice);

	if (*str == '-') {
		is_neg = 1;
	}

	char* endptr;
	int64_t ret = strtoll(str, &endptr, 10);

	int extra = slice.len;
	while (str + extra != endptr) {
		ret /= 10;
		++extra;
	}
	return !is_neg ? ret : -ret;
}

typedef struct {
	StringSlice slices[48];
	int len;
} StringSliceArray;
static StringSliceArray slice_split_by_char(StringSlice slice, char c) {
	StringSliceArray ret = { 0 };
	size_t counter = 0;
	char* str = slice_buffer_by_type(&slice);
	char* curr = str;
	for (size_t i = 0; i < slice.len; ++i) {
		if (ret.len == 48) {
			fprintf(stderr, "ERROR:\tToo many slices to be separated [%d]!\n", c);
			return ret;
		} else if (str[i] == c) {
			if (counter) {
				ret.slices[ret.len++] = slice_from_lit_n(counter, curr);
			}
			curr += counter + 1;
			counter = 0;
		} else {
			counter++;
		}
	}
	if (ret.len == 48) {
		fprintf(stderr, "ERROR:\tToo many slices to be separated [%d]!\n", c);
		return ret;
	} else {
		if (counter) {
			ret.slices[ret.len++] = slice_from_lit_n(counter, curr);
		}
	}
	return ret;
}
static int slice_count_char(StringSlice slice, char c) {
	int ret = 0;
	char* str = slice_buffer_by_type(&slice);
	for (size_t i = 0; i < slice.len; ++i) {
		if (str[i] == c) { ++ret; }
	}
	return ret;
}

enum LineType {
	OBJ_NONE,
	OBJ_VERTEX,
	OBJ_NORMAL,
	OBJ_FACE,
	OBJ_COUNT,
};
static StringSlice possible_line[] = {
	[OBJ_NONE] = { .in.buffer = "", .in.len = 0 },
	[OBJ_VERTEX] = { .in.buffer = "v", .in.len = 1 },
	[OBJ_NORMAL] = { .in.buffer = "vn", .in.len = 2 },
	[OBJ_FACE] = { .in.buffer = "f", .in.len = 1 },
};

int obj_parse(const char* file_path, uint32_t** indices, float** vertices) {
	EvoArena arena = evo_arena_new(EVO_KB(16));
	EvoAllocator alloc = evo_allocator_arena(&arena);

	ObjLexer lex = { 
		.file = cyx_str_from_file(&alloc, file_path),
		.curr = 0,
	};

	float* normals = cyx_array_new(float, &alloc);

	int row_count = 0;
	str_split_by_char(line, lex.file, '\n') {
		row_count++;
		StringSliceArray arr = slice_split_by_char(line, ' ');

		enum LineType curr_process = OBJ_NONE;
		for (size_t i = 0; i < OBJ_COUNT; ++i) {
			if (slice_eq(&possible_line[i], &arr.slices[0])) {
				curr_process = i;
				break;
			}
		}
		if (curr_process == OBJ_NONE) { continue; }

		if (curr_process == OBJ_VERTEX) {
			cyx_array_append_mult(*vertices,
				slice_read_double(arr.slices[1]),
				slice_read_double(arr.slices[2]),
				slice_read_double(arr.slices[3]),
				0, 0, 0
			);
		} else if (curr_process == OBJ_NORMAL) {
			cyx_array_append_mult(normals,
				slice_read_double(arr.slices[1]),
				slice_read_double(arr.slices[2]),
				slice_read_double(arr.slices[3])
			);
		} else if (curr_process == OBJ_FACE) {
			assert(arr.len >= 4);

			/////////////////
			int count = slice_count_char(arr.slices[1], '/');
			assert(count == 2);
			StringSliceArray nums = slice_split_by_char(arr.slices[1], '/');

			int first_vert = slice_read_long(nums.slices[0]) - 1;
			int first_norm = slice_read_long(nums.slices[nums.len - 1]) - 1;

			(*vertices)[6 * first_vert + 3] = normals[first_norm + 0];
			(*vertices)[6 * first_vert + 4] = normals[first_norm + 1];
			(*vertices)[6 * first_vert + 5] = normals[first_norm + 2];

			/////////////////////
			count = slice_count_char(arr.slices[2], '/');
			assert(count == 2);
			nums = slice_split_by_char(arr.slices[2], '/');

			int saved_vert = slice_read_long(nums.slices[0]) - 1;
			int saved_norm = slice_read_long(nums.slices[nums.len - 1]) - 1;

			(*vertices)[6 * saved_vert + 3] = normals[saved_norm + 0];
			(*vertices)[6 * saved_vert + 4] = normals[saved_norm + 1];
			(*vertices)[6 * saved_vert + 5] = normals[saved_norm + 2];

			///////////////////////////////
			for (size_t i = 3; (int)i < arr.len; ++i) {
				int count = slice_count_char(arr.slices[i], '/');
				assert(count == 2);
				StringSliceArray nums = slice_split_by_char(arr.slices[i], '/');
				int vert = slice_read_long(nums.slices[0]) - 1;
				int norm = slice_read_long(nums.slices[nums.len - 1]) - 1;

				cyx_array_append_mult(*indices, first_vert, saved_vert, vert);
				(*vertices)[6 * vert + 3] = normals[norm + 0];
				(*vertices)[6 * vert + 4] = normals[norm + 1];
				(*vertices)[6 * vert + 5] = normals[norm + 2];
				saved_vert = vert;
			}
		}
	}

	evo_alloc_destroy(&alloc);
	return 1;
}
