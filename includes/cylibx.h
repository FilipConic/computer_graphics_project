#ifndef __CYLIBX_H__
#define __CYLIBX_H__

// #define CYLIBX_IMPLEMENTATION
// #define CYLIBX_ALLOC

#if defined(CYLIBX_ALLOC) && defined(CYLIBX_IMPLEMENTATION)
#define EVOCO_IMPLEMENTATION
#endif // CYLIBX_ALLOC && CYLIBX_IMPLEMENTATION

#ifdef CYLIBX_ALLOC
#include <evoco.h>
#endif // CYLIBX_ALLOC

#if defined(CYLIBX_ALLOC) && defined(CYLIBX_IMPLEMENTATION)
static EvoAllocator EVO_LIBC_ALLOCATOR = {
	.ctx = NULL,

	.malloc_f = evo_libc_malloc,
	.calloc_f = evo_libc_calloc,
	.realloc_f = evo_libc_realloc,
	.reallocarray_f = evo_libc_reallocarray,
	.destroy_f = NULL,
	.free_f = evo_libc_free,
	.reset_f = NULL,

	.heap_flag = 0,
};
#endif // CYLIBX_ALLOC && CYLIBX_IMPLEMENTATION

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef CYLIBX_IMPLEMENTATION
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#endif // CYLIBX_IMPLEMENTATION

#define __CYX_CLOSE_FOLD 1

/*
 * Temporary Buffer
 */

#if __CYX_CLOSE_FOLD

#ifdef CYLIBX_IMPLEMENTATION

#ifndef CYLIBX_ALLOC
#define __CYX_TEMP_STACK_SIZE 16
struct __CyxTempStack {
	void* current;
	size_t filled;
	size_t mark;
	uint8_t buffer[1024 * __CYX_TEMP_STACK_SIZE];
};
static struct __CyxTempStack __cyx_temp_stack = { 0 };
void __cyx_temp_reset(struct __CyxTempStack* temp) {
	temp->current = temp->buffer;
	temp->filled = 0;
}
void* __cyx_temp_malloc(struct __CyxTempStack* temp, size_t size) {
	if (!temp->current) { temp->current = temp->buffer; }

	size_t capacity = sizeof(temp->buffer) / sizeof(*temp->buffer);
	size_t aligned = ((size - 1) / sizeof(uintptr_t) + 1) * sizeof(uintptr_t);

	if (temp->filled + aligned > capacity) {
		if (temp->mark) {
			__cyx_temp_reset(temp);
			if (temp->filled + aligned > capacity) {
				fprintf(stderr, "ERROR:\tYou have set a temporary mark which doesn't allow an allocation to reset the whole temporary stack, but you are trying to allocate an object too big to fit!\n");
				return NULL;
			}
		} else {
			__cyx_temp_reset(temp);
		}
	}

	uint8_t* ret = (uint8_t*)temp->current;
	temp->current = (char*)temp->current + aligned;
	temp->filled += aligned;
	return ret;
}
void* __cyx_temp_calloc(void* v, size_t n, size_t size) {
	void* ret = __cyx_temp_malloc(v, n * size);
	memset(ret, 0, n * size);
	return ret;
}
void __cyx_temp_set_mark(struct __CyxTempStack* temp) {
	temp->mark = temp->filled;
}
void __cyx_temp_reset_mark(struct __CyxTempStack* temp) {
	temp->filled = temp->mark;
	temp->current = (char*)temp->buffer + temp->filled;
	temp->mark = 0;
}
#else
static EvoTempStack __cyx_temp_stack = { 0 };
#define __cyx_temp_malloc(temp, size) evo_temp_malloc(temp, size)
#define __cyx_temp_calloc(temp, size) evo_temp_calloc(temp, size)
#define __cyx_temp_set_mark(temp) evo_temp_set_mark(temp)
#define __cyx_temp_reset_mark(temp) evo_temp_reset_mark(temp)
#define __cyx_temp_reset(temp) evo_temp_reset(temp)
#endif // CYLIBX_ALLOC

// TODO: add again support for temporary saving of deleted values

#endif // CYLIBX_IMPLEMENTATION

#endif // __CYX_CLOSE_FOLD

/*
 * Util macros
 */

#if __CYX_CLOSE_FOLD

enum __CyxDataType {
	__CYX_TYPE_ERROR,
	__CYX_TYPE_ARRAY,
	__CYX_TYPE_HASHSET,
	__CYX_TYPE_HASHMAP,
	__CYX_TYPE_BINHEAP,
	__CYX_TYPE_RINGBUF,
	__CYX_TYPE_STRING,
	__CYX_TYPE_BITMAP
};

#define __CYX_GET_TYPE(d) ((enum __CyxDataType*)(d) - 2) 
#define __CYX_TYPE_SIZE (sizeof(enum __CyxDataType) * 2)
#define __CYX_CONCAT_VALS_BASE__(a, b) a##b
#define __CYX_CONCAT_VALS__(a, b) __CYX_CONCAT_VALS_BASE__(a, b)
#define __CYX_UNIQUE_VAL__(a) __CYX_CONCAT_VALS__(a, __LINE__)

#ifdef CYLIBX_IMPLEMENTATION

#define __CYX_DATA_GET_AT(header, data_ptr, pos) ((char*)(data_ptr) + (pos) * (header)->size)
#define __CYX_PTR_CMP(header, l, r) (!(header)->is_ptr ? (header)->cmp_fn(l, r) : (header)->cmp_fn(*(void**)(l), *(void**)(r)))
#define __CYX_PTR_CMP_POS(header, data_ptr, l_pos, r_pos) (!(header)->is_ptr ? \
	(header)->cmp_fn(__CYX_DATA_GET_AT(header, data_ptr, l_pos), __CYX_DATA_GET_AT(header, data_ptr, r_pos)) : \
	(header)->cmp_fn(*(void**)(__CYX_DATA_GET_AT(header, data_ptr, l_pos)), *(void**)(__CYX_DATA_GET_AT(header, data_ptr, r_pos))) \
)
#define __CYX_SWAP(a, b, size) do { \
	__cyx_temp_set_mark(&__cyx_temp_stack); { \
		void* temp = __cyx_temp_malloc(&__cyx_temp_stack, size); \
		assert(temp && "ERROR: Can not allocate to the temporary buffer anymore!"); \
		memcpy(temp, a, size); \
		memcpy(a, b, size); \
		memcpy(b, temp, size); \
	} __cyx_temp_reset_mark(&__cyx_temp_stack); \
} while(0);

#endif // CYLIBX_IMPLEMENTATION

#endif // __CYX_CLOSE_FOLD

/*
 * String
 */

#if __CYX_CLOSE_FOLD

typedef struct {
	size_t len;
	size_t cap;
#ifdef CYLIBX_ALLOC
	EvoAllocator* alloc;
#endif // CYLIBX_ALLOC
} __CyxStringHeader;

#ifndef CYX_STRING_BASE_SIZE
#define CYX_STRING_BASE_SIZE 16
#endif // CYX_STRING_BASE_SIZE

#define __CYX_STRING_HEADER_SIZE (sizeof(__CyxStringHeader))
#define __CYX_STRING_GET_HEADER(str) ((__CyxStringHeader*)__CYX_GET_TYPE(str) - 1)
#define __CYX_STRING_TYPECHECK(str) assert(*__CYX_GET_TYPE(str) == __CYX_TYPE_STRING && \
		"ERROR: Type missmatch! Type of the provided pointer is not string but was expected to be of type string!")

#ifndef CYLIBX_ALLOC
char* cyx_str_new();
char* cyx_str_from_lit_n(const char* c_str, size_t n);
char* cyx_str_from_file(const char* file_path);
#else
char* cyx_str_new(EvoAllocator* allocator);
char* cyx_str_from_lit_n(EvoAllocator* allocator, const char* c_str, size_t n);
char* cyx_str_from_file(EvoAllocator* allocator, const char* file_path);
#endif // CYLIBX_ALLOC
char* cyx_str_append_char(char** str, char c);
char* cyx_str_insert_char(char** str, char c, int pos);
char* cyx_str_append_lit_n(char** str, const char* c_str, size_t n);
char* cyx_str_append_file(char** str, const char* file_path);
char* cyx_str_append_uint(char** str, uint64_t n);
void cyx_str_remove(char** str, int pos);
void cyx_str_replace(char* str, char to_replace, char c);
size_t cyx_str_count_char(char* str, char c);
uint64_t cyx_str_parse_uint(const char* str, size_t from, int* to);
int64_t cyx_str_parse_int(const char* str, size_t from, int* to);
void cyx_str_free(void* str);
void cyx_str_clear(char* str);
void cyx_str_print(const void* str);
int cyx_str_eq(const void* val1, const void* val2);
int cyx_str_cmp(const void* val1, const void* val2);

#define cyx_str_length(str) (__CYX_STRING_GET_HEADER(str)->len)
#define CYX_STR_FMT "%.*s"
#define CYX_STR_UNPACK(str) (int)cyx_str_length(str), str

#ifndef CYLIBX_ALLOC // str_from_lit
#define cyx_str_from_lit(c_str) (cyx_str_from_lit_n(c_str, strlen(c_str)))
#else 
#define cyx_str_from_lit(allocator, c_str) (cyx_str_from_lit_n(allocator, c_str, strlen(c_str)))
#endif // CYLIBX_ALLOC
#ifndef CYLIBX_ALLOC // str_copy()
#define cyx_str_copy(str) (__CYX_STRING_TYPECHECK(str), cyx_str_from_lit_n(str, cyx_str_length(other)))
#else 
#define cyx_str_copy(str) (__CYX_STRING_TYPECHECK(str), cyx_str_from_lit_n(__CYX_STRING_GET_HEADER(str)->alloc, str, cyx_str_length(str)))
#define cyx_str_copy_a(allocator, str) (__CYX_STRING_TYPECHECK(str), cyx_str_from_lit_n((allocator), str, cyx_str_length(str)))
#endif // CYLIBX_ALLOC
#define cyx_str_append_lit(str, c_str) (cyx_str_append_lit_n(str, c_str, strlen(c_str)))
#define cyx_str_append_str(self, other) (__CYX_STRING_TYPECHECK(other), (cyx_str_append_lit_n(self, other, cyx_str_length(other))))
#define cyx_str_pop(str) (cyx_str_remove(str, -1))

#ifdef CYLIBX_STRIP_PREFIX

#define STR_FMT CYX_STR_FMT
#define STR_UNPACK(str) CYX_STR_UNPACK(str)

#define str_length(str) cyx_str_length(str)
#ifndef CYLIBX_ALLOC
#define str_from_lit(c_str) cyx_str_from_lit(c_str)
#else
#define str_from_lit(allocator, c_str) cyx_str_from_lit(allocator, c_str)
#endif // CYLIBX_ALLOC
#define str_copy(str) cyx_str_copy(str)
#define str_append_lit(str, c_str) cyx_str_append_lit(str, c_str)
#define str_append_str(self, other) cyx_str_append_str(self, other)
#define str_pop(str) cyx_str_remove(str, -1)

#define str_new cyx_str_new
#define str_from_lit_n cyx_str_from_lit_n
#define str_from_file cyx_str_from_file
#define str_append_char cyx_str_append_char
#define str_insert_char cyx_str_insert_char
#define str_append_lit_n cyx_str_append_lit_n
#define str_append_file cyx_str_append_file
#define str_append_uint cyx_str_append_uint
#define str_count_char cyx_str_count_char
#define str_parse_uint cyx_str_parse_uint
#define str_parse_int cyx_str_parse_int
#define str_replace cyx_str_replace
#define str_remove cyx_str_remove
#define str_free cyx_str_free
#define str_clear cyx_str_clear
#define str_print cyx_str_print
#define str_eq cyx_str_eq
#define str_cmp cyx_str_cmp

#endif // CYLIBX_STRIP_PREFIX

#ifdef CYLIBX_IMPLEMENTATION

#define __CYX_BUFFER_SIZE 256

#ifndef CYLIBX_ALLOC // str_new, str_expand, str_from_lit_n, str_from_file
char* cyx_str_new() {
	__CyxStringHeader* head = malloc(__CYX_STRING_HEADER_SIZE + __CYX_TYPE_SIZE + CYX_STRING_BASE_SIZE * sizeof(char));
	if (!head) { return NULL; }

	memset(head, 0, __CYX_STRING_HEADER_SIZE + __CYX_TYPE_SIZE + CYX_STRING_BASE_SIZE * sizeof(char));
	head->cap = CYX_STRING_BASE_SIZE;

	enum __CyxDataType* type = (enum __CyxDataType*)(head + 1);
	*type = __CYX_TYPE_STRING;
	char* str = (char*)(type + 2);

	return str;
}
void __cyx_str_expand(char** str_ptr, size_t n) {
	char* str = *str_ptr;
	__CyxStringHeader* head = __CYX_STRING_GET_HEADER(str);

	size_t new_cap = head->cap;
	while (new_cap < head->len + n) { new_cap <<= 1; }
	if (new_cap == head->cap) { return; }

	__CyxStringHeader* new_head = malloc(__CYX_STRING_HEADER_SIZE + __CYX_TYPE_SIZE + new_cap * sizeof(char));
	memcpy(new_head, head, __CYX_STRING_HEADER_SIZE + __CYX_TYPE_SIZE + head->cap * sizeof(char) );
	new_head->cap = new_cap;
	enum __CyxDataType* type = (void*)(new_head + 1);
	char* new_str = (void*)(type + 2);
	free(head);
	*str_ptr = new_str;
}
char* cyx_str_from_lit_n(const char* c_str, size_t n) {
	__CyxStringHeader* head = malloc(__CYX_STRING_HEADER_SIZE + __CYX_TYPE_SIZE + n * sizeof(char));
	if (!head) { return NULL; }

	enum __CyxDataType* type = (void*)(head + 1);
	*type = __CYX_TYPE_STRING;
	memcpy(type + 2, c_str, n * sizeof(char));
	head->cap = n;
	head->len = n;
	
	return (char*)(type + 2);
}
char* cyx_str_from_file(const char* file_path) {
	char* res = cyx_str_new();
	return cyx_str_append_file(&res, file_path);
}
#else
char* cyx_str_new(EvoAllocator* allocator) {
	if (!allocator) { allocator = &EVO_LIBC_ALLOCATOR; }
	__CyxStringHeader* head = evo_alloc_malloc(allocator, __CYX_STRING_HEADER_SIZE + __CYX_TYPE_SIZE + CYX_STRING_BASE_SIZE * sizeof(char));
	if (!head) { return NULL; }

	memset(head, 0, __CYX_STRING_HEADER_SIZE + __CYX_TYPE_SIZE + CYX_STRING_BASE_SIZE * sizeof(char));
	head->cap = CYX_STRING_BASE_SIZE;
	head->alloc = allocator;

	enum __CyxDataType* type = (enum __CyxDataType*)(head + 1);
	*type = __CYX_TYPE_STRING;
	char* str = (char*)(type + 2);

	return str;
}
void __cyx_str_expand(char** str_ptr, size_t n) {
	char* str = *str_ptr;
	__CyxStringHeader* head = __CYX_STRING_GET_HEADER(str);

	size_t new_cap = head->cap;
	while (new_cap < head->len + n) { new_cap <<= 1; }
	if (new_cap == head->cap) { return; }

	__CyxStringHeader* new_head = evo_alloc_malloc(head->alloc, __CYX_STRING_HEADER_SIZE + __CYX_TYPE_SIZE + new_cap * sizeof(char));
	memcpy(new_head, head, __CYX_STRING_HEADER_SIZE + __CYX_TYPE_SIZE + head->cap * sizeof(char) );
	new_head->cap = new_cap;
	enum __CyxDataType* type = (void*)(new_head + 1);
	char* new_str = (void*)(type + 2);
	if (new_head->alloc->free_f) { evo_alloc_free(new_head->alloc, head); }
	*str_ptr = new_str;
}
char* cyx_str_from_lit_n(EvoAllocator* allocator, const char* c_str, size_t n) {
	if (!allocator) { allocator = &EVO_LIBC_ALLOCATOR; }
	__CyxStringHeader* head = evo_alloc_malloc(allocator, __CYX_STRING_HEADER_SIZE + __CYX_TYPE_SIZE + n * sizeof(char));
	if (!head) { return NULL; }
	head->alloc = allocator;
	head->cap = n;
	head->len = n;

	enum __CyxDataType* type = (void*)(head + 1);
	*type = __CYX_TYPE_STRING;
	memcpy(type + 2, c_str, n * sizeof(char));
	
	return (char*)(type + 2);
}
char* cyx_str_from_file(EvoAllocator* allocator, const char* file_path) {
	char* res = cyx_str_new(allocator);
	return cyx_str_append_file(&res, file_path);
}
#endif // CYLIBX_ALLOC
char* cyx_str_append_char(char** str_ptr, char c) {
	__CYX_STRING_TYPECHECK(*str_ptr);

	__cyx_str_expand(str_ptr, 1);
	char* str = *str_ptr;
	__CyxStringHeader* head = __CYX_STRING_GET_HEADER(str);
	str[head->len++] = c;

	return *str_ptr;
}
char* cyx_str_insert_char(char** str_ptr, char c, int pos) {
	__CYX_STRING_TYPECHECK(*str_ptr);

	__cyx_str_expand(str_ptr, 1);
	char* str = *str_ptr;
	__CyxStringHeader* head = __CYX_STRING_GET_HEADER(str);

	if (pos < 0) { pos += head->len; }
	assert(pos < (int)head->len);

	memmove(str + pos + 1, str + pos, head->len - pos);
	str[pos] = c;
	++head->len;

	return *str_ptr;
}
char* cyx_str_append_lit_n(char** str_ptr, const char* c_str, size_t n) {
	__CYX_STRING_TYPECHECK(*str_ptr);

	__cyx_str_expand(str_ptr, n);
	char* str = *str_ptr;
	for (size_t i = 0; i < n; ++i) {
		str[cyx_str_length(str)++] = c_str[i];
	}
	return *str_ptr;
}
char* cyx_str_append_file(char** str_ptr, const char* file_path) {
	__CYX_STRING_TYPECHECK(*str_ptr);

	FILE* file = fopen(file_path, "r");
	if (!file) { return *str_ptr; }

	fseek(file, 0, SEEK_END);
	size_t size = ftell(file);
	fseek(file, 0, SEEK_SET);
	__cyx_str_expand(str_ptr, size);
	char* str = *str_ptr;

	char buffer[__CYX_BUFFER_SIZE] = { 0 };
	int read_count = 0;
	while ((read_count = fread(buffer, sizeof(char), __CYX_BUFFER_SIZE, file))) {
		memcpy(str + cyx_str_length(str), buffer, read_count * sizeof(char));
		cyx_str_length(str) += read_count;
	}

	return *str_ptr;
}
char* cyx_str_append_uint(char** str_ptr, uint64_t n) {
	__CYX_STRING_TYPECHECK(*str_ptr);

	int n_len = n ? (int)log10(n) + 1 : 1;
	char buffer[24] = { 0 };
	for (int i = 0; i < n_len; ++i) {
		char c = (n % 10) + '0';
		n /= 10;
		buffer[n_len - i - 1] = c;
	}
	cyx_str_append_lit_n(str_ptr, buffer, n_len);

	return *str_ptr;
}
void cyx_str_remove(char** str_ptr, int pos) {
	char* str = *str_ptr;
	__CYX_STRING_TYPECHECK(str);

	__CyxStringHeader* head = __CYX_STRING_GET_HEADER(str);
	if (pos < 0) { pos += head->len; }
	assert(pos < (int)head->len);
	if (pos + 1 != (int)head->len) {
		memmove(str + pos, str + pos + 1, head->len - pos);
	}

	--head->len;
}
void cyx_str_replace(char* str, char to_replace, char c) {
	__CYX_STRING_TYPECHECK(str);
	if (c == to_replace) { return; }
	
	__CyxStringHeader* head = __CYX_STRING_GET_HEADER(str);
	for (size_t i = 0; i < head->len; ++i) {
		if (str[i] == to_replace) { str[i] = c; }
	}
}
void cyx_str_free(void* str) {
	__CYX_STRING_TYPECHECK(str);
	__CyxStringHeader* head = __CYX_STRING_GET_HEADER((void*)str);
#ifndef CYLIBX_ALLOC
	free(head);
#else
	if (head->alloc->free_f) {
		free(head);
	} else {
		fprintf(stderr, "ERROR:\tThe allocator you've provided to the string doesn't free single elements as you expected!\n");
	}
#endif // CYLIBX_ALLOC
}
void cyx_str_clear(char* str) {
	__CYX_STRING_TYPECHECK(str);
	cyx_str_length(str) = 0;
}
void cyx_str_print(const void* str) {
	__CYX_STRING_TYPECHECK(str);
	printf("\""CYX_STR_FMT"\"", CYX_STR_UNPACK((char*)str));
}
int cyx_str_eq(const void* val1, const void* val2) {
	__CYX_STRING_TYPECHECK(val1);
	__CYX_STRING_TYPECHECK(val2);

	char* str1 = (char*)val1;
	char* str2 = (char*)val2;
	if (cyx_str_length(str1) != cyx_str_length(str2)) { return 0; }
	for (size_t i = 0; i < cyx_str_length(str1); ++i) {
		if (str1[i] != str2[i]) { return 0; }
	}
	return 1;
}
int cyx_str_cmp(const void* val1, const void* val2) {
	__CYX_STRING_TYPECHECK(val1);
	__CYX_STRING_TYPECHECK(val2);

	char* str1 = (char*)val1;
	char* str2 = (char*)val2;
	int ret = strncmp(str1, str2, cyx_str_length(str1) < cyx_str_length(str2) ? cyx_str_length(str1) : cyx_str_length(str2)); 
	if (ret < 0) return -1;
	else if (ret > 0) return 1;
	else return 0;
}

