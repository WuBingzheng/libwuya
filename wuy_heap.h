#ifndef WUY_HEAP_H
#define WUY_HEAP_H

#include <stdbool.h>
#include <stddef.h>

typedef enum {
	WUY_HEAP_KEY_FUNC,
	WUY_HEAP_KEY_INT32,
	WUY_HEAP_KEY_UINT32,
	WUY_HEAP_KEY_INT64,
	WUY_HEAP_KEY_UINT64,
	WUY_HEAP_KEY_FLOAT,
	WUY_HEAP_KEY_DOUBLE,
	WUY_HEAP_KEY_STRING,
} wuy_heap_key_type_e;

typedef struct wuy_heap_s wuy_heap_t;

typedef struct {
	size_t index;
} wuy_heap_node_t;

typedef bool wuy_heap_less_f(const void *, const void *);

wuy_heap_t *wuy_heap_new_func(size_t node_offset, wuy_heap_less_f *key_less);

wuy_heap_t *wuy_heap_new_type(size_t node_offset, wuy_heap_key_type_e key_type,
		size_t key_offset, bool key_reverse);

bool wuy_heap_push(wuy_heap_t *heap, void *item);
void *wuy_heap_pop(wuy_heap_t *heap);
void *wuy_heap_min(wuy_heap_t *heap);
void wuy_heap_delete(wuy_heap_t *heap, void *item);

size_t wuy_heap_count(wuy_heap_t *heap);

#endif
