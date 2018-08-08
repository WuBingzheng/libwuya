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

static wuy_heap_t *wuy_heap_new(size_t node_offset)
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

wuy_heap_t *wuy_heap_new_func(wuy_heap_less_f *key_less, size_t node_offset)
{
	wuy_heap_t *heap = wuy_heap_new(node_offset);
	heap->key_less = key_less;
	return heap;
}

wuy_heap_t *wuy_heap_new_type(wuy_heap_key_type_e key_type, size_t key_offset,
		bool key_reverse, size_t node_offset)
{
	wuy_heap_t *heap = wuy_heap_new(node_offset);
	heap->key_less = NULL;
	heap->key_type = key_type;
	heap->key_offset = key_offset;
	heap->key_reverse = key_reverse;
	return heap;
}

static wuy_heap_node_t *_item_to_node(wuy_heap_t *heap, const void *item)
{
	return (wuy_heap_node_t *)((char *)item + heap->node_offset);
}
static void *_index_to_item(wuy_heap_t *heap, size_t i)
{
	return (char *)(heap->array[i]) - heap->node_offset;
}
static void *_index_to_key(wuy_heap_t *heap, size_t i)
{
	return (char *)(heap->array[i]) - heap->node_offset + heap->key_offset;
}

static bool wuy_heap_less(wuy_heap_t *heap, size_t i, size_t j)
{
	if (heap->key_less != NULL) {
		return heap->key_less(_index_to_item(heap, i), _index_to_item(heap, j));
	}

	void *pki = _index_to_key(heap, i);
	void *pkj = _index_to_key(heap, j);

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

static void wuy_heap_swap(wuy_heap_t *heap, size_t i, size_t j)
{
	wuy_heap_node_t *tmp = heap->array[i];
	heap->array[i] = heap->array[j];
	heap->array[j] = tmp;

	heap->array[i]->index = i;
	heap->array[j]->index = j;
}

static void wuy_heapify_up(wuy_heap_t *heap, size_t i)
{
	while (i > 0) {
		size_t parent = (i - 1) / 2;
		if (parent == i || !wuy_heap_less(heap, i, parent)) {
			break;
		}
		wuy_heap_swap(heap, i, parent);
		i = parent;
	}
}

static void wuy_heapify_down(wuy_heap_t *heap, size_t i)
{
	while (1) {
		size_t left = i * 2 + 1;
		size_t right = left + 1;

		if (left < heap->count && wuy_heap_less(heap, left, i)) {
			wuy_heap_swap(heap, left, i);
			i = left;
		} else if (right < heap->count && wuy_heap_less(heap, right, i)) {
			wuy_heap_swap(heap, right, i);
			i = right;
		} else {
			break;
		}
	}
}

bool wuy_heap_is_linked(wuy_heap_t *heap, wuy_heap_node_t *node)
{
	size_t index = node->index;
	return (index < heap->count && heap->array[index] == node);
}

bool wuy_heap_push_node(wuy_heap_t *heap, wuy_heap_node_t *node)
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

	heap->array[heap->count] = node;
	node->index = heap->count;

	wuy_heapify_up(heap, heap->count);
	heap->count++;
	return true;
}

bool wuy_heap_push(wuy_heap_t *heap, void *item)
{
	wuy_heap_node_t *node = _item_to_node(heap, item);
	assert(!wuy_heap_is_linked(heap, node));

	return wuy_heap_push_node(heap, node);
}

static void wuy_heap_fix_node(wuy_heap_t *heap, wuy_heap_node_t *node)
{
	size_t index = node->index;

	if (index > 0 && wuy_heap_less(heap, index, (index - 1) / 2)) {
		wuy_heapify_up(heap, index);
	} else {
		wuy_heapify_down(heap, index);
	}
}

void wuy_heap_fix(wuy_heap_t *heap, void *item)
{
	wuy_heap_node_t *node = _item_to_node(heap, item);
	assert(wuy_heap_is_linked(heap, node));

	wuy_heap_fix_node(heap, node);
}

bool wuy_heap_push_or_fix(wuy_heap_t *heap, void *item)
{
	wuy_heap_node_t *node = _item_to_node(heap, item);

	if (wuy_heap_is_linked(heap, node)) {
		wuy_heap_fix_node(heap, node);
		return true;
	} else {
		return wuy_heap_push_node(heap, node);
	}
}

static void wuy_heap_delete_index(wuy_heap_t *heap, size_t index)
{
	heap->count--;
	if (heap->count > 0) {
		wuy_heap_swap(heap, index, heap->count);
		wuy_heapify_down(heap, index);
	}
}

void *wuy_heap_pop(wuy_heap_t *heap)
{
	if (heap->count == 0) {
		return NULL;
	}

	void *item = _index_to_item(heap, 0);

	wuy_heap_delete_index(heap, 0);

	return item;
}

void *wuy_heap_min(wuy_heap_t *heap)
{
	if (heap->count == 0) {
		return NULL;
	}

	return _index_to_item(heap, 0);
}

bool wuy_heap_delete(wuy_heap_t *heap, void *item)
{
	wuy_heap_node_t *node = _item_to_node(heap, item);

	if (!wuy_heap_is_linked(heap, node)) {
		return false;
	}

	wuy_heap_delete_index(heap, node->index);
	return true;
}

size_t wuy_heap_count(wuy_heap_t *heap)
{
	return heap->count;
}