size_t cyx_str_count_char(char* str, char c) {
	size_t sum = 0;
	for (size_t i = 0; i < cyx_str_length(str); ++i) {
		if (str[i] == c) { ++sum; }
	}
	return sum;
}
uint64_t cyx_str_parse_uint(const char* str, size_t from, int* to) {
	if (from >= cyx_str_length(str)) { return 0; }
	size_t till = (!to || *to <= (int)from || *to >= (int)cyx_str_length(str) ? cyx_str_length(str) : (size_t)*to);
	size_t i = from;
	char buffer[21] = { 0 };
	short count = 0;
	for (; i < till; ++i) {
		if ('0' <= str[i] && str[i] <= '9') {
			buffer[count++] = str[i];
		} else { break; }
		if (count == 21) { break; }
	}
	if (to) { *to = i; }
	buffer[count] = '\0';
	return atol(buffer);
}
int64_t cyx_str_parse_int(const char* str, size_t from, int* to) {
	if (from >= cyx_str_length(str)) { return 0; }
	size_t till = (!to || *to <= (int)from || *to >= (int)cyx_str_length(str) ? cyx_str_length(str) : (size_t)*to);
	size_t i = from;
	char is_neg = 0;
	if (str[i] == '-') {
		is_neg = 1;
		++i;
	}
	char buffer[21] = { 0 };
	short count = 0;
	for (; i < till; ++i) {
		if ('0' <= str[i] && str[i] <= '9') {
			buffer[count++] = str[i];
		} else { break; }

		if (count == 21) { break; }
	}
	if (to) { *to = i; }
	if (!count) { return 0; }
	buffer[count] = '\0';
	return !is_neg ? atol(buffer) : -atol(buffer);
}

#endif // CYLIBX_IMPLEMENTATION

#endif // __CYX_CLOSE_FOLD

/*
 * Array
 */

#if __CYX_CLOSE_FOLD

typedef struct {
	size_t size;
	size_t len;
	size_t cap;

	void (*defer_fn)(void*);
	int (*cmp_fn)(const void*, const void*);
	void (*print_fn)(const void*);

#ifdef CYLIBX_ALLOC
	EvoAllocator* alloc;
#endif // CYLIBX_ALLOC

	char is_ptr;
} __CyxArrayHeader;

struct __CyxArrayParams {
#ifdef CYLIBX_ALLOC
	EvoAllocator* __allocator;
#endif // CYLIBX_ALLOC
	size_t __size;
	size_t reserve;
	char is_ptr;

	void (*defer_fn)(void*);
	int (*cmp_fn)(const void*, const void*);
	void (*print_fn)(const void*);
};

#ifndef CYX_ARRAY_BASE_SIZE
#define CYX_ARRAY_BASE_SIZE 32
#endif // CYX_ARRAY_BASE_SIZE

#define __CYX_ARRAY_HEADER_SIZE (sizeof(__CyxArrayHeader))
#define __CYX_ARRAY_GET_HEADER(arr) ((__CyxArrayHeader*)__CYX_GET_TYPE(arr) - 1)
#define __CYX_ARRAY_TYPECHECK(arr) assert(*__CYX_GET_TYPE(arr) == __CYX_TYPE_ARRAY && \
		"ERROR: Type missmatch! Type of the provided pointer is not array but was expected to be of type array!")

void* __cyx_array_new(struct __CyxArrayParams params);
void* __cyx_array_copy(void* arr);
void __cyx_array_expand(void** arr_ptr, size_t n);
void __cyx_array_append(void** arr, void* val);
void __cyx_array_insert(void** arr, void* val, int pos);
void* __cyx_array_remove(void* arr, int pos);
void* __cyx_array_at(void* arr, int pos);
void cyx_array_reverse(void* arr);
void cyx_array_free(void* arr);
void __cyx_array_append_mult_n(void** arr_ptr, size_t n, const void* mult);
void cyx_array_print(const void* arr);
void cyx_array_clear(void* arr);

void __cyx_array_sort(void* arr, int start, int end);
void* __cyx_array_map(const void* arr, void (*fn)(void*, const void*));
void* cyx_array_map_self(void* arr, void (*fn)(void*, const void*));
void* __cyx_array_filter(const void* arr, int (*fn)(const void*));
void* cyx_array_filter_self(void* arr, int (*fn)(const void*));
int cyx_array_find(void* arr, void* val);
int cyx_array_find_by(void* arr, int (*fn)(const void*));
void* __cyx_array_fold(void* arr, void* accumulator, void (*fn)(void*, const void*));

#define cyx_array_length(arr) (__CYX_ARRAY_GET_HEADER(arr)->len)
#define cyx_array_foreach(val, arr) __CYX_ARRAY_TYPECHECK(arr); \
	for (struct { typeof(*arr)* value; size_t idx; } val = { .value = arr, .idx = 0 }; \
		val.idx < cyx_array_length(arr); ++val.idx, \
		val.value = (typeof(*arr)*)((char*)val.value + __CYX_ARRAY_GET_HEADER(arr)->size) )
#define cyx_array_drain(val, arr) __CYX_ARRAY_TYPECHECK(arr); \
	for (typeof(*arr)* val = NULL; (val = array_pop(arr));)

#define __cyx_array_new_params(...) (__cyx_array_new((struct __CyxArrayParams){ __VA_ARGS__ }))
#ifndef CYLIBX_ALLOC // array_new()
#define cyx_array_new(T, ...) ((T*)__cyx_array_new_params(.__size = sizeof(T), __VA_ARGS__))
#else
#define cyx_array_new(T, allocator, ...) ((T*)__cyx_array_new_params(.__size = sizeof(T), .__allocator = (allocator), __VA_ARGS__))
#endif // CYLIBX_ALLOC
#define cyx_array_copy(arr) ((typeof(*arr)*)__cyx_array_copy(arr))
#define cyx_array_append(arr, val) do { \
	typeof(*arr) v = (val); \
	__cyx_array_append((void**)&(arr), &v); \
} while(0)
#define cyx_array_append_mult_n(arr, n, mult) (__cyx_array_append_mult_n((void**)&(arr), n, mult))
#define cyx_array_append_mult(arr, ...) do { \
	typeof(*arr) mult[] = { __VA_ARGS__ }; \
	__cyx_array_append_mult_n((void**)&(arr), sizeof(mult)/sizeof(*(mult)), mult); \
} while(0)
#define cyx_array_insert(arr, val, pos) do { \
	typeof(*arr) v = (val); \
	__cyx_array_insert((void**)&(arr), &v, pos); \
} while(0)
#define cyx_array_remove(arr, pos) ((typeof(*arr)*)__cyx_array_remove(arr, pos))
#define cyx_array_pop(arr) ((typeof(*arr)*)__cyx_array_remove(arr, -1))
#define cyx_array_at(arr, pos) ((typeof(*arr)*)__cyx_array_at(arr, pos))
#define cyx_array_top(arr) ((typeof(*arr)*)__cyx_array_at(arr, -1))
#define cyx_array_set_cmp(arr, cmp) do { __CYX_ARRAY_GET_HEADER(arr)->cmp_fn = cmp; } while(0)
#define cyx_array_sort(arr) do { \
	assert(arr); \
	assert(__CYX_ARRAY_GET_HEADER(arr)->cmp_fn && "ERROR: Trying to sort without a compare function provided!"); \
	__cyx_array_sort(arr, 0, __CYX_ARRAY_GET_HEADER(arr)->len - 1); \
} while(0)
#define cyx_array_map(arr, fn) (typeof(*arr)*)__cyx_array_map(arr, fn)
#define cyx_array_filter(arr, fn) (typeof(*arr)*)__cyx_array_filter(arr, fn)
#define cyx_array_fold(arr, accumulator, fn) ({ \
	typeof(*arr) acc = (accumulator); \
	(typeof(*arr)*)__cyx_array_fold(arr, &acc, fn); \
})

#ifdef CYLIBX_STRIP_PREFIX

#define array_length(arr) cyx_array_length(arr)
#define array_foreach(val, arr) cyx_array_foreach(val, arr)
#define array_drain(val, arr) cyx_array_drain(val, arr)

#ifndef CYLIBX_ALLOC
#define array_new(T, ...) cyx_array_new(T, __VA_ARGS__) 
#else
#define array_new(T, allocator, ...) cyx_array_new(T, allocator, __VA_ARGS__) 
#endif // CYLIBX_ALLOC 
#define array_copy(arr) cyx_array_copy(arr)
#define array_append(arr, val) cyx_array_append(arr, val)
#define array_append_mult_n(arr, n, mult) cyx_array_append_mult_n(arr, n, mult)
#define array_append_mult(arr, ...) cyx_array_append_mult(arr, __VA_ARGS__)
#define array_remove(arr, pos) cyx_array_remove(arr, pos)
#define array_pop(arr) cyx_array_pop(arr)
#define array_insert(arr, val, pos) cyx_array_insert(arr, val, pos)
#define array_at(arr, pos) cyx_array_at(arr, pos)
#define array_set_cmp(arr, cmp) cyx_array_set_cmp(arr, cmp)
#define array_sort(arr) cyx_array_sort(arr)
#define array_map(arr, fn) cyx_array_map(arr, fn)
#define array_filter(arr, fn) cyx_array_filter(arr, fn)
#define array_fold(arr, accumulator, fn) cyx_array_fold(arr, accumulator, fn)

#define array_reverse cyx_array_reverse
#define array_free cyx_array_free
#define array_print cyx_array_print
#define array_clear cyx_array_clear
#define array_map_self cyx_array_map_self
#define array_filter_self cyx_array_filter_self
#define array_find cyx_array_find
#define array_find_by cyx_array_find_by

#endif // CYLIBX_STRIP_PREFIX

#ifdef CYLIBX_IMPLEMENTATION

