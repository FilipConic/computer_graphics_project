#ifndef __EVOCO_H__
#define __EVOCO_H__

#include <stdint.h>
#include <assert.h>
#include <stddef.h>

// #define EVOCO_IMPLEMENTATION

#ifdef EVOCO_IMPLEMENTATION
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#endif // EVOCO_IMPLEMENTATION

/*
 * Evoco utils
 */

#define EVO_PTR_SIZE (sizeof(uintptr_t))
#define EVO_ALIGN8(n) (((n) - 1) / EVO_PTR_SIZE + 1)
#define EVO_ALIGN(n) (EVO_ALIGN8(n) * EVO_PTR_SIZE)
#define EVO_KB(n) (1024 * (n))
#define EVO_MB(n) (1024 * EVO_KB(n))
#define EVO_GB(n) (assert(0 && "No"), MB(n))

typedef void* (*evo_malloc_func)(void*, size_t);
typedef void* (*evo_calloc_func)(void*, size_t, size_t);
typedef void* (*evo_realloc_func)(void*, void*, size_t);
typedef void* (*evo_reallocarray_func)(void*, void*, size_t, size_t);
typedef void (*evo_destroy_func)(void*);
typedef void (*evo_free_func)(void*, void*);
typedef void (*evo_reset_func)(void*);

typedef struct {
	void* ctx;

	evo_malloc_func malloc_f;
	evo_calloc_func calloc_f;
	evo_realloc_func realloc_f;
	evo_reallocarray_func reallocarray_f;
	evo_destroy_func destroy_f;
	evo_free_func free_f;
	evo_reset_func reset_f;

	char heap_flag;
} EvoAllocator;

void evo_allocator_free(EvoAllocator* a);

#define __evo_alloc_error(a, func, func_name) (assert((a)->func && \
	"ASSERT: Trying to [" func_name "] for an allocator that doesn't implement [" func_name "] functionallity!"))
#define evo_alloc_malloc(a, size) (__evo_alloc_error(a, malloc_f, "malloc"), \
	(a)->malloc_f((a)->ctx, size))
#define evo_alloc_calloc(a, n, size) (__evo_alloc_error(a, calloc_f, "calloc"), \
	(a)->calloc_f((a)->ctx, (n), (size)))
#define evo_alloc_realloc(a, ptr, size) (__evo_alloc_error(a, realloc_f, "realloc"), \
	(a)->realloc_f((a)->ctx, (ptr), (size)))
#define evo_alloc_reallocarray(a, ptr, n, size) (__evo_alloc_error(a, reallocarray_f, "reallocarray"), \
	(a)->reallocarray_f((a)->ctx, (ptr), (n), (size)))
#define evo_alloc_destroy(a) (__evo_alloc_error(a, destroy_f, "destroy"), \
	(a)->destroy_f((a)->ctx))
#define evo_alloc_free(a, ptr) (__evo_alloc_error(a, free_f, "free"), \
	(a)->free_f((a)->ctx, (ptr)))
#define evo_alloc_reset(a) (__evo_alloc_error(a, reset_f, "reset"), \
	(a)->reset_f((a)->ctx))

#ifdef EVOCO_IMPLEMENTATION
void evo_allocator_free(EvoAllocator* a) {
	if (a->ctx) {
		if (a->destroy_f) {
			a->destroy_f(a->ctx);
		}
		if (a->heap_flag) {
			free(a->ctx);
		}
	}
}
#endif // EVOCO_IMPLEMENTATION

/*
 * Basic libc allocator functionallity
 */ 

void* evo_libc_malloc(void* v, size_t size);
void* evo_libc_calloc(void* v, size_t n, size_t size);
void* evo_libc_realloc(void* v, void* ptr, size_t size);
void* evo_libc_reallocarray(void* v, void* ptr, size_t n, size_t size);
void evo_libc_free(void* v, void* ptr);

#ifdef EVOCO_IMPLEMENTATION

