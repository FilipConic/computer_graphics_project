#ifndef __CUBE_MARCHING__
#define __CUBE_MARCHING__

#include <stdlib.h>
#include <stdint.h>

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

StringSlice slice_from_lit_n(size_t n, char* str);
#define slice_from_lit(str) slice_from_lit_n(strlen(str), str)
#define slice_from_str(str) (__CYX_STRING_TYPECHECK(str), slice_from_lit_n(cyx_str_length(str), str))
#define slice_buffer_by_type(slice) ((char*)((slice)->len <= STRING_SLICE_CONTAIN ? (slice)->in.buffer : (slice)->out.ptr))
#define SLICE_UNPACK(slice) (int)(slice)->len, slice_buffer_by_type(slice)

int slice_eq(const void* a, const void* b);
size_t slice_hash(const void* v);

typedef struct {
	StringSlice key;
	double value;
} VariableKV;

typedef struct {
	uint32_t res;
	double left, right;
	double bottom, top;
	double near, far;
} CubeMarchDefintions;

int cube_march(uint32_t** indicies, float** triangles, char* equation, VariableKV* vars, CubeMarchDefintions defs, char** err_msg);

#endif // __CUBE_MARCHING__