#ifndef CYLIBX_ALLOC // __cyx_array_new()
void* __cyx_array_new(struct __CyxArrayParams params) {
	__CyxArrayHeader* head;
	if (!params.reserve) {
		head = malloc(__CYX_ARRAY_HEADER_SIZE + __CYX_TYPE_SIZE + CYX_ARRAY_BASE_SIZE * params.__size);
		if (!head) { return NULL; }
		head->cap = CYX_ARRAY_BASE_SIZE;
	} else {
		head = malloc(__CYX_ARRAY_HEADER_SIZE + __CYX_TYPE_SIZE + params.reserve * params.__size);
		if (!head) { return NULL; }
		head->cap = params.reserve;
	}
	head->size = params.__size;
	head->len = 0;
	head->is_ptr = params.is_ptr;
	head->print_fn = params.print_fn;
	head->cmp_fn = params.cmp_fn;
	head->defer_fn = params.defer_fn;

	enum __CyxDataType* type = (void*)(head + 1);
	*type = __CYX_TYPE_ARRAY;
	return (void*)(type + 2);
}
void* __cyx_array_copy(void* arr) {
	__CYX_ARRAY_TYPECHECK(arr);
	__CyxArrayHeader* head = __CYX_ARRAY_GET_HEADER(arr);

	__CyxArrayHeader* res_head = malloc(__CYX_ARRAY_HEADER_SIZE + __CYX_TYPE_SIZE + head->cap * head->size);
	if (!res_head) { return NULL; }
	memcpy(res_head, head, __CYX_ARRAY_HEADER_SIZE + __CYX_TYPE_SIZE + head->cap * head->size);

	enum __CyxDataType* type = (void*)(res_head + 1);
	*type = __CYX_TYPE_ARRAY;
	return (void*)(type + 2);
}
void __cyx_array_expand(void** arr_ptr, size_t n) {
	__CyxArrayHeader* head = __CYX_ARRAY_GET_HEADER(*arr_ptr);
	size_t new_cap = head->cap;

	while (n + head->len > (new_cap <<= 1));
	__CyxArrayHeader* new_head = malloc(__CYX_ARRAY_HEADER_SIZE + __CYX_TYPE_SIZE + new_cap * head->size);
	memcpy(new_head, head, __CYX_ARRAY_HEADER_SIZE + __CYX_TYPE_SIZE + head->cap * head->size);
	new_head->cap = new_cap;

	enum __CyxDataType* type = (void*)(new_head + 1);
	*type = __CYX_TYPE_ARRAY;


	free(head);
	*arr_ptr = (void*)(type + 2);
}
#else
void* __cyx_array_new(struct __CyxArrayParams params) {
	if (!params.__allocator) { params.__allocator = &EVO_LIBC_ALLOCATOR; }

	__CyxArrayHeader* head;
	if (!params.reserve) {
		head = evo_alloc_malloc(params.__allocator, __CYX_ARRAY_HEADER_SIZE + __CYX_TYPE_SIZE + CYX_ARRAY_BASE_SIZE * params.__size);
		if (!head) { return NULL; }
		head->cap = CYX_ARRAY_BASE_SIZE;
	} else {
		head = evo_alloc_malloc(params.__allocator, __CYX_ARRAY_HEADER_SIZE + __CYX_TYPE_SIZE + params.reserve * params.__size);
		if (!head) { return NULL; }
		head->cap = params.reserve;
	}
	head->size = params.__size;
	head->len = 0;
	head->is_ptr = params.is_ptr;
	head->print_fn = params.print_fn;
	head->cmp_fn = params.cmp_fn;
	head->defer_fn = params.defer_fn;
	head->alloc = params.__allocator;

	enum __CyxDataType* type = (void*)(head + 1);
	*type = __CYX_TYPE_ARRAY;
	return (void*)(type + 2);
}
void* __cyx_array_copy(void* arr) {
	__CYX_ARRAY_TYPECHECK(arr);
	__CyxArrayHeader* head = __CYX_ARRAY_GET_HEADER(arr);

	__CyxArrayHeader* res_head = malloc(__CYX_ARRAY_HEADER_SIZE + __CYX_TYPE_SIZE + head->cap * head->size);
	if (!res_head) { return NULL; }
	memcpy(res_head, head, __CYX_ARRAY_HEADER_SIZE + __CYX_TYPE_SIZE + head->cap * head->size);

	enum __CyxDataType* type = (void*)(res_head + 1);
	*type = __CYX_TYPE_ARRAY;
	return (void*)(type + 2);
}
void __cyx_array_expand(void** arr_ptr, size_t n) {
	__CyxArrayHeader* head = __CYX_ARRAY_GET_HEADER(*arr_ptr);
	size_t new_cap = head->cap;

	while (n + head->len > (new_cap <<= 1));
	__CyxArrayHeader* new_head = evo_alloc_malloc(head->alloc, __CYX_ARRAY_HEADER_SIZE + __CYX_TYPE_SIZE + new_cap * head->size);
	memcpy(new_head, head, __CYX_ARRAY_HEADER_SIZE + __CYX_TYPE_SIZE + head->cap * head->size);
	new_head->cap = new_cap;

	enum __CyxDataType* type = (void*)(new_head + 1);
	*type = __CYX_TYPE_ARRAY;

	if (head->alloc->free_f) {
		evo_alloc_free(head->alloc, head);
	}
	*arr_ptr = (void*)(type + 2);
}
#endif // CYLIBX_ALLOC
void __cyx_array_append(void** arr, void* val) {
	__CYX_ARRAY_TYPECHECK(*arr);

	__CyxArrayHeader* head = __CYX_ARRAY_GET_HEADER(*arr);
	if (head->len == head->cap) {
		__cyx_array_expand(arr, 1);
		head = __CYX_ARRAY_GET_HEADER(*arr);
	}
	memcpy(__CYX_DATA_GET_AT(head, *arr, head->len), val, head->size);
	head->len++;
}
void __cyx_array_insert(void** arr, void* val, int pos) {
	__CYX_ARRAY_TYPECHECK(*arr);

	__CyxArrayHeader* head = __CYX_ARRAY_GET_HEADER(*arr);
	if (pos < 0) { pos += head->len; }
	if (head->len == head->cap) {
		__cyx_array_expand(arr, 1);
		head = __CYX_ARRAY_GET_HEADER(*arr);
	}
	memmove(__CYX_DATA_GET_AT(head, *arr, pos + 1), __CYX_DATA_GET_AT(head, *arr, pos), (head->len - pos) * head->size);
	memcpy(__CYX_DATA_GET_AT(head, *arr, pos), val, head->size);
	++head->len;
}
void* __cyx_array_remove(void* arr, int pos) {
	__CYX_ARRAY_TYPECHECK(arr);

	__CyxArrayHeader* head = __CYX_ARRAY_GET_HEADER(arr);
	if (pos < 0) { pos += head->len; }
	assert(pos < (int)head->len);

	if (pos + 1 != (int)head->len) {
		memmove(__CYX_DATA_GET_AT(head, arr, pos), __CYX_DATA_GET_AT(head, arr, pos + 1), (head->len - pos) * head->size);
	}
	--head->len;
	return NULL;
}
void* __cyx_array_at(void* arr, int pos) {
	if (!arr) { return NULL; }
	__CYX_ARRAY_TYPECHECK(arr);

	__CyxArrayHeader* head = __CYX_ARRAY_GET_HEADER(arr);
	if (pos < 0) { pos += head->len; }
	if (pos >= (int)head->len) { return NULL; }

	return (char*)arr + pos * head->size;
}
void cyx_array_reverse(void* arr) {
	__CYX_ARRAY_TYPECHECK(arr);

	__CyxArrayHeader* head = __CYX_ARRAY_GET_HEADER(arr);

	if (head->len <= 1) { return; }

	__cyx_temp_set_mark(&__cyx_temp_stack);
	void* temp = __cyx_temp_malloc(&__cyx_temp_stack, head->size);
	for (size_t i = 0, j = head->len - 1; i < j; ++i, --j) {
		memcpy(temp, __CYX_DATA_GET_AT(head, arr, i), head->size);
		memcpy(__CYX_DATA_GET_AT(head, arr, i), __CYX_DATA_GET_AT(head, arr, j), head->size);
		memcpy(__CYX_DATA_GET_AT(head, arr, j), temp, head->size);
	}
	__cyx_temp_reset_mark(&__cyx_temp_stack);
}
void cyx_array_free(void* arr) {
	__CYX_ARRAY_TYPECHECK(arr);

	__CyxArrayHeader* head = __CYX_ARRAY_GET_HEADER(arr);
	if (head->defer_fn) {
		if (!head->is_ptr) {
			for (size_t i = 0; i < head->len; ++i) {
				head->defer_fn((char*)arr + i * head->size);
			}
		} else {
			for (size_t i = 0; i < head->len; ++i) {
				head->defer_fn(*(void**)((char*)arr + i * head->size));
			}
		}
	}
#ifndef CYLIBX_ALLOC
	free(head);
#else
	if (head->alloc->free_f) {
		free(head);
	} else {
		fprintf(stderr, "ERROR:\tThe allocator you've provided to the array doesn't free single elements as you expected!\n");
	}
#endif // CYLIBX_ALLOC
}
void __cyx_array_append_mult_n(void** arr_ptr, size_t n, const void* mult) {
	void* arr = *arr_ptr;
	__CYX_ARRAY_TYPECHECK(arr);

	__CyxArrayHeader* head = __CYX_ARRAY_GET_HEADER(arr);

	if (head->len + n >= head->cap) {
		__cyx_array_expand(arr_ptr, n);
		arr = *arr_ptr;
		head = __CYX_ARRAY_GET_HEADER(arr);
	}

	memcpy(__CYX_DATA_GET_AT(head, arr, head->len), mult, n * head->size);
	head->len += n;
}
void cyx_array_print(const void* arr) {
	__CYX_ARRAY_TYPECHECK(arr);

	__CyxArrayHeader* head = __CYX_ARRAY_GET_HEADER(arr);
	assert(head->print_fn && "ERROR: No print function provided to the array!");
	printf("[ ");
	if (!head->is_ptr) {
		for (size_t i = 0; i < head->len; ++i) {
			if (i != 0) { printf(", "); }
			head->print_fn((char*)arr + i * head->size);
		}
	} else {
		for (size_t i = 0; i < head->len; ++i) {
			if (i != 0) { printf(", "); }
			head->print_fn(*(void**)((char*)arr + i * head->size));
		}
	}
	printf(" ]");
}
void cyx_array_clear(void* arr) {
	__CYX_ARRAY_TYPECHECK(arr);
	__CyxArrayHeader* head = __CYX_ARRAY_GET_HEADER(arr);
	head->len = 0;
}

void __cyx_array_sort(void* arr, int start, int end) {
	__CYX_ARRAY_TYPECHECK(arr);

	__CyxArrayHeader* head = __CYX_ARRAY_GET_HEADER(arr);
	if (end - start <= 1 || end < start) {
		if (end - start == 1 && (!head->is_ptr ?
			head->cmp_fn((char*)arr + start * head->size, (char*)arr + (start + 1) * head->size) == 1 :
			head->cmp_fn(*(void**)((char*)arr + start * head->size), *(void**)((char*)arr + (start + 1) * head->size)) == 1)) {
			__CYX_SWAP((char*)arr + start * head->size, (char*)arr + (start + 1) * head->size, head->size);
		}
		return;
	}
	void* pivot = (char*)arr + end * head->size;
	void* a = (char*)arr + (start - 1) * head->size;
	if (!head->is_ptr) {
		for (void* b = (char*)arr + start * head->size; b < pivot; b = (char*)b + head->size) {
			if (head->cmp_fn(b, pivot) == -1) {
				a = (char*)a + head->size;
				__CYX_SWAP(a, b, head->size);
			}
		}
	} else {
		for (void* b = (char*)arr + start * head->size; b < pivot; b = (char*)b + head->size) {
			if (head->cmp_fn(*(void**)b, *(void**)pivot) == -1) {
				a = (char*)a + head->size;
				__CYX_SWAP(a, b, head->size);
			}
		}
	}
	a = (char*)a + head->size;
	int mid = ((size_t)a - (size_t)arr) / head->size;
	__CYX_SWAP(a, pivot, head->size);

	__cyx_array_sort(arr, start, mid - 1);
	__cyx_array_sort(arr, mid + 1, end);
}
void* __cyx_array_map(const void* arr, void (*fn)(void*, const void*)) {
	__CYX_ARRAY_TYPECHECK(arr);

	__CyxArrayHeader* head = __CYX_ARRAY_GET_HEADER(arr);

	void* curr = (void*)arr;
	void* res = __cyx_array_new((struct __CyxArrayParams){
		.__size = head->size,
		.reserve = head->cap,
		.defer_fn = head->defer_fn,
		.print_fn = head->print_fn,
		.cmp_fn = head->cmp_fn,
	});

	__cyx_temp_set_mark(&__cyx_temp_stack);
	void* curr_res = __cyx_temp_malloc(&__cyx_temp_stack, head->size);
	for (size_t i = 0; i < head->len; ++i) {
		fn(curr_res, curr);
		__cyx_array_append(&res, curr_res);
		curr = (char*)curr + head->size;
	}
	__cyx_temp_reset_mark(&__cyx_temp_stack);
	return res;
}
void* cyx_array_map_self(void* arr, void (*fn)(void*, const void*)) {
	__CYX_ARRAY_TYPECHECK(arr);

	__CyxArrayHeader* head = __CYX_ARRAY_GET_HEADER(arr);
	void* val = arr;

	__cyx_temp_set_mark(&__cyx_temp_stack);
	void* res = __cyx_temp_malloc(&__cyx_temp_stack, head->size);
	for (size_t i = 0; i < head->len; ++i) {
		fn(res, val);
		memcpy(val, res, head->size);
		val = (char*)val + head->size;
	}
	__cyx_temp_reset_mark(&__cyx_temp_stack);
	return arr;
}
void* __cyx_array_filter(const void* arr, int (*fn)(const void*)) {
	__CYX_ARRAY_TYPECHECK(arr);

	__CyxArrayHeader* head = __CYX_ARRAY_GET_HEADER(arr);

	void* curr = (void*)arr;
	void* res = __cyx_array_new((struct __CyxArrayParams){
		.__size = head->size,
		.is_ptr = head->is_ptr,
		.reserve = head->cap,
		.defer_fn = head->defer_fn,
		.print_fn = head->print_fn,
		.cmp_fn = head->cmp_fn,
	});

	for (size_t i = 0; i < head->len; ++i) {
		if (fn(curr)) { __cyx_array_append(&res, curr); }
		curr = (char*)curr + head->size;
	}
	return res;
}
void* cyx_array_filter_self(void* arr, int (*fn)(const void*)) {
	__CYX_ARRAY_TYPECHECK(arr);

	__CyxArrayHeader* head = __CYX_ARRAY_GET_HEADER(arr);
	size_t move_back = 0;
	
	for (size_t i = 0; i < head->len; ++i) {
		if (fn((char*)arr + i * head->size)) {
			memcpy((char*)arr + (i - move_back) * head->size, (char*)arr + i * head->size, head->size);
		} else {
			++move_back;
		}
	}
	head->len -= move_back;
	return arr;
}
int cyx_array_find(void* arr, void* val) {
	__CYX_ARRAY_TYPECHECK(arr);

	__CyxArrayHeader* head = __CYX_ARRAY_GET_HEADER(arr);
	for (size_t i = 0; i < head->len; ++i) {
		if (!head->cmp_fn(val, (char*)arr + i * head->size)) {
			return (int)i;
		}
	}
	return -1;
}
int cyx_array_find_by(void* arr, int (*fn)(const void*)) {
	__CYX_ARRAY_TYPECHECK(arr);

	__CyxArrayHeader* head = __CYX_ARRAY_GET_HEADER(arr);
	for (size_t i = 0; i < head->len; ++i) {
		if (fn((char*)arr + i * head->size)) {
			return (int)i;
		}
	}
	return -1;
}
void* __cyx_array_fold(void* arr, void* accumulator, void (*fn)(void*, const void*)) {
	__CYX_ARRAY_TYPECHECK(arr);

	__CyxArrayHeader* head = __CYX_ARRAY_GET_HEADER(arr);

	void* ret = __cyx_temp_malloc(&__cyx_temp_stack, head->size); // TODO: Technically fills the temp allocator should be fixed
	if (head->defer_fn) { head->defer_fn(!head->is_ptr ? accumulator : *(void**)accumulator); }

	for (size_t i = 0; i < head->len; ++i) { fn(ret, __CYX_DATA_GET_AT(head, arr, i)); }

	return ret;
}

#endif // CYLIBX_IMPLEMENTATION

#endif // __CYX_CLOSE_FOLD

/*
 * Bitmap
 */

#if __CYX_CLOSE_FOLD

typedef struct {
	size_t size;

#ifdef CYLIBX_ALLOC
	EvoAllocator* alloc;
#endif // CYLIBX_ALLOC
} __CyxBitmapHeader;

#define __CYX_BITMAP_CALC_SIZE(size) (((size) - 1) / (8 * sizeof(size_t)) + 1)
#define __CYX_BITMAP_HEADER_SIZE (sizeof(__CyxBitmapHeader))
#define __CYX_BITMAP_GET_HEADER(bitmap) ((__CyxBitmapHeader*)__CYX_GET_TYPE(bitmap) - 1)
#define __CYX_BITMAP_TYPECHECK(arr) assert(*__CYX_GET_TYPE(arr) == __CYX_TYPE_BITMAP && \
	"ERROR: Type missmatch! Type of the provided pointer is not bitmap but was expected to be of type bitmap!")
#define cyx_bitmap_size(bitmap) (__CYX_BITMAP_GET_HEADER(bitmap)->size)

#ifndef CYLIBX_ALLOC // bitmap_new
size_t* cyx_bitmap_new(size_t size);
#else
size_t* cyx_bitmap_new(EvoAllocator* allocator, size_t size);
#endif // CYLIBX_ALLOC
size_t* cyx_bitmap_copy(const size_t* const bitmap);
int cyx_bitmap_get(const size_t* const bitmap, int pos);
void cyx_bitmap_flip(size_t* bitmap, int pos);
void cyx_bitmap_set(size_t* bitmap, int pos, int val);
void cyx_bitmap_free(void* bitmap);
void cyx_bitmap_print(const void* bitmap);
size_t* cyx_bitmap_and(const size_t* const bitmap1, const size_t* const bitmap2);
size_t* cyx_bitmap_and_self(size_t* self, const size_t* const other);
size_t* cyx_bitmap_or(const size_t* const bitmap1, const size_t* const bitmap2);
size_t* cyx_bitmap_or_self(size_t* self, const size_t* const other);
size_t* cyx_bitmap_xor(const size_t* const bitmap1, const size_t* const bitmap2);
size_t* cyx_bitmap_xor_self(size_t* self, const size_t* const other);

#ifdef CYLIBX_STRIP_PREFIX

#define bitmap_size(bitmap) cyx_bitmap_size(bitmap)

#define bitmap_new cyx_bitmap_new
#define bitmap_copy cyx_bitmap_copy
#define bitmap_get cyx_bitmap_get
#define bitmap_flip cyx_bitmap_flip
#define bitmap_set cyx_bitmap_set
#define bitmap_free cyx_bitmap_free
#define bitmap_print cyx_bitmap_print

#define bitmap_and cyx_bitmap_and
#define bitmap_and_self cyx_bitmap_and_self
#define bitmap_or cyx_bitmap_or
#define bitmap_or_self cyx_bitmap_or_self
#define bitmap_xor cyx_bitmap_xor
#define bitmap_xor_self cyx_bitmap_xor_self

#endif // CYLIBX_STRIP_PREFIX

#ifdef CYLIBX_IMPLEMENTATION


