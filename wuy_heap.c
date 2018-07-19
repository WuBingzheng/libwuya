#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "wuy_heap.h"

struct wuy_heap_s {
	wuy_heap_key_type_e	key_type;
	wuy_heap_less_f		*key_less;
	size_t			key_offset;
	bool			key_reverse;

	wuy_heap_node_t 	**array;
	size_t			capture;
	size_t			count;

	size_t			node_offset;
};

#define WUY_HEAP_SIZE_INIT 1024

static wuy_heap_t *__heap_new(size_t node_offset)
{
	wuy_heap_t *heap = malloc(sizeof(wuy_heap_t));
	assert(heap != NULL);

	heap->capture = WUY_HEAP_SIZE_INIT;
	heap->array = malloc(sizeof(wuy_heap_node_t *) * heap->capture);
	assert(heap->array != NULL);

	heap->count = 0;
	heap->node_offset = node_offset;
	return heap;
}

wuy_heap_t *wuy_heap_new_func(size_t node_offset, wuy_heap_less_f *key_less)
{
	wuy_heap_t *heap = __heap_new(node_offset);
	heap->key_less = key_less;
	return heap;
}

wuy_heap_t *wuy_heap_new_type(size_t node_offset, wuy_heap_key_type_e key_type,
		size_t key_offset, bool key_reverse)
{
	wuy_heap_t *heap = __heap_new(node_offset);
	heap->key_less = NULL;
	heap->key_type = key_type;
	heap->key_offset = key_offset;
	heap->key_reverse = key_reverse;
	return heap;
}

static wuy_heap_node_t *__item_to_node(wuy_heap_t *heap, const void *item)
{
	return (wuy_heap_node_t *)((char *)item - heap->node_offset);
}
static void *__index_to_item(wuy_heap_t *heap, size_t i)
{
	return (char *)(heap->array[i]) + heap->node_offset;
}
static void *__index_to_key(wuy_heap_t *heap, size_t i)
{
	return (char *)(heap->array[i]) + heap->node_offset - heap->key_offset;
}

bool __heap_less(wuy_heap_t *heap, size_t i, size_t j)
{
	if (heap->key_less != NULL) {
		return heap->key_less(__index_to_item(heap, i), __index_to_item(heap, j));
	}

	void *pki = __index_to_key(heap, i);
	void *pkj = __index_to_key(heap, j);

	bool ret;

	switch (heap->key_type) {
	case WUY_HEAP_KEY_INT32:
		ret = *(int32_t *)pki < *(int32_t *)pkj;
		break;
	case WUY_HEAP_KEY_UINT32:
		ret = *(uint32_t *)pki < *(uint32_t *)pkj;
		break;
	case WUY_HEAP_KEY_INT64:
		ret = *(int64_t *)pki < *(int64_t *)pkj;
		break;
	case WUY_HEAP_KEY_UINT64:
		ret = *(uint64_t *)pki < *(uint64_t *)pkj;
		break;
	case WUY_HEAP_KEY_FLOAT:
		ret = *(float *)pki < *(float *)pkj;
		break;
	case WUY_HEAP_KEY_DOUBLE:
		ret = *(double *)pki < *(double *)pkj;
		break;
	case WUY_HEAP_KEY_STRING:
		ret = strcmp(pki, pkj) < 0;
		break;
	default:
		abort();
	}

	return heap->key_reverse ? ret : !ret;
}

static void __heap_swap(wuy_heap_t *heap, size_t i, size_t j)
{
	wuy_heap_node_t *tmp = heap->array[i];
	heap->array[i] = heap->array[j];
	heap->array[j] = tmp;

	heap->array[i]->index = i;
	heap->array[j]->index = j;
}

static void __heapify_up(wuy_heap_t *heap, size_t i)
{
	while (1) {
		size_t parent = (i - 1) / 2;
		if (parent == i || !__heap_less(heap, i, parent)) {
			break;
		}
		__heap_swap(heap, i, parent);
		i = parent;
	}
}

static void __heapify_down(wuy_heap_t *heap, size_t i)
{
	while (1) {
		size_t left = i * 2 + 1;
		size_t right = left + 1;

		if (left < heap->count && __heap_less(heap, left, i)) {
			__heap_swap(heap, left, i);
			i = left;
		} else if (right < heap->count && __heap_less(heap, right, i)) {
			__heap_swap(heap, right, i);
			i = right;
		} else {
			break;
		}
	}
}

bool wuy_heap_add(wuy_heap_t *heap, void *item)
{
	if (heap->count >= heap->capture) {
		void *olda = heap->array;
		size_t oldc = heap->capture;

		heap->capture *= 2;
		heap->array = realloc(heap->array, heap->capture);
		if (heap->array == NULL) {
			heap->capture = oldc + WUY_HEAP_SIZE_INIT;
			heap->array = realloc(heap->array, heap->capture);
			if (heap->array == NULL) { /* roll back */
				heap->array = olda;
				heap->capture = oldc;
				return false;
			}
		}
	}

	wuy_heap_node_t *node = __item_to_node(heap, item);
	heap->array[heap->count] = node;
	node->index = heap->count;

	__heapify_up(heap, heap->count);
	heap->count++;
	return true;
}

void *wuy_heap_pop(wuy_heap_t *heap)
{
	if (heap->count == 0) {
		return NULL;
	}

	void *item = __index_to_item(heap, 0);

	__heap_swap(heap, 0, heap->count--);
	__heapify_down(heap, 0);

	return item;
}

void *wuy_heap_min(wuy_heap_t *heap)
{
	if (heap->count == 0) {
		return NULL;
	}

	return __index_to_item(heap, 0);
}

bool wuy_heap_delete(wuy_heap_t *heap, void *item)
{
	wuy_heap_node_t *node = __item_to_node(heap, item);
	size_t index = node->index;

	if (index >= heap->count || heap->array[index] != node) {
		return false;
	}

	__heap_swap(heap, index, heap->count--);
	__heapify_down(heap, index);
	return true;
}

size_t wuy_heap_count(wuy_heap_t *heap)
{
	return heap->count;
}