void* evo_libc_malloc(void* v, size_t size) {
	(void)v;
	return malloc(size);
}
void* evo_libc_calloc(void* v, size_t n, size_t size) {
	(void)v;
	return calloc(n, size);
}
void* evo_libc_realloc(void* v, void* ptr, size_t size) {
	(void)v;
	return realloc(ptr, size);
}
void* evo_libc_reallocarray(void* v, void* ptr, size_t n, size_t size) {
	(void)v;
	return reallocarray(ptr, n, size);
}
void evo_libc_free(void* v, void* ptr) {
	(void)v;
	free(ptr);
}
#endif // EVOCO_IMPLEMENTATION

/*
 * Arena Allocator
 */

typedef struct EvoArena EvoArena;
struct EvoArena {
	void* buffer;
	void* current;
	size_t capacity;
	size_t filled;

	EvoArena* next;
};

EvoArena evo_arena_new(size_t base_cap);
void* evo_arena_malloc(void* v, size_t size);
void* evo_arena_calloc(void* v, size_t n, size_t size);
void* evo_arena_realloc(void* v, void* ptr, size_t size);
void* evo_arena_reallocarray(void* v, void* ptr, size_t n, size_t size);
void evo_arena_destroy(void* v);
void evo_arena_reset(void* v);
size_t evo_arena_size(EvoArena* v);
EvoAllocator evo_allocator_arena(EvoArena* a);
EvoAllocator evo_allocator_arena_heap(size_t base_cap);

#ifdef EVOCO_IMPLEMENTATION
EvoArena evo_arena_new(size_t base_cap) {
	if (!base_cap) {
		fprintf(stderr, "ERROR: Can't create an arena with the capacity of 0!\n");
		return (EvoArena){ 0 };
	}
	EvoArena a = {
		.buffer = malloc(base_cap),
		.capacity = base_cap,
		.filled = 0,
	};
	assert(a.buffer && "Buy more RAM");
	a.current = a.buffer;
	return a;
}
void __evo_arena_expand(EvoArena* a, size_t size) {
	a->next = (EvoArena*)malloc(sizeof(*a));
	assert(a->next && "Buy more RAM");
	size_t new_cap = a->capacity;
	while ((new_cap <<= 1) < size);
	*(a->next) = evo_arena_new(new_cap);
}
EvoArena* __evo_arena_get_last(EvoArena* a) {
	if (!a->next) {
		return a;
	}
	return __evo_arena_get_last(a->next);
}
void* evo_arena_malloc(void* v, size_t size) {
	if (!size) { return NULL; }

	size_t aligned = EVO_ALIGN8(size);

	EvoArena* a = __evo_arena_get_last((EvoArena*)v);
	if (a->filled + aligned * EVO_PTR_SIZE > a->capacity) {
		__evo_arena_expand(a, aligned * EVO_PTR_SIZE);
		assert(a->next);
		a = a->next;
	}

	void* ret = a->current;
	a->current = (uintptr_t*)a->current + aligned;
	a->filled += aligned * EVO_PTR_SIZE;

	return ret;
}
void* evo_arena_calloc(void* v, size_t n, size_t size) {
	void* ret = evo_arena_malloc(v, n * size);
	memset(ret, 0, n * size);
	return ret;
}
void* evo_arena_realloc(void* v, void* ptr, size_t size) {
	void* ret = evo_arena_malloc(v, size);
	memcpy(ret, ptr, size);
	return ret;
}
void* evo_arena_reallocarray(void* v, void* ptr, size_t n, size_t size) {
	void* ret = evo_arena_malloc(v, n * size);
	memcpy(ret, ptr, size * n);
	return ret;
}
void evo_arena_destroy(void* v) {
	EvoArena* a = (EvoArena*)v;

	if (a->buffer) { free(a->buffer); }
	a->buffer = NULL;
	a->current = NULL;
	a->capacity = 0;
	a->filled = 0;

	if (a->next) {
		evo_arena_destroy(a->next);
		free(a->next);
	}
	a->next = NULL;
}
void evo_arena_reset(void* v) {
	EvoArena* a = (EvoArena*)v;
	a->current = a->buffer;
	a->filled = 0;
	if (a->next) {
		evo_arena_destroy(a->next);
	}
}
size_t evo_arena_size(EvoArena* a) {
	if (a->next) {
		return a->filled + evo_arena_size(a->next);
	}
	return a->filled;
}