#ifndef CYLIBX_ALLOC // bitmap_new
size_t* cyx_bitmap_new(size_t size) {
	size_t to_alloc = __CYX_BITMAP_HEADER_SIZE + __CYX_TYPE_SIZE + __CYX_BITMAP_CALC_SIZE(size) * sizeof(size_t);
	__CyxBitmapHeader* ret = malloc(to_alloc);
	memset(ret, 0, to_alloc);
	if (!ret) { return NULL; }
	ret->size = size;

	enum __CyxDataType* type = (void*)(ret + 1);
	*type = __CYX_TYPE_BITMAP;
	return (void*)(type + 2);
}
#else
size_t* cyx_bitmap_new(EvoAllocator* allocator, size_t size) {
	if (!allocator) { allocator = &EVO_LIBC_ALLOCATOR; }
	size_t to_alloc = __CYX_BITMAP_HEADER_SIZE + __CYX_TYPE_SIZE + __CYX_BITMAP_CALC_SIZE(size) * sizeof(size_t);
	__CyxBitmapHeader* ret = evo_alloc_malloc(allocator, to_alloc);
	if (!ret) { return NULL; }
	memset(ret, 0, to_alloc);
	ret->size = size;
	ret->alloc= allocator;

	enum __CyxDataType* type = (void*)(ret + 1);
	*type = __CYX_TYPE_BITMAP;
	return (void*)(type + 2);
}
#endif // CYLIBX_ALLOC
size_t* cyx_bitmap_copy(const size_t* bitmap) {
	__CyxBitmapHeader* head = __CYX_BITMAP_GET_HEADER(bitmap);
	size_t alloc_size = sizeof(size_t) + __CYX_TYPE_SIZE + __CYX_BITMAP_CALC_SIZE(head->size) * sizeof(size_t);
#ifndef CYLIBX_ALLOC
	__CyxBitmapHeader* ret = malloc(alloc_size);
#else
	__CyxBitmapHeader* ret = evo_alloc_malloc(head->alloc, alloc_size);
#endif // CYLIBX_ALLOC
	if (!ret) { return NULL; }

	memcpy(ret, head, alloc_size);
	enum __CyxDataType* type = (void*)(ret + 1);
	*type = __CYX_TYPE_BITMAP;
	return (void*)(type + 2);
}
int cyx_bitmap_get(const size_t* bitmap, int pos) {
	__CYX_BITMAP_TYPECHECK(bitmap);

	if (pos < 0) { pos += cyx_bitmap_size(bitmap); }
	assert(pos < (int)cyx_bitmap_size(bitmap) && "ERROR: Trying to access an out of range value!");

	int bitmap_pos = pos / (8 * sizeof(size_t));
	int inbyte_pos = pos - bitmap_pos * 8 * sizeof(size_t);

	return bitmap[bitmap_pos] & (0b1ull << inbyte_pos) ? 1 : 0;
}
void cyx_bitmap_flip(size_t* bitmap, int pos) {
	__CYX_BITMAP_TYPECHECK(bitmap);

	if (pos < 0) { pos += cyx_bitmap_size(bitmap); }
	assert(pos < (int)cyx_bitmap_size(bitmap) && "ERROR: Trying to access an out of range value!");

	int bitmap_pos = pos / (8 * sizeof(size_t));
	int inbyte_pos = pos - bitmap_pos * 8 * sizeof(size_t);

	if (bitmap[bitmap_pos] & (0b1 << inbyte_pos) ? 1 : 0) {
		bitmap[bitmap_pos] |= 0b1 << inbyte_pos;
	} else {
		bitmap[bitmap_pos] &= ~(0b1 << inbyte_pos);
	}
}
void cyx_bitmap_set(size_t* bitmap, int pos, int val) {
	__CYX_BITMAP_TYPECHECK(bitmap);

	if (pos < 0) { pos += cyx_bitmap_size(bitmap); }
	assert(pos < (int)cyx_bitmap_size(bitmap) && "ERROR: Trying to access an out of range value!");

	int bitmap_pos = pos / (8 * sizeof(size_t));
	int inbyte_pos = pos - bitmap_pos * 8 * sizeof(size_t);

	if (val) {
		bitmap[bitmap_pos] |= 0b1ull << inbyte_pos;
	} else {
		bitmap[bitmap_pos] &= ~(0b1ull << inbyte_pos);
	}
}
void cyx_bitmap_free(void* bitmap) {
	__CYX_BITMAP_TYPECHECK(bitmap);

	__CyxBitmapHeader* head = __CYX_BITMAP_GET_HEADER(bitmap);
#ifndef CYLIBX_ALLOC
	free(head);
#else
	if (head->alloc) {
		evo_alloc_free(head->alloc, head);
	} else {
		fprintf(stderr, "ERROR:\tThe allocator you've provided to the bitmap doesn't free single elements as you expected!\n");
	}
#endif // CYLIBX_ALLOC
}
void cyx_bitmap_print(const void* bitmap) {
	__CYX_BITMAP_TYPECHECK(bitmap);

	printf("0b");
	for (size_t i = 0; i < cyx_bitmap_size((size_t*)bitmap); ++i) {
		printf("%c", cyx_bitmap_get(bitmap, i) + '0');
	}
}

size_t* cyx_bitmap_and(const size_t* bitmap1, const size_t* bitmap2) {
	__CYX_BITMAP_TYPECHECK(bitmap1);
	__CYX_BITMAP_TYPECHECK(bitmap2);

	if (!bitmap1 || !bitmap2 || cyx_bitmap_size(bitmap1) != cyx_bitmap_size(bitmap2)) { return NULL; }
	size_t* res = cyx_bitmap_copy(bitmap1);
	size_t size = (cyx_bitmap_size(res) - 1) / (sizeof(size_t) * 8) + 1; 
	for (size_t i = 0; i < size; ++i) { res[i] &= bitmap2[i]; }
	return res;
}
size_t* cyx_bitmap_and_self(size_t* self, const size_t* other) {
	__CYX_BITMAP_TYPECHECK(self);
	__CYX_BITMAP_TYPECHECK(other);

	if (!self || !other || cyx_bitmap_size(self) != cyx_bitmap_size(other)) { return NULL; }
	size_t size = (cyx_bitmap_size(self) - 1) / (sizeof(size_t) * 8) + 1; 
	for (size_t i = 0; i < size; ++i) { self[i] &= other[i]; }
	return self;
}
size_t* cyx_bitmap_or(const size_t* bitmap1, const size_t* bitmap2) {
	__CYX_BITMAP_TYPECHECK(bitmap1);
	__CYX_BITMAP_TYPECHECK(bitmap2);

	if (!bitmap1 || !bitmap2 || cyx_bitmap_size(bitmap1) != cyx_bitmap_size(bitmap2)) { return NULL; }
	size_t* res = cyx_bitmap_copy(bitmap1);
	size_t size = (cyx_bitmap_size(res) - 1) / (sizeof(size_t) * 8) + 1; 
	for (size_t i = 0; i < size; ++i) { res[i] |= bitmap2[i]; }
	return res;
}
size_t* cyx_bitmap_or_self(size_t* self, const size_t* other) {
	__CYX_BITMAP_TYPECHECK(self);
	__CYX_BITMAP_TYPECHECK(other);

	if (!self || !other || cyx_bitmap_size(self) != cyx_bitmap_size(other)) { return NULL; }
	size_t size = (cyx_bitmap_size(self) - 1) / (sizeof(size_t) * 8) + 1; 
	for (size_t i = 0; i < size; ++i) { self[i] |= other[i]; }
	return self;
}
size_t* cyx_bitmap_xor(const size_t* bitmap1, const size_t* bitmap2) {
	__CYX_BITMAP_TYPECHECK(bitmap1);
	__CYX_BITMAP_TYPECHECK(bitmap2);

	if (!bitmap1 || !bitmap2 || cyx_bitmap_size(bitmap1) != cyx_bitmap_size(bitmap2)) { return NULL; }
	size_t* res = cyx_bitmap_copy(bitmap1);
	size_t size = (cyx_bitmap_size(res) - 1) / (sizeof(size_t) * 8) + 1; 
	for (size_t i = 0; i < size; ++i) { res[i] ^= bitmap2[i]; }
	return res;
}
size_t* cyx_bitmap_xor_self(size_t* self, const size_t* other) {
	__CYX_BITMAP_TYPECHECK(self);
	__CYX_BITMAP_TYPECHECK(other);

	if (!self || !other || cyx_bitmap_size(self) != cyx_bitmap_size(other)) { return NULL; }
	size_t size = (cyx_bitmap_size(self) - 1) / (sizeof(size_t) * 8) + 1; 
	for (size_t i = 0; i < size; ++i) { self[i] ^= other[i]; }
	return self;
}

#endif // CYLIBX_IMPLEMENTATION

#endif // __CYX_CLOSE_FOLD

/*
 * Hash Functions
 */

#if __CYX_CLOSE_FOLD

size_t cyx_hash_size_t(const void* val);
size_t cyx_hash_int8(const void* val);
size_t cyx_hash_int16(const void* val);
size_t cyx_hash_int32(const void* val);
size_t cyx_hash_int64(const void* val);
size_t cyx_hash_double(const void* val);
size_t cyx_hash_str(const void* val);

int cyx_eq_int8(const void* a, const void* b);
int cyx_eq_int16(const void* a, const void* b);
int cyx_eq_int32(const void* a, const void* b);
int cyx_eq_int64(const void* a, const void* b);
int cyx_eq_str(const void* a, const void* b);

int cyx_cmp_int8(const void* a, const void* b);
int cyx_cmp_int16(const void* a, const void* b);
int cyx_cmp_int32(const void* a, const void* b);
int cyx_cmp_int64(const void* a, const void* b);

#ifdef CYLIBX_STRIP_PREFIX

#define hash_size_t cyx_hash_size_t
#define hash_int8 cyx_hash_int8
#define hash_int16 cyx_hash_int16
#define hash_int32 cyx_hash_int32
#define hash_int64 cyx_hash_int64
#define hash_double cyx_hash_double
#define hash_str cyx_hash_str

#define eq_int8 cyx_eq_int8
#define eq_int16 cyx_eq_int16
#define eq_int32 cyx_eq_int32
#define eq_int64 cyx_eq_int64
#define eq_str cyx_eq_str

#define cmp_int8 cyx_cmp_int8
#define cmp_int16 cyx_cmp_int16
#define cmp_int32 cyx_cmp_int32
#define cmp_int64 cyx_cmp_int64

#endif // CYLIBX_STRIP_PREFIX

#ifdef CYLIBX_IMPLEMENTATION

size_t cyx_hash_size_t(const void* val) {
	size_t key = *(size_t*)val;
	key = (~key) + (key << 21);
	key = key ^ (key >> 24);
	key = (key + (key << 3)) + (key << 8);
	key = key ^ (key >> 14);
	key = (key + (key << 2)) + (key << 4);
	key = key ^ (key >> 28);
	key = key + (key << 31);
	return key;
}
size_t cyx_hash_int8(const void* val) {
	int8_t val_int = *(int8_t*)val;
	size_t val_size = ((size_t)val_int << 48) | ((size_t)val_int << 32) | ((size_t)val_int << 16) | (size_t)val_int;
	return cyx_hash_size_t(&val_size);
}
size_t cyx_hash_int16(const void* val) {
	int16_t val_int = *(int16_t*)val;
	size_t val_size = ((size_t)val_int << 48) | ((size_t)val_int << 32) | ((size_t)val_int << 16) | (size_t)val_int;
	return cyx_hash_size_t(&val_size);
}
size_t cyx_hash_int32(const void* val) {
	int32_t val_int = *(int32_t*)val;
	size_t val_size = ((size_t)val_int << 32) | (size_t)val_int;
	return cyx_hash_size_t(&val_size);
}
size_t cyx_hash_int64(const void* val) {
	int64_t val_int = *(int64_t*)val;
	size_t val_size = (size_t)val_int;
	return cyx_hash_size_t(&val_size);
}
size_t cyx_hash_double(const void* val) {
	return cyx_hash_size_t(val);
}
size_t cyx_hash_str(const void* val) {
    size_t hash = 5381;
	char* str = (char*)val;
	__CYX_STRING_TYPECHECK(str);
	for (size_t i = 0; i < cyx_str_length(str); ++i) { 
        hash = ((hash << 5) + hash) + str[i];
	}
    return hash;
}

int cyx_eq_int8(const void* a, const void* b) { return *(int8_t*)a == *(int8_t*)b; }
int cyx_eq_int16(const void* a, const void* b) { return *(int16_t*)a == *(int16_t*)b; }
int cyx_eq_int32(const void* a, const void* b) { return *(int32_t*)a == *(int32_t*)b; }
int cyx_eq_int64(const void* a, const void* b) { return *(int64_t*)a == *(int64_t*)b; }
int cyx_eq_str(const void* a, const void* b) {
	char* str_a = (char*)a;
	char* str_b = (char*)b;
	if (cyx_str_length(str_a) != cyx_str_length(str_b)) { return 0; }
	for (size_t i = 0; i < cyx_str_length(str_a); ++i) {
		if (str_a[i] != str_b[i]) { return 0; }
	}
	return 1;
}

int cyx_cmp_int8(const void* a, const void* b) {
	return *(int8_t*)a < *(int8_t*)b ? -1 : (*(int8_t*)a > *(int8_t*)b ? 1 : 0);
}
int cyx_cmp_int16(const void* a, const void* b) {
	return *(int16_t*)a < *(int16_t*)b ? -1 : (*(int16_t*)a > *(int16_t*)b ? 1 : 0);
}
int cyx_cmp_int32(const void* a, const void* b) {
	return *(int32_t*)a < *(int32_t*)b ? -1 : (*(int32_t*)a > *(int32_t*)b ? 1 : 0);
}
int cyx_cmp_int64(const void* a, const void* b) {
	return *(int64_t*)a < *(int64_t*)b ? -1 : (*(int64_t*)a > *(int64_t*)b ? 1 : 0);
}

#endif // CYLIBX_IMPLEMENTATION

#endif // __CYX_CLOSE_FOLD

/*
 * HashSet
 */

#if __CYX_CLOSE_FOLD

typedef struct {
	size_t len;
	size_t cap;
	size_t size;

	size_t (*hash_fn)(const void*);
	int (*eq_fn)(const void*, const void*);
	void (*defer_fn)(void*);
	void (*print_fn)(const void*);
	
#ifdef CYLIBX_ALLOC
	EvoAllocator* alloc;
#endif // CYLIBX_ALLOC

	char is_ptr;
} __CyxHashSetHeader;

struct __CyxHashSetParams {
#ifdef CYLIBX_ALLOC
	EvoAllocator* __allocator;
#endif // CYLIBX_ALLOC

	size_t __size;
	char is_ptr;

	size_t (*__hash_fn)(const void* const);
	int (*__eq_fn)(const void* const, const void* const);
	void (*defer_fn)(void*);
	void (*print_fn)(const void*);
};
struct __CyxHashSetContainsParams {
	const void* const __set;
	void* __val;
	char defer;
};

#ifndef CYX_HASH_SET_BASE_SIZE
#define CYX_HASH_SET_BASE_SIZE 32
#endif // CYX_HASH_SET_BASE_SIZE

#define __CYX_HASH_SET_HEADER_SIZE (sizeof(__CyxHashSetHeader))
#define __CYX_HASH_SET_GET_HEADER(set) ((__CyxHashSetHeader*)__CYX_GET_TYPE(set) - 1)
#define __CYX_HASH_SET_GET_BITMAP(set) ((size_t*)((char*)set + __CYX_HASH_SET_GET_HEADER(set)->cap * __CYX_HASH_SET_GET_HEADER(set)->size +\
			__CYX_BITMAP_HEADER_SIZE + __CYX_TYPE_SIZE))
#define __CYX_HASHSET_TYPECHECK(set) assert(*__CYX_GET_TYPE(set) == __CYX_TYPE_HASHSET && \
		"ERROR: Type missmatch! Type of the provided pointer is not hashset but was expected to be of type hashset!")

void* __cyx_hashset_new(struct __CyxHashSetParams params);
void* __cyx_hashset_copy(const void* set);
void __cyx_hashset_expand(void** set_ptr);
void __cyx_hashset_add(void** set_ptr, void* val);
void __cyx_hashset_add_mult_n(void** set_ptr, size_t n, void** mult);
void __cyx_hashset_remove(void* set, void* val);
int __cyx_hashset_contains(struct __CyxHashSetContainsParams params);
void cyx_hashset_free(void* set);
void cyx_hashset_print(const void* set);
void cyx_hashset_clear(void* set);

void* __cyx_hashset_union(const void* set1, const void* set2);
void* __cyx_hashset_union(const void* set1, const void* set2);
void* __cyx_hashset_union_self(void* self, const void* other);
void* __cyx_hashset_intersec(const void* set1, const void* set2);
void* __cyx_hashset_intersec_self(void* self, const void* other);
void* __cyx_hashset_diff(const void* set1, const void* set2);
void* __cyx_hashset_diff_self(void* self, const void* other);

#define cyx_hashset_length(set) (__CYX_HASH_SET_GET_HEADER(set)->len)
#define cyx_hashset_foreach(val, set) __CYX_HASHSET_TYPECHECK(set); \
	size_t __CYX_UNIQUE_VAL__(counter) = 0; \
	for (typeof(*set)* val = (set); \
		 __CYX_UNIQUE_VAL__(counter) < __CYX_HASH_SET_GET_HEADER(set)->cap; \
		 ++__CYX_UNIQUE_VAL__(counter), \
		 val = (void*)((char*)val + __CYX_HASH_SET_GET_HEADER(set)->size)) \
		if (cyx_bitmap_get(__CYX_HASH_SET_GET_BITMAP(set), 2 * __CYX_UNIQUE_VAL__(counter)) && \
			!cyx_bitmap_get(__CYX_HASH_SET_GET_BITMAP(set), 2 * __CYX_UNIQUE_VAL__(counter) + 1))

#define __cyx_hashset_new_params(...) (__cyx_hashset_new((struct __CyxHashSetParams){ __VA_ARGS__ }))

#ifndef CYLIBX_ALLOC //hashset_new
#define cyx_hashset_new(T, hash, eq, ...) ((T*)__cyx_hashset_new_params(.__size = sizeof(T), .__hash_fn = (hash), .__eq_fn = (eq), __VA_ARGS__))
#else
#define cyx_hashset_new(T, allocator, hash, eq, ...) ((T*)__cyx_hashset_new_params(.__allocator = (allocator), .__size = sizeof(T), .__hash_fn = (hash), .__eq_fn = (eq), __VA_ARGS__))
#endif // CYLIBX_ALLOC
#define cyx_hashset_copy(set) ((typeof(*set)*)__cyx_hashset_copy(set))
#define cyx_hashset_add(set, val) do { \
	typeof(*set) v = val; \
	__cyx_hashset_add((void**)&set, &v); \
} while(0)
#define cyx_hashset_add_mult_n(set, n, mult) (__cyx_hashset_add_mult_n((void**)&(set), n, (void**)mult))
#define cyx_hashset_add_mult(set, ...) do { \
	typeof(*set) mult[] = { __VA_ARGS__ };\
	__cyx_hashset_add_mult_n((void**)&(set), sizeof(mult) / sizeof(*mult), (void**)mult); \
} while (0)
#define cyx_hashset_remove(set, val) do { \
	typeof(*set) v = val; \
	__cyx_hashset_remove(set, &v); \
} while(0)
#define __cyx_hashset_contains_params(...) (__cyx_hashset_contains((struct __CyxHashSetContainsParams){ __VA_ARGS__ }))
#define cyx_hashset_contains(set, val, ...) ({ \
	typeof(*set) v = val; \
	__cyx_hashset_contains_params(.__set = set, .__val = &v, __VA_ARGS__); \
})

#define cyx_hashset_union(set1, set2) ((typeof(*set1)*)__cyx_hashset_union(set1, set2))
#define cyx_hashset_union_self(self, other) ((typeof(*self)*)__cyx_hashset_union_self(self, other))
#define cyx_hashset_intersec(set1, set2) ((typeof(*set1)*)__cyx_hashset_intersec(set1, set2))
#define cyx_hashset_intersec_self(self, other) ((typeof(*self)*)__cyx_hashset_intersec_self(self, other))
#define cyx_hashset_diff(set1, set2) ((typeof(*set1)*)__cyx_hashset_diff(set1, set2))
#define cyx_hashset_diff_self(self, other) ((typeof(*self)*)__cyx_hashset_diff_self(self, other))

#ifdef CYLIBX_STRIP_PREFIX

#define hashset_length(set) cyx_hashset_length(set)
#define hashset_foreach(val, set) cyx_hashset_foreach(val, set)

#ifndef CYLIBX_ALLOC
#define hashset_new(T, hash, eq, ...) cyx_hashset_new(T, hash, eq, __VA_ARGS__)
#else
#define hashset_new(T, allocator, hash, eq, ...) cyx_hashset_new(T, allocator, hash, eq, __VA_ARGS__)
#endif // CYLIBX_ALLOC
#define hashset_copy(set) cyx_hashset_copy(set)
#define hashset_add(set, val) cyx_hashset_add(set, val)
#define hashset_add_mult_n(set, n, mult) cyx_hashset_add_mult_n(set, n, mult)
#define hashset_add_mult(set, ...) cyx_hashset_add_mult(set, __VA_ARGS__)
#define hashset_remove(set, val) cyx_hashset_remove(set, val)
#define hashset_contains(set, val, ...) cyx_hashset_contains(set, val, __VA_ARGS__)

#define hashset_union(set1, set2) cyx_hashset_union(set1, set2)
#define hashset_union_self(self, other) cyx_hashset_union_self(self, other)
#define hashset_intersec(set1, set2) cyx_hashset_intersec(set1, set2)
#define hashset_intersec_self(self, other) cyx_hashset_intersec_self(self, other)
#define hashset_diff(set1, set2) cyx_hashset_diff(set1, set2)
#define hashset_diff_self(self, other) cyx_hashset_diff_self(self, other)

#define hashset_free cyx_hashset_free
#define hashset_print cyx_hashset_print
#define hashset_clear cyx_hashset_clear

#endif // CYLIBX_STRIP_PREFIX

#ifdef CYLIBX_IMPLEMENTATION

#ifndef CYLIBX_ALLOC // hashset_new
void* __cyx_hashset_new(struct __CyxHashSetParams params) {
	size_t to_alloc = __CYX_HASH_SET_HEADER_SIZE + __CYX_TYPE_SIZE +
		CYX_HASH_SET_BASE_SIZE * params.__size +
		__CYX_HASH_SET_HEADER_SIZE + __CYX_TYPE_SIZE + __CYX_BITMAP_CALC_SIZE(CYX_HASH_SET_BASE_SIZE * 2) * sizeof(size_t);

	__CyxHashSetHeader* head = malloc(to_alloc);
	memset(head, 0, to_alloc);

	enum __CyxDataType* type = (void*)(head + 1);
	*type = __CYX_TYPE_HASHSET;
	void* set = (void*)(type + 2);

	head->cap = CYX_HASH_SET_BASE_SIZE;
	head->size = params.__size;
	head->is_ptr = params.is_ptr;
	head->hash_fn = params.__hash_fn;
	head->eq_fn = params.__eq_fn;
	head->print_fn = params.print_fn;
	head->defer_fn = params.defer_fn;

	size_t* bitmap = __CYX_HASH_SET_GET_BITMAP(set);
	*__CYX_GET_TYPE(bitmap) = __CYX_TYPE_BITMAP;
	__CYX_BITMAP_GET_HEADER(bitmap)->size = CYX_HASH_SET_BASE_SIZE * 2;

	return set;
}
#else
void* __cyx_hashset_new(struct __CyxHashSetParams params) {
	size_t to_alloc = __CYX_HASH_SET_HEADER_SIZE + __CYX_TYPE_SIZE +
		CYX_HASH_SET_BASE_SIZE * params.__size +
		__CYX_BITMAP_HEADER_SIZE + __CYX_TYPE_SIZE + __CYX_BITMAP_CALC_SIZE(CYX_HASH_SET_BASE_SIZE * 2) * sizeof(size_t);

	if (!params.__allocator) { params.__allocator = &EVO_LIBC_ALLOCATOR; }
	__CyxHashSetHeader* head = evo_alloc_malloc(params.__allocator, to_alloc);
	if (!head) { return NULL; }
	memset(head, 0, to_alloc);

	enum __CyxDataType* type = (void*)(head + 1);
	*type = __CYX_TYPE_HASHSET;
	void* set = (void*)(type + 2);

	head->cap = CYX_HASH_SET_BASE_SIZE;
	head->size = params.__size;
	head->is_ptr = params.is_ptr;
	head->hash_fn = params.__hash_fn;
	head->eq_fn = params.__eq_fn;
	head->print_fn = params.print_fn;
	head->defer_fn = params.defer_fn;
	head->alloc = params.__allocator;

	size_t* bitmap = __CYX_HASH_SET_GET_BITMAP(set);
	*__CYX_GET_TYPE(bitmap) = __CYX_TYPE_BITMAP;
	__CYX_BITMAP_GET_HEADER(bitmap)->size = CYX_HASH_SET_BASE_SIZE * 2;

	return set;
}
#endif // CYLIBX_ALLOC
void* __cyx_hashset_copy(const void* set) {
	__CYX_HASHSET_TYPECHECK(set);

	__CyxHashSetHeader* head = __CYX_HASH_SET_GET_HEADER(set);
	void* set_res = __cyx_hashset_new((struct __CyxHashSetParams){
#ifdef CYLIBX_ALLOC
		.__allocator = head->alloc,
#endif // CYLIBX_ALLOC
		.__size = head->size,
		.__hash_fn = head->hash_fn,
		.__eq_fn = head->eq_fn,
		.is_ptr = head->is_ptr,
		.print_fn = head->print_fn,
		.defer_fn = head->defer_fn,
	});
	cyx_hashset_foreach(a, set) { __cyx_hashset_add(&set_res, (void*)a); }
	return set_res;
}
void __cyx_hashset_expand(void** set_ptr) {
	void* set = *set_ptr;

	__CyxHashSetHeader* head = __CYX_HASH_SET_GET_HEADER(set);
	size_t to_alloc = __CYX_HASH_SET_HEADER_SIZE + __CYX_TYPE_SIZE +
		2 * head->cap * head->size +
		__CYX_BITMAP_HEADER_SIZE + __CYX_TYPE_SIZE + __CYX_BITMAP_CALC_SIZE(head->cap * 4) * sizeof(size_t);

#ifndef CYLIBX_ALLOC
	__CyxHashSetHeader* new_head = malloc(to_alloc);
#else
	__CyxHashSetHeader* new_head = evo_alloc_malloc(head->alloc, to_alloc);
#endif // CYLIBX_ALLOC
	memset(new_head, 0, to_alloc);

	enum __CyxDataType* type = (void*)(new_head + 1);
	*type = __CYX_TYPE_HASHSET;
	void* new_set = (void*)(type + 2);
	memcpy(new_head, head, __CYX_HASH_SET_HEADER_SIZE);
	new_head->cap <<= 1;

	size_t* bitmap = __CYX_HASH_SET_GET_BITMAP(set);

	size_t* new_bitmap = __CYX_HASH_SET_GET_BITMAP(new_set);
	*__CYX_GET_TYPE(new_bitmap) = __CYX_TYPE_BITMAP;
	__CYX_BITMAP_GET_HEADER(new_bitmap)->size = 2 * new_head->cap;

	for (size_t i = 0; i < head->cap; ++i) {
		if (cyx_bitmap_get(bitmap, 2 * i)) {
			void* val = (char*)set + i * head->size;
			size_t pos = new_head->hash_fn(!new_head->is_ptr ? val : *(void**)val) % new_head->cap;
			for (size_t j = 0; j < new_head->cap; ++j) {
				size_t probe = (pos + j * j) % new_head->cap;
				if (!cyx_bitmap_get(new_bitmap, 2 * probe)) {
					memcpy((char*)new_set + probe * new_head->size, val, new_head->size);
					cyx_bitmap_set(new_bitmap, 2 * probe, 1);
					break;
				}
			}
		}
	}

#ifndef CYLIBX_ALLOC
	free(head);
#else
	if (head->alloc->free_f) {
		evo_alloc_free(head->alloc, head);
	}
#endif // CYLIBX_ALLOC
	*set_ptr = new_set;
}
void __cyx_hashset_add(void** set_ptr, void* val) {
	void* set = *set_ptr;
	__CYX_HASHSET_TYPECHECK(set);

	__CyxHashSetHeader* head = __CYX_HASH_SET_GET_HEADER(set);
	if (head->len * 1000 > head->cap * 650) {
		__cyx_hashset_expand(set_ptr);
		set = *set_ptr;
		head = __CYX_HASH_SET_GET_HEADER(set);
	}
	assert(head->hash_fn && "ERROR: No hash function provided to hashset!");

	size_t* bitmap = __CYX_HASH_SET_GET_BITMAP(set);
	size_t pos = head->hash_fn(!head->is_ptr ? val : *(void**)val) % head->cap;
	for (size_t i = 0; i < head->cap; ++i) {
		size_t probe = (pos + i * i) % head->cap;
		if (!cyx_bitmap_get(bitmap, 2 * probe)) {
			memcpy((char*)set + probe * head->size, val, head->size);
			cyx_bitmap_set(bitmap, 2 * probe, 1);
			++head->len;
			return;
		} else if (cyx_bitmap_get(bitmap, 2 * probe + 1)) {
			memcpy((char*)set + probe * head->size, val, head->size);
			cyx_bitmap_set(bitmap, 2 * probe + 1, 0);
			++head->len;
			return;
		} else if (!head->is_ptr ? head->eq_fn((char*)set + probe * head->size, val) : head->eq_fn(*(void**)((char*)set + probe * head->size), *(void**)val)) {
			return;
		}
	}
}
void __cyx_hashset_add_mult_n(void** set_ptr, size_t n, void** mult) {
	void* set = *set_ptr;

	__CYX_HASHSET_TYPECHECK(set);
	__CyxHashSetHeader* head = __CYX_HASH_SET_GET_HEADER(set);
	while ((head->len + n) * 1000 > head->cap * 650) {
		__cyx_hashset_expand(set_ptr);
		set = *set_ptr;
		head = __CYX_HASH_SET_GET_HEADER(set);
	}
	assert(head->hash_fn && "ERROR: No hash function provided to hashset!");

	size_t* bitmap = __CYX_HASH_SET_GET_BITMAP(set);
	for (size_t nn = 0; nn < n; ++nn) {
		void* val = (char*)mult + nn * head->size;
		size_t pos = head->hash_fn(!head->is_ptr ? val : *(void**)val) % head->cap;
		for (size_t i = 0; i < head->cap; ++i) {
			size_t probe = (pos + i * i) % head->cap;
			if (!cyx_bitmap_get(bitmap, 2 * probe)) {
				memcpy((char*)set + probe * head->size, val, head->size);
				cyx_bitmap_set(bitmap, 2 * probe, 1);
				++head->len;
				return;
			} else if (cyx_bitmap_get(bitmap, 2 * probe + 1)) {
				memcpy((char*)set + probe * head->size, val, head->size);
				cyx_bitmap_set(bitmap, 2 * probe + 1, 0);
				++head->len;
				return;
			} else if (!head->is_ptr ? head->eq_fn((char*)set + probe * head->size, val) : head->eq_fn(*(void**)((char*)set + probe * head->size), *(void**)val)) {
				return;
			}
		}
	}
}
void __cyx_hashset_remove(void* set, void* val) {
	__CYX_HASHSET_TYPECHECK(set);
	__CyxHashSetHeader* head = __CYX_HASH_SET_GET_HEADER(set);
	assert(head->hash_fn && "ERROR: No hash function provided to hashset!");
	if (head->len > 0) { return; }

	size_t* bitmap = __CYX_HASH_SET_GET_BITMAP(set);
	size_t pos = head->hash_fn(!head->is_ptr ? val : *(void**)val);
	char found = -1;
	for (size_t i = 0; i < head->cap; ++i) {
		size_t probe = (pos + i * i) % head->cap;
		if (cyx_bitmap_get(bitmap, 2 * probe)) {
			if (!head->is_ptr ? head->eq_fn((char*)set + probe * head->size, val) : head->eq_fn(*(void**)((char*)set + probe * head->size), *(void**)val)) {
				found = (int)probe;
			}
		}
	}
	if (found != -1) {
		cyx_bitmap_set(bitmap, 2 * found + 1, 1);
		if (head->defer_fn) {
			if (!head->is_ptr) {
				head->defer_fn(__CYX_DATA_GET_AT(head, set, found));
				head->defer_fn(val);
			} else {
				head->defer_fn(*(void**)__CYX_DATA_GET_AT(head, set, found));
				head->defer_fn(*(void**)val);
			}
		}
		--head->len;
	}
}
int __cyx_hashset_contains(struct __CyxHashSetContainsParams params) {
	void* set = (void*)params.__set;
	void* val = params.__val;

	__CYX_HASHSET_TYPECHECK(set);
	__CyxHashSetHeader* head = __CYX_HASH_SET_GET_HEADER(set);
	size_t* bitmap = __CYX_HASH_SET_GET_BITMAP(set);

	int res = 0;
	for (size_t i = 0; i < head->cap; ++i) {
		if (cyx_bitmap_get(bitmap, 2 * i) && !cyx_bitmap_get(bitmap, 2 * i + 1)) {
			if (!head->is_ptr ?
					head->eq_fn((char*)set + i * head->size, val) :
					head->eq_fn(*(void**)((char*)set + i * head->size), *(void**)val)) {
				res = 1;
				break;
			} else {
				break;
			}
		}
	}
	if (params.defer && head->defer_fn) {
		head->defer_fn(!head->is_ptr ? val : *(void**)val);
	}
	return res;
}
void cyx_hashset_free(void* set) {
	__CYX_HASHSET_TYPECHECK(set);

	__CyxHashSetHeader* head = __CYX_HASH_SET_GET_HEADER(set);

	if (head->defer_fn) {
		if (!head->is_ptr) {
			cyx_hashset_foreach(a, set) { head->defer_fn(a); }
		} else {
			cyx_hashset_foreach(a, set) { head->defer_fn(*(void**)a); }
		}
	}
	
#ifndef CYLIBX_ALLOC
	free(head);
#else
	if (head->alloc->free_f) {
		evo_alloc_free(head->alloc, head);
	} else {
		fprintf(stderr, "ERROR:\tThe allocator you've provided to the hashset doesn't free single elements as you expected!\n");
	}
#endif // CYLIBX_ALLOC
}
void cyx_hashset_print(const void* set) {
	__CYX_HASHSET_TYPECHECK(set);

	__CyxHashSetHeader* head = __CYX_HASH_SET_GET_HEADER(set);
	printf("{ ");
	char started = 0;
	if (!head->is_ptr) {
		cyx_hashset_foreach(a, set) {
			if (started) { printf(", "); } else { started = 1; }
			head->print_fn(a);
		}
	} else {
		cyx_hashset_foreach(a, set) {
			if (started) { printf(", "); } else { started = 1; }
			head->print_fn(*(void**)a);
		}
	}
	printf(" }");
}
void cyx_hashset_clear(void* set) {
	__CYX_HASHSET_TYPECHECK(set);

	__CyxHashSetHeader* head = __CYX_HASH_SET_GET_HEADER(set);
	head->len = 0;
	if (head->defer_fn) {
		cyx_hashset_foreach(a, set) {
			head->defer_fn(!head->is_ptr ? a : *(void**)a);
		}
	}

	size_t* bitmap = __CYX_HASH_SET_GET_BITMAP(set);
	__CyxBitmapHeader* bitmap_head = __CYX_BITMAP_GET_HEADER(bitmap);
	memset(bitmap, 0, __CYX_BITMAP_CALC_SIZE(bitmap_head->size) * sizeof(size_t));
}
void* __cyx_hashset_union(const void* set1, const void* set2) {
	__CYX_HASHSET_TYPECHECK(set1);
	__CYX_HASHSET_TYPECHECK(set2);

	__CyxHashSetHeader* head1 = __CYX_HASH_SET_GET_HEADER(set1);
	__CyxHashSetHeader* head2 = __CYX_HASH_SET_GET_HEADER(set2);
	assert(head1->size == head2->size && head1->hash_fn == head2->hash_fn && head1->eq_fn == head2->eq_fn);

	void* set_res = __cyx_hashset_copy(set1);
	cyx_hashset_foreach(a, set2) { __cyx_hashset_add(&set_res, (void*)a); }

	return set_res;
}
void* __cyx_hashset_union_self(void* self, const void* other) {
	__CYX_HASHSET_TYPECHECK(self);
	__CYX_HASHSET_TYPECHECK(other);

	__CyxHashSetHeader* head1 = __CYX_HASH_SET_GET_HEADER(self);
	__CyxHashSetHeader* head2 = __CYX_HASH_SET_GET_HEADER(other);
	assert(head1->size == head2->size && head1->hash_fn == head2->hash_fn && head1->eq_fn == head2->eq_fn);

	cyx_hashset_foreach(a, other) { __cyx_hashset_add(&self, (void*)a); }

	return self;
}
void* __cyx_hashset_intersec(const void* set1, const void* set2) {
	__CYX_HASHSET_TYPECHECK(set1);
	__CYX_HASHSET_TYPECHECK(set2);
	assert(set1 && set2);
	__CyxHashSetHeader* head1 = __CYX_HASH_SET_GET_HEADER(set1);
	__CyxHashSetHeader* head2 = __CYX_HASH_SET_GET_HEADER(set2);
	assert(head1->size == head2->size && head1->hash_fn == head2->hash_fn && head1->eq_fn == head2->eq_fn);

	void* set_res = __cyx_hashset_copy(set1);
	cyx_hashset_foreach(a, set1) {
		if (!__cyx_hashset_contains_params(.__set = set2, .__val = (void*)a, .defer = 0)) {
			__cyx_hashset_remove(set_res, (void*)a);
		}
	}

	return set_res;
}
void* __cyx_hashset_intersec_self(void* self, const void* other) {
	__CYX_HASHSET_TYPECHECK(self);
	__CYX_HASHSET_TYPECHECK(other);

	__CyxHashSetHeader* head1 = __CYX_HASH_SET_GET_HEADER(self);
	__CyxHashSetHeader* head2 = __CYX_HASH_SET_GET_HEADER(other);
	assert(head1->size == head2->size && head1->hash_fn == head2->hash_fn && head1->eq_fn == head2->eq_fn);

	cyx_hashset_foreach(a, other) {
		if (!__cyx_hashset_contains_params(.__set = other, .__val = (void*)a, .defer = 0)) {
			__cyx_hashset_remove(self, (void*)a);
		}
	}

	return self;
}
void* __cyx_hashset_diff(const void* set1, const void* set2) {
	__CYX_HASHSET_TYPECHECK(set1);
	__CYX_HASHSET_TYPECHECK(set2);

	__CyxHashSetHeader* head1 = __CYX_HASH_SET_GET_HEADER(set1);
	__CyxHashSetHeader* head2 = __CYX_HASH_SET_GET_HEADER(set2);
	assert(head1->size == head2->size && head1->hash_fn == head2->hash_fn && head1->eq_fn == head2->eq_fn);

	void* set_res = __cyx_hashset_copy(set1);
	cyx_hashset_foreach(a, set2) { __cyx_hashset_remove(set_res, (void*)a); }

	return set_res;
}
void* __cyx_hashset_diff_self(void* self, const void* other) {
	__CYX_HASHSET_TYPECHECK(self);
	__CYX_HASHSET_TYPECHECK(other);

	__CyxHashSetHeader* head1 = __CYX_HASH_SET_GET_HEADER(self);
	__CyxHashSetHeader* head2 = __CYX_HASH_SET_GET_HEADER(other);
	assert(head1->size == head2->size && head1->hash_fn == head2->hash_fn && head1->eq_fn == head2->eq_fn);

	cyx_hashset_foreach(a, other) { __cyx_hashset_remove(self, (void*)a); }

	return self;
}