EvoAllocator evo_allocator_arena(EvoArena* a) {
	return (EvoAllocator) {
		.ctx = a,

		.malloc_f = evo_arena_malloc,
		.calloc_f = evo_arena_calloc,
		.realloc_f = evo_arena_realloc,
		.reallocarray_f = evo_arena_reallocarray,
		.destroy_f = evo_arena_destroy,
		.free_f = NULL,
		.reset_f = evo_arena_reset,

		.heap_flag = 0,
	};
}
EvoAllocator evo_allocator_arena_heap(size_t base_cap) {
	EvoAllocator ret = (EvoAllocator) {
		.ctx = malloc(sizeof(EvoArena)),

		.malloc_f = evo_arena_malloc,
		.calloc_f = evo_arena_calloc,
		.realloc_f = evo_arena_realloc,
		.reallocarray_f = evo_arena_reallocarray,
		.destroy_f = evo_arena_destroy,
		.free_f = NULL,
		.reset_f = evo_arena_reset,

		.heap_flag = 1,
	};
	EvoArena a = evo_arena_new(base_cap);
	memcpy(ret.ctx, &a, sizeof(a));
	return ret;
}
#endif // EVOCO_IMPLEMENTATION

// Generic Pool Allocator

#ifndef EVO_POOL_BASE_CAPACITY
#define EVO_POOL_BASE_CAPACITY 128
#endif // EVO_POOL_BASE_CAPACITY

#ifndef EVO_POOL_FREE_LIST_CAPACITY
#define EVO_POOL_FREE_LIST_CAPACITY (EVO_POOL_BASE_CAPACITY - (EVO_PTR_SIZE + sizeof(size_t)) / sizeof(int))
#endif // POOL_FREE_LIST_CAPACITY

struct __EvoPoolFreeList {
	struct __EvoPoolFreeList* next;
	int empty[EVO_POOL_FREE_LIST_CAPACITY];
	size_t len;
};

#ifdef EVOCO_IMPLEMENTATION
struct __EvoPoolFreeList* __evo_pool_free_list_push(struct __EvoPoolFreeList** ptr) {
	if (!*ptr) {
		*ptr = (struct __EvoPoolFreeList*)malloc(sizeof(**ptr));
		**ptr = (struct __EvoPoolFreeList){ 0 };
		return *ptr;
	} else {
		return __evo_pool_free_list_push(&(*ptr)->next);
	}
}
int __evo_pool_free_list_find(struct __EvoPoolFreeList* node, int pos) {
	if (!node) { return 0; }
	for (size_t i = 0; i < node->len; ++i) {
		if (node->empty[i] == pos) {
			return 1;
		}
	}
	return __evo_pool_free_list_find(node->next, pos);
}
void __evo_pool_free_list_add(struct __EvoPoolFreeList** ptr, int pos) {
	struct __EvoPoolFreeList* node = *ptr;
	if (node == NULL || node->len == EVO_POOL_FREE_LIST_CAPACITY) {
		node = __evo_pool_free_list_push(ptr);
	}
	node->empty[node->len++] = pos;
}
void __evo_pool_free_list_pop(struct __EvoPoolFreeList** ptr) {
	struct __EvoPoolFreeList* next = (*ptr)->next;
	free(*ptr);
	*ptr = next;
}
#endif // EVOCO_IMPLEMENTATION

typedef struct EvoPool EvoPool;
struct EvoPool {
	void* buffer;
	void* current;

	size_t filled;
	size_t size;
	size_t capacity;

	EvoPool* next;
	struct __EvoPoolFreeList* free_list;
};

EvoPool evo_pool_new(size_t size);
void* evo_pool_malloc(void* v, size_t size);
void evo_pool_destroy(void* v);
void evo_pool_free(void* v, void* ptr);
void evo_pool_reset(void* v);
EvoAllocator evo_allocator_pool(EvoPool* pool);
EvoAllocator evo_allocator_pool_heap(size_t size);