#endif // CYLIBX_IMPLEMENTATION

#endif // __CYX_CLOSE_FOLD

/*
 * HashMap
 */

#if __CYX_CLOSE_FOLD

typedef struct {
	size_t len;
	size_t cap;
	size_t size_key;
	size_t size_value;
	size_t size;
	
	size_t (*hash_fn)(const void*);
	int (*eq_fn)(const void*, const void*);
	void (*defer_key_fn)(void*);
	void (*defer_value_fn)(void*);
	void (*print_key_fn)(const void*);
	void (*print_value_fn)(const void*);

#ifdef CYLIBX_ALLOC
	EvoAllocator* alloc;
#endif // CYLIBX_ALLOC

	char is_key_ptr;
	char is_value_ptr;
} __CyxHashMapHeader;

struct __CyxHashMapParams {
#ifdef CYLIBX_ALLOC
	EvoAllocator* __allocator;
#endif // CYLIBX_ALLOC

	size_t __size_key;
	size_t __size_value;
	
	char is_key_ptr;
	char is_value_ptr;

	size_t (*__hash_fn)(const void* const);
	int (*__eq_fn)(const void* const, const void* const);
	void (*defer_key_fn)(void*);
	void (*defer_value_fn)(void*);
	void (*print_key_fn)(const void* const);
	void (*print_value_fn)(const void* const);
};
struct __CyxHashMapFuncParams {
	void* __map;
	void* __key;
	char defer;
};

#ifndef CYX_HASHMAP_BASE_SIZE
#define CYX_HASHMAP_BASE_SIZE 32
#endif // CYX_HASHMAP_BASE_SIZE

#define __CYX_HASHMAP_HEADER_SIZE (sizeof(__CyxHashMapHeader))
#define __CYX_HASHMAP_GET_HEADER(map) ((__CyxHashMapHeader*)__CYX_GET_TYPE(map) - 1)
#define __CYX_HASHMAP_GET_BITMAP(map) ((size_t*)((char*)(map) + __CYX_HASHMAP_GET_HEADER(map)->size * __CYX_HASHMAP_GET_HEADER(map)->cap + \
			__CYX_BITMAP_HEADER_SIZE + __CYX_TYPE_SIZE))
#define __CYX_HASHMAP_TYPECHECK(map) assert(*__CYX_GET_TYPE(map) == __CYX_TYPE_HASHMAP && \
		"ERROR: Type missmatch! Type of the provided pointer is not hashmap but was expected to be of type hashmap!")

void* __cyx_hashmap_new(struct __CyxHashMapParams params);
void __cyx_hashmap_expand(void** map_ptr);
void __cyx_hashmap_add(void** map_ptr, void* key);
void __cyx_hashmap_add_v(void** map_ptr, void* key, void* val);
void* __cyx_hashmap_get(struct __CyxHashMapFuncParams params);
void* __cyx_hashmap_remove(struct __CyxHashMapFuncParams params);
void cyx_hashmap_free(void* map);
void cyx_hashmap_print(const void* map);

#define cyx_hashmap_length(map) (__CYX_HASHMAP_GET_HEADER(map)->len)
#define cyx_hashmap_foreach(val, map) __CYX_HASHMAP_TYPECHECK(map); \
	size_t __CYX_UNIQUE_VAL__(i) = 0; \
	for (typeof(*map)* val = (void*)(map); \
		__CYX_UNIQUE_VAL__(i) < __CYX_HASHMAP_GET_HEADER(map)->cap; \
		++__CYX_UNIQUE_VAL__(i), \
		val = (void*)((char*)val + __CYX_HASHMAP_GET_HEADER(map)->size)) \
		if (cyx_bitmap_get(__CYX_HASHMAP_GET_BITMAP(map), 2 * __CYX_UNIQUE_VAL__(i)) && \
			!cyx_bitmap_get(__CYX_HASHMAP_GET_BITMAP(map), 2 * __CYX_UNIQUE_VAL__(i) + 1))

#define __cyx_hashmap_new_params(...) __cyx_hashmap_new((struct __CyxHashMapParams){ __VA_ARGS__ })
#ifndef CYLIBX_ALLOC // hashmap_new
#define cyx_hashmap_new(T, hash, eq, ...) (T*)__cyx_hashmap_new_params(.__size_key = sizeof(typeof(((T*)0)->key)), .__size_value = sizeof(typeof(((T*)0)->value)), .__hash_fn = hash, .__eq_fn = eq, __VA_ARGS__)
#else
#define cyx_hashmap_new(T, allocator, hash, eq, ...) (T*)__cyx_hashmap_new_params(.__allocator = (allocator), .__size_key = sizeof(typeof(((T*)0)->key)), .__size_value = sizeof(typeof(((T*)0)->value)), .__hash_fn = hash, .__eq_fn = eq, __VA_ARGS__)
#endif // CYLIBX_ALLOC
#define cyx_hashmap_add(map, k) do { \
	typeof(map->key) key = k; \
	__cyx_hashmap_add((void**)&(map), &key); \
} while(0)
#define cyx_hashmap_add_v(map, k, v) do { \
	typeof(map->key) key = k; \
	typeof(map->value) val = v; \
	__cyx_hashmap_add_v((void**)&(map), &key, &val); \
} while(0)
#define __cyx_hashmap_get_params(...) __cyx_hashmap_get((struct __CyxHashMapFuncParams){ __VA_ARGS__ })
#define cyx_hashmap_get(map, k, ...) ({ \
	typeof((map)->key) key = k; \
	(typeof((map)->value)*) __cyx_hashmap_get_params( .__map = map, .__key = &key, __VA_ARGS__ ); \
})
#define __cyx_hashmap_remove_params(...) __cyx_hashmap_remove((struct __CyxHashMapFuncParams){ __VA_ARGS__ })
#define cyx_hashmap_remove(map, k, ...) ({ \
	typeof((map)->key) key = k; \
	(typeof((map)->value)*)__cyx_hashmap_remove_params( .__map = map, .__key = &key, __VA_ARGS__ ); \
})

#ifdef CYLIBX_STRIP_PREFIX

#define hashmap_length(map) cyx_hashmap_length(map)
#define hashmap_foreach(val, map) cyx_hashmap_foreach(val, map)

#ifndef CYLIBX_ALLOC // hashmap_new
#define hashmap_new(T, hash, eq, ...) cyx_hashmap_new(T, hash, eq, __VA_ARGS__)
#else
#define hashmap_new(T, allocator, hash, eq, ...) cyx_hashmap_new(T, allocator, hash, eq, __VA_ARGS__)
#endif // CYLIBX_ALLOC
#define hashmap_add(map, k) cyx_hashmap_add(map, k)
#define hashmap_add_v(map, k, v) cyx_hashmap_add_v(map, k, v)
#define hashmap_get(map, k, ...) cyx_hashmap_get(map, k, __VA_ARGS__)
#define hashmap_remove(map, k, ...) cyx_hashmap_remove(map, k, __VA_ARGS__)

#define hashmap_free cyx_hashmap_free
#define hashmap_print cyx_hashmap_print

#endif // CYLIBX_STRIP_PREFIX

#ifdef CYLIBX_IMPLEMENTATION

#ifndef CYLIBX_ALLOC // hashmap_new
void* __cyx_hashmap_new(struct __CyxHashMapParams params) {
	size_t alloc_size = __CYX_HASHMAP_HEADER_SIZE + __CYX_TYPE_SIZE +
			CYX_HASHMAP_BASE_SIZE * (params.__size_key + params.__size_value) +
			__CYX_BITMAP_HEADER_SIZE + __CYX_TYPE_SIZE + __CYX_BITMAP_CALC_SIZE(2 * CYX_HASHMAP_BASE_SIZE) * sizeof(size_t);
	
	__CyxHashMapHeader* head = malloc(alloc_size);
	memset(head, 0, alloc_size);

	head->len = 0;
	head->cap = CYX_HASHMAP_BASE_SIZE;
	head->size_key = params.__size_key;
	head->size_value = params.__size_value;
	head->size = params.__size_value + params.__size_key;
	
	head->is_key_ptr = params.is_key_ptr;
	head->is_value_ptr = params.is_value_ptr;

	head->hash_fn = params.__hash_fn;
	head->eq_fn = params.__eq_fn;
	head->defer_key_fn = params.defer_key_fn;
	head->defer_value_fn = params.defer_value_fn;
	head->print_key_fn = params.print_key_fn;
	head->print_value_fn = params.print_value_fn;
	
	enum __CyxDataType* type = (void*)(head + 1);
	*type = __CYX_TYPE_HASHMAP;

	void* map = (void*)(type + 2);
	size_t* bitmap = __CYX_HASHMAP_GET_BITMAP(map);
	*__CYX_GET_TYPE(bitmap) = __CYX_TYPE_BITMAP;
	__CYX_BITMAP_GET_HEADER(bitmap)->size = 2 * CYX_HASHMAP_BASE_SIZE;
	
	return map;
}
#else
void* __cyx_hashmap_new(struct __CyxHashMapParams params) {
	size_t alloc_size = __CYX_HASHMAP_HEADER_SIZE + __CYX_TYPE_SIZE +
			CYX_HASHMAP_BASE_SIZE * (params.__size_key + params.__size_value) +
			__CYX_BITMAP_HEADER_SIZE + __CYX_TYPE_SIZE + __CYX_BITMAP_CALC_SIZE(2 * CYX_HASHMAP_BASE_SIZE) * sizeof(size_t);
	
	if (!params.__allocator) { params.__allocator = &EVO_LIBC_ALLOCATOR; }
	__CyxHashMapHeader* head = evo_alloc_malloc(params.__allocator, alloc_size);
	memset(head, 0, alloc_size);

	head->len = 0;
	head->cap = CYX_HASHMAP_BASE_SIZE;
	head->size_key = params.__size_key;
	head->size_value = params.__size_value;
	head->size = params.__size_value + params.__size_key;
	
	head->is_key_ptr = params.is_key_ptr;
	head->is_value_ptr = params.is_value_ptr;

	head->hash_fn = params.__hash_fn;
	head->eq_fn = params.__eq_fn;
	head->defer_key_fn = params.defer_key_fn;
	head->defer_value_fn = params.defer_value_fn;
	head->print_key_fn = params.print_key_fn;
	head->print_value_fn = params.print_value_fn;

	head->alloc = params.__allocator;
	
	enum __CyxDataType* type = (void*)(head + 1);
	*type = __CYX_TYPE_HASHMAP;

	void* map = (void*)(type + 2);
	size_t* bitmap = __CYX_HASHMAP_GET_BITMAP(map);
	*__CYX_GET_TYPE(bitmap) = __CYX_TYPE_BITMAP;
	__CYX_BITMAP_GET_HEADER(bitmap)->size = 2 * CYX_HASHMAP_BASE_SIZE;
	
	return map;
}
#endif // CYLIBX_ALLOC
void __cyx_hashmap_expand(void** map_ptr) {
	void* map = *map_ptr;

	__CyxHashMapHeader* head = __CYX_HASHMAP_GET_HEADER(map);
	size_t* bitmap = __CYX_HASHMAP_GET_BITMAP(map);

	size_t alloc_size = __CYX_HASHMAP_HEADER_SIZE + __CYX_TYPE_SIZE +
			(2 * head->cap) * head->size +
			__CYX_BITMAP_HEADER_SIZE + __CYX_TYPE_SIZE + __CYX_BITMAP_CALC_SIZE(head->cap * 4) * sizeof(size_t);
#ifndef CYLIBX_ALLOC
	__CyxHashMapHeader* new_head = malloc(alloc_size);
#else
	__CyxHashMapHeader* new_head = evo_alloc_malloc(head->alloc, alloc_size);
#endif // CYLIBX_ALLOC

	memset(new_head, 0, alloc_size);
	memcpy(new_head, head, __CYX_HASHMAP_HEADER_SIZE);
	new_head->len = 0;
	new_head->cap <<= 1;

	enum __CyxDataType* type = (void*)(new_head + 1);
	*type = __CYX_TYPE_HASHMAP;
	void* new_map = (void*)(type + 2);

	size_t* new_bitmap = __CYX_HASHMAP_GET_BITMAP(new_map);
	*__CYX_GET_TYPE(new_bitmap) = __CYX_TYPE_BITMAP;
	__CYX_BITMAP_GET_HEADER(new_bitmap)->size = new_head->cap * 2;
	
	for (size_t i = 0; i < head->cap; ++i) {
		if (!cyx_bitmap_get(bitmap, 2 * i) || cyx_bitmap_get(bitmap, 2 * i + 1)) { continue; }

		void* key = (char*)map + i * head->size;
		size_t pos = head->hash_fn(!head->is_key_ptr ? key : *(void**)key) % new_head->cap;
		for (size_t j = 0; j < new_head->cap; ++j) {
			size_t probe = (pos + j * j) % new_head->cap;
			if (!cyx_bitmap_get(new_bitmap, 2 * probe)) {
				memcpy((char*)new_map + probe * head->size, key, head->size);
				cyx_bitmap_set(new_bitmap, 2 * probe, 1);
				++head->len;
				break;
			}
		}
	}

#ifndef CYLIBX_ALLOC
	free(head);
#else
	if (head->alloc->free_f) {
		evo_alloc_free(head->alloc, head);
	}
#endif // CYLIBX_ALLOC
	*map_ptr = new_map;
}
void __cyx_hashmap_add(void** map_ptr, void* key) {
	void* map = *map_ptr;
	__CYX_HASHMAP_TYPECHECK(map);

	__CyxHashMapHeader* head = __CYX_HASHMAP_GET_HEADER(map);
	if (head->len * 1000 >= head->cap * 650) {
		__cyx_hashmap_expand(map_ptr);
		map = *map_ptr;
		head = __CYX_HASHMAP_GET_HEADER(map);
	}

	assert(head->hash_fn && head->eq_fn);

	size_t* bitmap = __CYX_HASHMAP_GET_BITMAP(map);
	size_t pos = head->hash_fn(!head->is_key_ptr ? key : *(void**)key) % head->cap;
	for (size_t i = 0; i < head->cap; ++i) {
		size_t probe = (pos + i * i) % head->cap;
		if (!cyx_bitmap_get(bitmap, 2 * probe)) {
			memcpy((char*)map + probe * head->size, key, head->size_key);
			cyx_bitmap_set(bitmap, 2 * probe, 1);
			++head->len;
			return;
		} else if (cyx_bitmap_get(bitmap, 2 * probe + 1)) {
			memcpy((char*)map + probe * head->size, key, head->size_key);
			cyx_bitmap_set(bitmap, 2 * probe + 1, 0);
			++head->len;
			return;
		} else if (!head->is_key_ptr ? head->eq_fn((char*)map + probe * head->size, key) : head->eq_fn(*(void**)((char*)map + probe * head->size), *(void**)key)) {
			return;
		}
	}
}
void __cyx_hashmap_add_v(void** map_ptr, void* key, void* val) {
	void* map = *map_ptr;
	__CYX_HASHMAP_TYPECHECK(map);

	__CyxHashMapHeader* head = __CYX_HASHMAP_GET_HEADER(map);
	if (head->len * 1000 >= head->cap * 650) {
		__cyx_hashmap_expand(map_ptr);
		map = *map_ptr;
		head = __CYX_HASHMAP_GET_HEADER(map);
	}

	assert(head->hash_fn && head->eq_fn);

	size_t* bitmap = __CYX_HASHMAP_GET_BITMAP(map);
	size_t pos = head->hash_fn(!head->is_key_ptr ? key : *(void**)key) % head->cap;
	for (size_t i = 0; i < head->cap; ++i) {
		size_t probe = (pos + i * i) % head->cap;
		if (!cyx_bitmap_get(bitmap, 2 * probe)) {
			memcpy((char*)map + probe * head->size, key, head->size_key);
			memcpy((char*)map + probe * head->size + head->size_key, val, head->size_value);
			cyx_bitmap_set(bitmap, 2 * probe, 1);
			++head->len;
			return;
		} else if (cyx_bitmap_get(bitmap, 2 * probe + 1)) {
			memcpy((char*)map + probe * head->size, key, head->size_key);
			memcpy((char*)map + probe * head->size + head->size_key, val, head->size_value);
			cyx_bitmap_set(bitmap, 2 * probe + 1, 0);
			++head->len;
			return;
		} else if (!head->is_key_ptr ? head->eq_fn((char*)map + probe * head->size, key) : head->eq_fn(*(void**)((char*)map + probe * head->size), *(void**)key)) {
			memcpy((char*)map + probe * head->size + head->size_key, val, head->size_value);
			return;
		}

	}
}
void* __cyx_hashmap_get(struct __CyxHashMapFuncParams params) {
	__CYX_HASHMAP_TYPECHECK(params.__map);
	__CyxHashMapHeader* head = __CYX_HASHMAP_GET_HEADER(params.__map);
	assert(head->hash_fn && head->eq_fn);

	size_t* bitmap = __CYX_HASHMAP_GET_BITMAP(params.__map);
	size_t pos = head->hash_fn(!head->is_key_ptr ? params.__key : *(void**)params.__key) % head->cap;

	void* res = NULL;
	for (size_t i = 0; i < head->cap; ++i) {
		size_t probe = (pos + i * i) % head->cap;
		if (cyx_bitmap_get(bitmap, 2 * probe) && !cyx_bitmap_get(bitmap, 2 * probe + 1)) {
			if (!head->is_key_ptr ?
				 head->eq_fn((char*)params.__map + probe * head->size, params.__key) :
				 head->eq_fn(*(void**)((char*)params.__map + probe * head->size), *(void**)params.__key)) {
				res = (char*)params.__map + probe * head->size + head->size_key;
				break;
			} 
		} else {
			break;
		}
	}
	if (params.defer && head->defer_key_fn) {
		head->defer_key_fn(!head->is_key_ptr ? params.__key : *(void**)params.__key);
	}
	return res;
}
void* __cyx_hashmap_remove(struct __CyxHashMapFuncParams params) {
	__CYX_HASHMAP_TYPECHECK(params.__map);

	__CyxHashMapHeader* head = __CYX_HASHMAP_GET_HEADER(params.__map);
	assert(head->hash_fn && head->eq_fn);

	size_t* bitmap = __CYX_HASHMAP_GET_BITMAP(params.__map);
	size_t pos = head->hash_fn(!head->is_key_ptr ? params.__key : *(void**)params.__key) % head->cap;

	int res = -1;
	for (size_t i = 0; i < head->cap; ++i) {
		size_t probe = (pos + i * i) % head->cap;
		if (cyx_bitmap_get(bitmap, 2 * probe) && !cyx_bitmap_get(bitmap, 2 * probe + 1)) {
			if (!head->is_key_ptr ?
				 head->eq_fn((char*)params.__map + probe * head->size, params.__key) :
				 head->eq_fn(*(void**)((char*)params.__map + probe * head->size), *(void**)params.__key)) {
				res = (int)probe;
				break;
			} 
		} else {
			break;
		}
	}
	if (params.defer && head->defer_key_fn) {
		head->defer_key_fn(!head->is_key_ptr ? params.__key : *(void**)params.__key);
	}
	void* removed = NULL;
	if (res >= 0) {
		--head->len;
		cyx_bitmap_set(bitmap, 2 * res + 1, 1);
		void* key = (char*)params.__map + res * head->size;
		if (head->defer_key_fn) {
			head->defer_key_fn(!head->is_key_ptr ? key : *(void**)key);
		}
		void* value = (char*)key + head->size_key;
		if (head->defer_value_fn) {
			removed = __cyx_temp_malloc(&__cyx_temp_stack, head->size_value);
			memcpy(removed, value, head->size_value); // TODO: fix memory leak
		} else {
			removed = value;
		}
	}
	return removed;
}
void cyx_hashmap_free(void* map) {
	__CYX_HASHMAP_TYPECHECK(map);
	
	__CyxHashMapHeader* head = __CYX_HASHMAP_GET_HEADER(map);

	size_t* bitmap = __CYX_HASHMAP_GET_BITMAP(map);
	if (head->defer_key_fn) {
		if (!head->is_key_ptr) {
			for (size_t i = 0; i < head->cap; ++i) {
				if (cyx_bitmap_get(bitmap, 2 * i) && !cyx_bitmap_get(bitmap, 2 * i + 1)) {
					head->defer_key_fn((char*)map + i * head->size);
				}
			}
		} else {
			for (size_t i = 0; i < head->cap; ++i) {
				if (cyx_bitmap_get(bitmap, 2 * i) && !cyx_bitmap_get(bitmap, 2 * i + 1)) {
					head->defer_key_fn(*(void**)((char*)map + i * head->size));
				}
			}
		}
	}

	if (head->defer_value_fn) {
		if (!head->is_value_ptr) {
			for (size_t i = 0; i < head->cap; ++i) {
				if (cyx_bitmap_get(bitmap, 2 * i) && !cyx_bitmap_get(bitmap, 2 * i + 1)) {
					head->defer_value_fn((char*)map + i * head->size + head->size_key);
				}
			}
		} else {
			for (size_t i = 0; i < head->cap; ++i) {
				if (cyx_bitmap_get(bitmap, 2 * i) && !cyx_bitmap_get(bitmap, 2 * i + 1)) {
					void* val = *(void**)((char*)map + i * head->size + head->size_key);
					if (val) {
						head->defer_value_fn(val);
					}
				}
			}
		}
	}
#ifndef CYLIBX_ALLOC
	free(head);
#else
	if (head->alloc) {
		evo_alloc_free(head->alloc, head);
	} else {
		fprintf(stderr, "ERROR:\tThe allocator you've provided to the hashmap doesn't free single elements as you expected!\n");
	}
#endif // CYLIBX_ALLOC
}
void cyx_hashmap_print(const void* map) {
	__CYX_HASHMAP_TYPECHECK(map);

	__CyxHashMapHeader* head = __CYX_HASHMAP_GET_HEADER(map);
	assert(head->print_key_fn && head->print_value_fn);
	size_t* bitmap = __CYX_HASHMAP_GET_BITMAP(map);

	printf("{ ");
	char started = 0;
	for (size_t i = 0; i < head->cap; ++i) {
		if (!cyx_bitmap_get(bitmap, 2 * i) || cyx_bitmap_get(bitmap, 2 * i + 1)) { continue; }

		if (started) { printf(", "); } else { started = 1; }
		void* key = (char*)map + i * head->size;
		void* val = (char*)map + i * head->size + head->size_key;
		head->print_key_fn(!head->is_key_ptr ? key : *(void**)key);
		printf(" : ");
		head->print_value_fn(!head->is_value_ptr ? val : *(void**)val);
	}
	printf(" }");
}

#endif // CYLIBX_IMPLEMENTATION

#endif // __CYX_CLOSE_FOLD

/*
 * BinaryHeap
 */

#if __CYX_CLOSE_FOLD

typedef struct {
	size_t len;
	size_t cap;
	size_t size;

	int (*cmp_fn)(const void*, const void*);
	void (*defer_fn)(void*);
	void (*print_fn)(const void*);

#ifdef CYLIBX_ALLOC
	EvoAllocator* alloc;
#endif // CYLIBX_ALLOC

	char is_ptr;
} __CyxBinaryHeapHeader;

struct __CyxBinaryHeapParams {
#ifdef CYLIBX_ALLOC
	EvoAllocator* __allocator;
#endif // CYLIBX_ALLOC

	size_t __size;

	char is_ptr;

	int (*__cmp_fn)(const void*, const void*);
	void (*defer_fn)(void*);
	void (*print_fn)(const void*);
};
struct __CyxBinaryHeapSearchParams {
	void* __heap;
	void* __val;
	char defer;
};

#ifndef CYX_BINHEAP_BASE_SIZE
#define CYX_BINHEAP_BASE_SIZE 16
#endif // CYX_BINHEAP_BASE_SIZE

#define __CYX_BINHEAP_HEADER_SIZE (sizeof(__CyxBinaryHeapHeader))
#define __CYX_BINHEAP_GET_HEADER(heap) ((__CyxBinaryHeapHeader*)__CYX_GET_TYPE(heap) - 1)
#define __CYX_BINHEAP_TYPECHECK(heap) assert(*__CYX_GET_TYPE(heap) == __CYX_TYPE_BINHEAP && \
		"ERROR: Type missmatch! Type of the provided pointer is not binary heap but was expected to be of type binary heap!")

void* __cyx_binheap_new(struct __CyxBinaryHeapParams params);
void __cyx_binheap_expand(void** heap_ptr, size_t n);
void __cyx_binheap_insert(void** heap_ptr, void* val);
void __cyx_binheap_insert_mult_n(void** heap_ptr, size_t n, void* mult);
void __cyx_binheap_try_fit(void* heap, size_t curr);
void* __cyx_binheap_extract(void* heap);
int __cyx_binheap_contains(struct __CyxBinaryHeapSearchParams params);
int __cyx_binheap_remove(struct __CyxBinaryHeapSearchParams params);
void cyx_binheap_free(void* heap);
void cyx_binheap_print(const void* heap);

#define cyx_binheap_length(heap) (__CYX_BINHEAP_GET_HEADER(heap)->len)
#define cyx_binheap_drain(val, heap) __CYX_BINHEAP_TYPECHECK(heap); \
	for (typeof(*heap)* val = NULL; (val = cyx_binheap_extract(heap));)

#define __cyx_binheap_new_params(...) (__cyx_binheap_new((struct __CyxBinaryHeapParams){ __VA_ARGS__ }))
#ifndef CYLIBX_ALLOC // binheap_new
#define cyx_binheap_new(T, cmp, ...) ((T*)__cyx_binheap_new_params(.__size = sizeof(T), .__cmp_fn = cmp, __VA_ARGS__))
#else
#define cyx_binheap_new(T, allocator, cmp, ...) ((T*)__cyx_binheap_new_params(.__allocator = (allocator), .__size = sizeof(T), .__cmp_fn = cmp, __VA_ARGS__))
#endif // CYLIBX_ALLOC
#define cyx_binheap_insert(heap, val) do { \
	typeof(*heap) v = val; \
	__cyx_binheap_insert((void**)&(heap), &v); \
} while(0)
#define cyx_binheap_insert_mult_n(heap, n, mult) (__cyx_binheap_insert_mult_n((void**)&(heap), n, mult))
#define cyx_binheap_insert_mult(heap, ...) do { \
	typeof(*heap) mult[] = { __VA_ARGS__ }; \
	__cyx_binheap_insert_mult_n((void**)&(heap), sizeof(mult)/sizeof(*mult), mult); \
} while(0)
#define cyx_binheap_extract(heap) ((typeof(*heap)*)__cyx_binheap_extract(heap))
#define __cyx_binheap_contains_params(...) (__cyx_binary_contains((struct __CyxBinaryHeapSearchParams){ __VA_ARGS__ }))
#define cyx_binheap_contains(heap, val, ...) ({ \
	typeof(*(heap)) v = (val); \
	__cyx_binary_contains_params(.__heap = (heap), .__val = &v, __VA_ARGS__); \
})
#define __cyx_binheap_remove_params(...) (__cyx_binheap_remove((struct __CyxBinaryHeapSearchParams){ __VA_ARGS__ }))
#define cyx_binheap_remove(heap, val, ...) ({ \
	typeof(*heap) v = (val); \
	__cyx_binheap_remove_params(.__heap = (heap), .__val = &v, __VA_ARGS__); \
})

#ifdef CYLIBX_STRIP_PREFIX

#define binheap_length(heap) cyx_binheap_length(heap)
#define binheap_drain(val, heap) cyx_binheap_drain(val, heap)

#ifndef CYLIBX_ALLOC
#define binheap_new(T, cmp, ...) cyx_binheap_new(T, cmp, __VA_ARGS__)
#else
#define binheap_new(T, allocator, cmp, ...) cyx_binheap_new(T, allocator, cmp, __VA_ARGS__)
#endif // CYLIBX_ALLOC
#define binheap_insert(heap, val) cyx_binheap_insert(heap, val)
#define binheap_insert_mult_n(heap, n, mult) cyx_binheap_insert_mult_n(heap, n, mult)
#define binheap_insert_mult(heap, ...) cyx_binheap_insert_mult(heap, __VA_ARGS__)
#define binheap_extract(heap) cyx_binheap_extract(heap)
#define binheap_contains(heap, val, ...) cyx_binheap_contains(heap, val, __VA_ARGS__)
#define binheap_remove(heap, val, ...) cyx_binheap_remove(heap, val, __VA_ARGS__)

#define binheap_free cyx_binheap_free
#define binheap_print cyx_binheap_print

#endif // CYLIBX_STRIP_RREFIX

#ifdef CYLIBX_IMPLEMENTATION

#define __cyx_get_left_child(n) (((n) << 1) + 1)
#define __cyx_get_right_child(n) (((n) << 1) + 2)
#define __cyx_get_parent(n) (((n) - 1) >> 1)