#ifdef EVOCO_IMPLEMENTATION
void __evo_pool_expand(EvoPool* v) {
	assert(!v->next);
	EvoPool next = {
		.buffer = malloc((v->capacity << 1) * v->size * EVO_PTR_SIZE),
		
		.filled = 0,
		.size = v->size,
		.capacity = v->capacity << 1,

		.next = NULL,
		.free_list = NULL,
	};
	next.current = next.buffer;
	v->next = (EvoPool*)malloc(sizeof(*v));
	memcpy(v->next, &next, sizeof(*v));
}
EvoPool evo_pool_new(size_t size) {
	EvoPool pool = {
		.buffer = malloc(size * EVO_POOL_BASE_CAPACITY),

		.filled = 0,
		.size = EVO_ALIGN8(size),
		.capacity = EVO_POOL_BASE_CAPACITY,

		.next = NULL,
		.free_list = NULL,
	};
	pool.current = pool.buffer;
	return pool;
}
void* evo_pool_malloc(void* v, size_t size) {
	(void)size;
	EvoPool* pool = (EvoPool*)v;
	if (!pool->capacity) {
		fprintf(stderr, "ERROR:\tTrying to allocate from a destroyed allocator!\n");
		return NULL;
	}
	
	if (pool->free_list) {
		struct __EvoPoolFreeList* list = pool->free_list;
		assert(list->len > 0);
		int pos = list->empty[--list->len];
		if (!list->len) {
			__evo_pool_free_list_pop(&pool->free_list);
		}
		return (uintptr_t*)pool->buffer + pos * pool->size;
	} else if (pool->filled == pool->capacity) {
		if (pool->next) {
			return evo_pool_malloc(pool->next, 0);
		} else {
			__evo_pool_expand(pool);
			assert(pool->next);
			pool = pool->next;

			void* current = pool->current;
			pool->current = (uintptr_t*)pool->current + pool->size;
			++pool->filled;
			return current;
		}
	} else {
		void* current = pool->current;
		pool->current = (uintptr_t*)pool->current + pool->size;
		++pool->filled;
		return current;
	}
}
void evo_pool_destroy(void* v) {
	EvoPool* pool = (EvoPool*)v;

	free(pool->buffer);
	pool->buffer = NULL;
	pool->current = NULL;

	pool->capacity = 0;
	pool->filled = 0;
	pool->size = 0;

	if (pool->next) {
		evo_pool_destroy(pool->next);
		free(pool->next);
		pool->next = NULL;
	}
	while (pool->free_list) { __evo_pool_free_list_pop(&pool->free_list); }
}
void evo_pool_free(void* v, void* ptr) {
	EvoPool* pool = (EvoPool*)v;
	uintptr_t ibuffer = (uintptr_t)pool->buffer;
	uintptr_t iptr = (uintptr_t)ptr;
	if (ibuffer <= iptr && iptr <= ibuffer + pool->filled * pool->size * EVO_PTR_SIZE) {
		uintptr_t pos = (iptr - ibuffer) / EVO_PTR_SIZE;
		if (__evo_pool_free_list_find(pool->free_list, pos)) {
			fprintf(stderr, "ERROR:\tDouble free detected in the memory pool allocator!\n");
			return;
		}
		__evo_pool_free_list_add(&pool->free_list, pos);
	} else if (pool->next) {
		evo_pool_free(pool->next, ptr);
	} else {
		fprintf(stderr, "ERROR:\tPointer provided to the memory pool free function doesn't belong to the provided memory pool!\n");
	}
}
void evo_pool_reset(void* v) {
	EvoPool* pool = (EvoPool*)v;

	pool->current = pool->buffer;
	pool->filled = 0;

	if (pool->next) {
		evo_pool_destroy(pool->next);
		free(pool->next);
		pool->next = NULL;
	}
	while (pool->free_list) { __evo_pool_free_list_pop(&pool->free_list); }
}