void* __cyx_binheap_new(struct __CyxBinaryHeapParams params) {
#ifndef CYLIBX_ALLOC
	__CyxBinaryHeapHeader* head = malloc(__CYX_BINHEAP_HEADER_SIZE + __CYX_TYPE_SIZE + params.__size * CYX_BINHEAP_BASE_SIZE);
#else
	if (!params.__allocator) { params.__allocator = &EVO_LIBC_ALLOCATOR; }
	__CyxBinaryHeapHeader* head = evo_alloc_malloc(params.__allocator, __CYX_BINHEAP_HEADER_SIZE + __CYX_TYPE_SIZE + params.__size * CYX_BINHEAP_BASE_SIZE);
#endif // CYLIBX_ALLOC

	if (!head) { return NULL; }

	enum __CyxDataType* type = (void*)(head + 1);
	*type = __CYX_TYPE_BINHEAP;
	void* heap = (void*)(type + 2);

	head->len = 0;
	head->cap = CYX_BINHEAP_BASE_SIZE;
	head->size = params.__size;
	head->is_ptr = params.is_ptr;

	head->cmp_fn = params.__cmp_fn;
	head->defer_fn = params.defer_fn;
	head->print_fn = params.print_fn;

#ifdef CYLIBX_ALLOC
	head->alloc = params.__allocator;
#endif // CYLIBX_ALLOC

	return heap;
}
void __cyx_binheap_expand(void** heap_ptr, size_t n) {
	void* heap = *heap_ptr;

	__CyxBinaryHeapHeader* head = __CYX_BINHEAP_GET_HEADER(heap);
	size_t new_cap = head->cap;
	while (new_cap < head->len + n) { new_cap <<= 1; }

#ifndef CYLIBX_ALLOC
	__CyxBinaryHeapHeader* new_head = malloc(__CYX_BINHEAP_HEADER_SIZE + new_cap * head->size);
#else
	__CyxBinaryHeapHeader* new_head = evo_alloc_malloc(head->alloc, __CYX_BINHEAP_HEADER_SIZE + new_cap * head->size);
#endif // CYLIBX_ALLOC

	enum __CyxDataType* type = (void*)(new_head + 1);
	*type = __CYX_TYPE_BINHEAP;

	new_head->len = head->len;
	new_head->cap = new_cap;
	new_head->size = head->size;
	new_head->is_ptr = head->is_ptr;

	new_head->cmp_fn = head->cmp_fn;
	new_head->defer_fn = head->defer_fn;
	new_head->print_fn = head->print_fn;

#ifdef CYLIBX_ALLOC
	new_head->alloc = head->alloc;
#endif // CYLIBX_ALLOC

	void* new_heap = (void*)(type + 2);
	memcpy(new_heap, heap, head->cap * head->size);
#ifndef CYLIBX_ALLOC
	free(head);
#else
	if (head->alloc->free_f) {
		evo_alloc_free(head->alloc, head);
	}
#endif // CYLIBX_ALLOC
	*heap_ptr = new_heap;
}
void __cyx_binheap_insert(void** heap_ptr, void* val) {
	void* heap = *heap_ptr;
	__CYX_BINHEAP_TYPECHECK(heap);

	__CyxBinaryHeapHeader* head = __CYX_BINHEAP_GET_HEADER(heap);
	if (head->cap < head->len + 1) {
		__cyx_binheap_expand(heap_ptr, 1);
		heap = *heap_ptr;
		head = __CYX_BINHEAP_GET_HEADER(heap);
	}

	size_t pos = head->len++;
	size_t parent_pos = __cyx_get_parent(pos);;
	while (pos) {
		if (__CYX_PTR_CMP(head, val, (char*)heap + parent_pos * head->size) != -1) { break; }
		memcpy(__CYX_DATA_GET_AT(head, heap, pos), __CYX_DATA_GET_AT(head, heap, parent_pos), head->size);
		pos = parent_pos;
		parent_pos = __cyx_get_parent(pos);
	}
	memcpy(__CYX_DATA_GET_AT(head, heap, pos), val, head->size);
}
void __cyx_binheap_insert_mult_n(void** heap_ptr, size_t n, void* mult) {
	void* heap = *heap_ptr;
	__CYX_BINHEAP_TYPECHECK(heap);

	__CyxBinaryHeapHeader* head = __CYX_BINHEAP_GET_HEADER(heap);
	if (head->cap < n + head->len) {
		__cyx_binheap_expand(heap_ptr, n);
		heap = *heap_ptr;
		head = __CYX_BINHEAP_GET_HEADER(heap);
	}

	for (size_t i = 0; i < n; ++i) {
		__cyx_binheap_insert(heap_ptr, __CYX_DATA_GET_AT(head, mult, i));
	}
}
void __cyx_binheap_try_fit(void* heap, size_t curr) {
	__CYX_BINHEAP_TYPECHECK(heap);

	__CyxBinaryHeapHeader* head = __CYX_BINHEAP_GET_HEADER(heap);
	while (curr < head->len) {
		size_t left = __cyx_get_left_child(curr);
		if (left >= head->len) { break; }
		size_t right = __cyx_get_right_child(curr);
		if (right < head->len) {
			if (__CYX_PTR_CMP_POS(head, heap, left, right) == -1) {
				if (__CYX_PTR_CMP_POS(head, heap, curr, left) == 1) {
					__CYX_SWAP(__CYX_DATA_GET_AT(head, heap, curr), __CYX_DATA_GET_AT(head, heap, left), head->size);
					curr = left;
				} else { break; }
			} else {
				if (__CYX_PTR_CMP_POS(head, heap, curr, right) == 1) {
					__CYX_SWAP(__CYX_DATA_GET_AT(head, heap, curr), __CYX_DATA_GET_AT(head, heap, right), head->size);
					curr = right;
				} else { break; }
			} 
		} else {
			if (__CYX_PTR_CMP_POS(head, heap, curr, left) == 1) {
				__CYX_SWAP(__CYX_DATA_GET_AT(head, heap, curr), __CYX_DATA_GET_AT(head, heap, left), head->size);
				curr = left;
			} else { break; }
		}
	}
}
void* __cyx_binheap_extract(void* heap) {
	__CYX_BINHEAP_TYPECHECK(heap);

	__CyxBinaryHeapHeader* head = __CYX_BINHEAP_GET_HEADER(heap);
	if (!head->len) { return NULL; }

	--head->len;
	__CYX_SWAP(heap, __CYX_DATA_GET_AT(head, heap, head->len), head->size);
	__cyx_binheap_try_fit(heap, 0);

	return __CYX_DATA_GET_AT(head, heap, head->len);
}
int __cyx_binheap_contains(struct __CyxBinaryHeapSearchParams params) {
	__CYX_BINHEAP_TYPECHECK(params.__heap);

	__CyxBinaryHeapHeader* head = __CYX_BINHEAP_GET_HEADER(params.__heap);

	int res = 0;
	for (size_t i = 0; i < head->len; ++i) {
		if (__CYX_PTR_CMP(head, params.__val, __CYX_DATA_GET_AT(head, params.__heap, i)) == 0) {
			res = 1;
			break;
		}
	}
	if (params.defer && head->defer_fn) {
		head->defer_fn(!head->is_ptr ? params.__val : *(void**)params.__val);
	}
	return res;
}
int __cyx_binheap_remove(struct __CyxBinaryHeapSearchParams params) {
	__CYX_BINHEAP_TYPECHECK(params.__heap);

	__CyxBinaryHeapHeader* head = __CYX_BINHEAP_GET_HEADER(params.__heap);

	int res = -1;
	for (size_t i = 0; i < head->len; ++i) {
		if (__CYX_PTR_CMP(head, params.__val, __CYX_DATA_GET_AT(head, params.__heap, i)) == 0) {
			res = (int)i;
			break;
		}
	}

	if (head->defer_fn && params.defer) {
		head->defer_fn(!head->is_ptr ? params.__val : *(void**)params.__val);
	}
	if (res != -1) {
		if (head->defer_fn) {
			head->defer_fn(!head->is_ptr ? __CYX_DATA_GET_AT(head, params.__heap, res) : *(void**)__CYX_DATA_GET_AT(head, params.__heap, res));
		}
		--head->len;
		memcpy(__CYX_DATA_GET_AT(head, params.__heap, res), __CYX_DATA_GET_AT(head, params.__heap, head->len), head->size);
		__cyx_binheap_try_fit(params.__heap, res);
	}
	return res != -1;
}
void cyx_binheap_free(void* heap) {
	__CYX_BINHEAP_TYPECHECK(heap);

	__CyxBinaryHeapHeader* head = __CYX_BINHEAP_GET_HEADER(heap);

	if (head->defer_fn) {
		if (!head->is_ptr) {
			for (size_t i = 0; i < head->len; ++i) {
				head->defer_fn(__CYX_DATA_GET_AT(head, heap, i));
			} 
		} else {
			for (size_t i = 0; i < head->len; ++i) {
				head->defer_fn(*(void**)__CYX_DATA_GET_AT(head, heap, i));
			}
		}
	}


#ifndef CYLIBX_ALLOC
	free(head);
#else
	if (head->alloc->free_f) {
		free(head);
	} else {
		fprintf(stderr, "ERROR:\tThe allocator you've provided to the binary heap doesn't free single elements as you expected!\n");
	}
#endif // CYLIBX_ALLOC
}
void cyx_binheap_print(const void* heap) {
	__CYX_BINHEAP_TYPECHECK(heap);

	__CyxBinaryHeapHeader* head = __CYX_BINHEAP_GET_HEADER(heap);
	assert(head->print_fn);

	if (!head->len) { 
		printf("{ }");
		return;
	}
	size_t layer_count = 0;
	for (size_t pos = head->len; pos << 1; pos >>= 1) { ++layer_count; }

	size_t curr_pos = 0;
	printf("{ ");
	for (size_t i = 0; i < layer_count; ++i) {
		if (i) { printf(", { "); } else { printf("{ "); }
		for (size_t j = 0; j < (0b1ull << i); ++j) {
			if (curr_pos >= head->len) { break; }
			if (j) { printf(", "); }
			head->print_fn(!head->is_ptr ? (char*)heap + curr_pos * head->size : *(void**)((char*)heap + curr_pos * head->size));
			curr_pos++;
		}
		printf(" }");
	}
	printf(" }");
}

#undef __cyx_get_right_child
#undef __cyx_get_left_child
#undef __cyx_get_parent

#endif // CYLIBX_IMPLEMENTATION

#endif // __CYX_CLOSE_FOLD

/*
 * RingBuffer
 */

#if __CYX_CLOSE_FOLD

typedef struct {
	size_t len;
	size_t cap;
	size_t size;

	size_t start;
	size_t end;

	void (*defer_fn)(void*);
	void (*print_fn)(const void*);

#ifdef CYLIBX_ALLOC
	EvoAllocator* alloc;
#endif // CYLIBX_ALLOC

	char is_ptr;
} __CyxRingBufHeader;

struct __CyxRingBufParams {
#ifdef CYLIBX_ALLOC
	EvoAllocator* __allocator;
#endif // CYLIBX_ALLOC

	size_t __size;

	char is_ptr;
	void (*defer_fn)(void*);
	void (*print_fn)(const void*);
};

#ifndef CYX_RINGBUF_BASE_SIZE
#define CYX_RINGBUF_BASE_SIZE (CYX_ARRAY_BASE_SIZE)
#endif // CYX_RINGBUF_BASE_SIZE

#define __CYX_RINGBUF_HEADER_SIZE (sizeof(__CyxRingBufHeader))
#define __CYX_RINGBUF_GET_HEADER(ring) ((__CyxRingBufHeader*)__CYX_GET_TYPE(ring) - 1)
#define __CYX_RINGBUF_TYPECHECK(ring) assert(*__CYX_GET_TYPE(ring) == __CYX_TYPE_RINGBUF && \
		"ERROR: Type missmatch! Type of the provided pointer is not ring buffer but was expected to be of type ring buffer!")

void* __cyx_ring_new(struct __CyxRingBufParams params);
void __cyx_ring_expand(void** ring_ptr, size_t n);
void __cyx_ring_push(void** ring_ptr, void* val);
void __cyx_ring_push(void** ring_ptr, void* val);
void __cyx_ring_push_mult_n(void** ring_ptr, size_t n, void** mult);
void* __cyx_ring_pop(void* ring);
void cyx_ring_free(void* ring);
void cyx_ring_print(const void* ring);

#define cyx_ring_length(ring) (__CYX_RINGBUF_GET_HEADER(ring)->len)
#define cyx_ring_foreach(val, ring)  __CYX_RINGBUF_TYPECHECK(ring); \
	__CyxRingBufHeader* __CYX_UNIQUE_VAL__(head) = __CYX_RINGBUF_GET_HEADER(ring); \
	char __CYX_UNIQUE_VAL__(started) = __CYX_UNIQUE_VAL__(head)->len != 0; \
	size_t __CYX_UNIQUE_VAL__(i) = __CYX_UNIQUE_VAL__(head)->start; \
	for (typeof(*ring)* val = __CYX_DATA_GET_AT(__CYX_UNIQUE_VAL__(head), ring, __CYX_UNIQUE_VAL__(head)->start);\
		__CYX_UNIQUE_VAL__(i) != __CYX_UNIQUE_VAL__(head)->end || __CYX_UNIQUE_VAL__(started); \
		__CYX_UNIQUE_VAL__(i) = (__CYX_UNIQUE_VAL__(i) + 1) % __CYX_UNIQUE_VAL__(head)->cap, \
		val = (char*)val + __CYX_UNIQUE_VAL__(head)->size, \
		__CYX_UNIQUE_VAL__(started) = 0)
#define cyx_ring_drain(val, ring) __CYX_RINGBUF_TYPECHECK(ring); \
	for(typeof(*ring)* val = NULL; (val = cyx_ring_pop(ring));)

#define __cyx_ring_new_params(...) (__cyx_ring_new((struct __CyxRingBufParams){ __VA_ARGS__ }))
#ifndef CYLIBX_ALLOC // ring_new
#define cyx_ring_new(T, ...) ((T*)__cyx_ring_new_params(.__size = sizeof(T), __VA_ARGS__))
#else
#define cyx_ring_new(T, allocator, ...) ((T*)__cyx_ring_new_params(.__allocator = (allocator), .__size = sizeof(T), __VA_ARGS__))
#endif // CYLIBX_ALLOC
#define cyx_ring_push(ring, val) do { \
	typeof(*ring) v = (val); \
	__cyx_ring_push((void**)&(ring), &v); \
} while(0)
#define cyx_ring_push_mult_n(ring, n, mult) (__cyx_ring_push_mult_n((void**)&(ring), n, mult))
#define cyx_ring_push_mult(ring, ...) do { \
	typeof(*ring) mult[] = { __VA_ARGS__ }; \
	cyx_ring_push_mult_n(ring, sizeof(*mult)/sizeof(mult), mult); \
} while (0)
#define cyx_ring_pop(ring) ((typeof(*ring)*)__cyx_ring_pop(ring))

#ifdef CYLIBX_STRIP_PREFIX

#define ring_length(ring) cyx_ring_length(ring)
#define ring_foreach(val, ring) cyx_ring_foreach(val, ring)
#define ring_drain(val, ring) cyx_ring_drain(val, ring)

#ifndef CYLIBX_ALLOC
#define ring_new(T, ...) cyx_ring_new(T, __VA_ARGS__)
#else
#define ring_new(T, allocator, ...) cyx_ring_new(T, allocator, __VA_ARGS__)
#endif // CYLIBX_ALLOC
#define ring_push(ring, val) cyx_ring_push(ring, val)
#define ring_push_mult_n(ring, n, mult) cyx_ring_push_mult_n(ring, n, mult)
#define ring_push_mult(ring, ...) cyx_ring_push_mult(ring, __VA_ARGS__)
#define ring_pop(ring) cyx_ring_pop(ring)

#define ring_free cyx_ring_free
#define ring_print cyx_ring_print

#endif // CYLIBX_STRIP_PREFIX

#ifdef CYLIBX_IMPLEMENTATION 

void* __cyx_ring_new(struct __CyxRingBufParams params) {
#ifndef CYLIBX_ALLOC
	__CyxRingBufHeader* head = malloc(__CYX_RINGBUF_HEADER_SIZE + __CYX_TYPE_SIZE + params.__size * CYX_RINGBUF_BASE_SIZE);
#else
	if (!params.__allocator) { params.__allocator = &EVO_LIBC_ALLOCATOR; }
	__CyxRingBufHeader* head = evo_alloc_malloc(params.__allocator, __CYX_RINGBUF_HEADER_SIZE + __CYX_TYPE_SIZE + params.__size * CYX_RINGBUF_BASE_SIZE);
#endif // CYLIBX_ALLOC

	memset(head, 0, __CYX_RINGBUF_HEADER_SIZE + params.__size * CYX_RINGBUF_BASE_SIZE);
	head->size = params.__size;
	head->cap = CYX_RINGBUF_BASE_SIZE;
	head->len = 0;
#ifdef CYLIBX_ALLOC
	head->alloc = params.__allocator;
#endif // CYLIBX_ALLOC

	head->start = 0;
	head->end = 0;

	head->is_ptr = params.is_ptr;
	head->defer_fn = params.defer_fn;
	head->print_fn = params.print_fn;

	enum __CyxDataType* type = (void*)(head + 1);
	*type = __CYX_TYPE_RINGBUF;

	return (void*)(type + 2);
}
void __cyx_ring_expand(void** ring_ptr, size_t n) {
	__CyxRingBufHeader* head = __CYX_RINGBUF_GET_HEADER(*ring_ptr);

	size_t new_cap = head->cap;
	while ((new_cap <<= 1) <= head->len + n);

#ifndef CYLIBX_ALLOC
	__CyxRingBufHeader* new_head = malloc(__CYX_RINGBUF_HEADER_SIZE + __CYX_TYPE_SIZE + new_cap * head->size);
#else
	__CyxRingBufHeader* new_head = evo_alloc_malloc(head->alloc, __CYX_RINGBUF_HEADER_SIZE + __CYX_TYPE_SIZE + new_cap * head->size);
#endif // CYLIBX_ALLOC
	enum __CyxDataType* type = (void*)(new_head + 1);
	*type = __CYX_TYPE_RINGBUF;
	void* new_ring = (void*)(type + 2);

	memcpy(new_head, head, __CYX_RINGBUF_HEADER_SIZE);
	new_head->cap = new_cap;
	new_head->start = 0;
	new_head->end = 0;

	cyx_ring_foreach(val, *ring_ptr) {
		memcpy(__CYX_DATA_GET_AT(new_head, new_ring, new_head->len), val, head->size);
		new_head->len++;
		new_head->end++;
	}

#ifndef CYLIBX_ALLOC
	free(head);
#else
	if (head->alloc->free_f) {
		evo_alloc_free(head->alloc, head);
	}
#endif // CYLIBX_ALLOC
	*ring_ptr = new_ring;
}
void __cyx_ring_push(void** ring_ptr, void* val) {
	__CYX_RINGBUF_TYPECHECK(*ring_ptr);

	__CyxRingBufHeader* head = __CYX_RINGBUF_GET_HEADER(*ring_ptr);
	if (head->len == head->cap) {
		__cyx_ring_expand(ring_ptr, 1);
		head = __CYX_RINGBUF_GET_HEADER(*ring_ptr);
	}

	if (head->len == 0) { head->end = 0; head->start = 0; }
	memcpy(__CYX_DATA_GET_AT(head, *ring_ptr, head->end++), val, head->size);
	if (head->end == head->cap) { head->end = 0; }

	head->len++;
}
void __cyx_ring_push_mult_n(void** ring_ptr, size_t n, void** mult) {
	__CYX_RINGBUF_TYPECHECK(*ring_ptr);

	__CyxRingBufHeader* head = __CYX_RINGBUF_GET_HEADER(*ring_ptr);
	if (head->len + n >= head->cap) {
		__cyx_ring_expand(ring_ptr, n);
		head = __CYX_RINGBUF_GET_HEADER(*ring_ptr);
	}

	for (size_t i = 0; i < n; ++i) {
		if (head->len == 0) { head->end = 0; head->start = 0; }
		memcpy(__CYX_DATA_GET_AT(head, *ring_ptr, head->end), __CYX_DATA_GET_AT(head, mult, head->end), head->size);
		head->end++;
		if (head->end == head->cap) { head->end = 0; }

		head->len++;
	}
}
void* __cyx_ring_pop(void* ring) {
	__CYX_RINGBUF_TYPECHECK(ring);

	__CyxRingBufHeader* head = __CYX_RINGBUF_GET_HEADER(ring);
	if (!head->len) { return NULL; }
	void* ret = NULL;
	if (head->defer_fn) {
		ret = __cyx_temp_malloc(&__cyx_temp_stack, head->size); // TODO: memory leak
		memcpy(ret, __CYX_DATA_GET_AT(head, ring, head->start), head->size);
	} else {
		ret = __CYX_DATA_GET_AT(head, ring, head->start);
	}
	++head->start;
	--head->len;
	if (head->start == head->cap) { head->start = 0; }

	return ret;
}
void cyx_ring_free(void* ring) {
	__CYX_RINGBUF_TYPECHECK(ring);

	__CyxRingBufHeader* head = __CYX_RINGBUF_GET_HEADER(ring);
	if (head->defer_fn) {
		if (!head->is_ptr) {
			cyx_ring_foreach(val, ring) {
				head->defer_fn(val);
			}
		} else {
			cyx_ring_foreach(val, ring) {
				head->defer_fn(*(void**)val);
			}
		}
	}
#ifndef CYLIBX_ALLOC
	free(head);
#else
	if (head->alloc->free_f) {
		evo_alloc_free(head->alloc, head);
	} else {
		fprintf(stderr, "ERROR:\tThe allocator you've provided to the ring buffer doesn't free single elements as you expected!\n");
	}
#endif // CYLIBX_ALLOC
}
void cyx_ring_print(const void* ring) {
	__CYX_RINGBUF_TYPECHECK(ring);

	__CyxRingBufHeader* head = __CYX_RINGBUF_GET_HEADER(ring);
	printf("[ ");
	char started = 0;
	cyx_ring_foreach(val, ring) {
		if (started) { printf(", "); } else { started = 1; }
		head->print_fn(!head->is_ptr ? val : *(void**)val);
	}
	printf(" ]");
}

#endif // CYLIBX_IMPLEMENTATION

#endif // __CYX_CLOSE_FOLD

#endif // __CYLIBX_H__