EvoAllocator evo_allocator_pool(EvoPool* pool) {
	return (EvoAllocator) {
		.ctx = (pool),

		.malloc_f = evo_pool_malloc,
		.calloc_f = NULL,
		.realloc_f = NULL,
		.reallocarray_f = NULL,
		.destroy_f = evo_pool_destroy,
		.free_f = evo_pool_free,
		.reset_f = evo_pool_reset,

		.heap_flag = 0,
	};
}
EvoAllocator evo_allocator_pool_heap(size_t size) {
	EvoPool* pool = (EvoPool*)malloc(sizeof(*pool));
	*pool = evo_pool_new(size);

	return (EvoAllocator) {
		.ctx = pool,

		.malloc_f = evo_pool_malloc,
		.calloc_f = NULL,
		.realloc_f = NULL,
		.reallocarray_f = NULL,
		.destroy_f = evo_pool_destroy,
		.free_f = evo_pool_free,
		.reset_f = evo_pool_reset,

		.heap_flag = 1,
	};
}
#endif // EVOCO_IMPLEMENTATION

/*
 * Temporary Stack Allocator
 */

/*
 * create a global variable or on the stack of a function
 * the library comes with the 32KB temporary stack created
 * you can change the base size of 32KB by #define EVO_TEMP_STACK_BASE_CAPACITY [value]
 */

#ifndef EVO_TEMP_STACK_BASE_CAPACITY
#define EVO_TEMP_STACK_BASE_CAPACITY 32
#endif // EVO_TEMP_STACK_BASE_CAPACITY

typedef struct {
	void* current;
	size_t filled;
	size_t mark;
	uint8_t buffer[EVO_KB(EVO_TEMP_STACK_BASE_CAPACITY)];
} EvoTempStack;

void* evo_temp_malloc(void* v, size_t size);
void* evo_temp_calloc(void* v, size_t n, size_t size);
void evo_temp_set_mark(EvoTempStack* temp);
void evo_temp_reset_mark(EvoTempStack* temp);
void evo_temp_reset(void* v);

EvoAllocator evo_allocator_temp(EvoTempStack* temp);

#ifdef EVOCO_IMPLEMENTATION
void* evo_temp_malloc(void* v, size_t size) {
	EvoTempStack* temp = (EvoTempStack*)v;
	if (!temp->current) { temp->current = temp->buffer; }

	size_t capacity = sizeof(temp->buffer) / sizeof(*temp->buffer);
	size_t aligned = EVO_ALIGN(size);

	if (temp->filled + aligned > capacity) {
		if (temp->mark) {
			evo_temp_reset_mark(temp);
			if (temp->filled + aligned > capacity) {
				fprintf(stderr, "ERROR:\tYou have set a temporary mark which doesn't allow an allocation to reset the whole temporary stack, but you are trying to allocate an object too big to fit!\n");
				return NULL;
			}
		} else {
			evo_temp_reset(temp);
		}
	}

	uint8_t* ret = (uint8_t*)temp->current;
	temp->current = (char*)temp->current + aligned;
	temp->filled += aligned;
	return ret;
}
void* evo_temp_calloc(void* v, size_t n, size_t size) {
	void* ret = evo_temp_malloc(v, n * size);
	memset(ret, 0, n * size);
	return ret;
}
void evo_temp_set_mark(EvoTempStack* temp) {
	temp->mark = temp->filled;
}
void evo_temp_reset_mark(EvoTempStack* temp) {
	temp->filled = temp->mark;
	temp->current = (char*)temp->buffer + temp->filled;
	temp->mark = 0;
}
void evo_temp_reset(void* v) {
	EvoTempStack* temp = (EvoTempStack*)v;
	temp->current = temp->buffer;
	temp->filled = 0;
}

EvoAllocator evo_allocator_temp(EvoTempStack* temp) {
	return (EvoAllocator) {
		.ctx = temp,

		.malloc_f = evo_temp_malloc,
		.calloc_f = evo_temp_calloc,
		.realloc_f = NULL,
		.reallocarray_f = NULL,
		.destroy_f = NULL,
		.free_f = NULL,
		.reset_f = evo_temp_reset,

		.heap_flag = 0,
	};
}

#endif // EVOCO_IMPLEMENTATION

// Stack Allocator
// Buddy Allocator
// Ring Buffer Allocator
#endif // __EVOCO_H__
