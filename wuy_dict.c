#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <strings.h>
#include <assert.h>

#include "wuy_dict.h"

struct wuy_dict_s {
	wuy_dict_hash_f		*hash;
	wuy_dict_equal_f	*equal;

	wuy_hlist_head_t	*buckets;
	wuy_hlist_head_t	*prev_buckets;

	uint32_t		bucket_size;
	uint32_t		split;

	size_t			node_offset;

	long			count;

	bool			expansion;
};

#define WUY_DICT_SIZE_INIT		64
#define WUY_DICT_SIZE_MAX		64*1024*1024
#define WUY_DICT_EXPANSION_FACTOR	2

wuy_dict_t *wuy_dict_new(wuy_dict_hash_f *hash, wuy_dict_equal_f *equal,
		size_t node_offset)
{
	wuy_dict_t *dict = malloc(sizeof(wuy_dict_t));
	assert(dict != NULL);

	dict->bucket_size = WUY_DICT_SIZE_INIT;
	dict->buckets = calloc(dict->bucket_size, sizeof(wuy_hlist_head_t));
	assert(dict->buckets != NULL);

	dict->hash = hash;
	dict->equal = equal;
	dict->node_offset = node_offset;

	dict->count = 0;
	dict->split = 0;
	dict->prev_buckets = NULL;
	dict->expansion = true;
	return dict;
}

void wuy_dict_destroy(wuy_dict_t *dict)
{
	free(dict->prev_buckets);
	free(dict->buckets);
	free(dict);
}

void wuy_dict_disable_expasion(wuy_dict_t *dict, uint32_t bucket_size)
{
	assert(dict->count == 0);

	dict->expansion = false;

	uint32_t align = 1;
	while (align < bucket_size) {
		align <<= 1;
	}

	dict->bucket_size = align;
	dict->buckets = realloc(dict->buckets, align * sizeof(wuy_hlist_head_t));
	assert(dict->buckets != NULL);
	bzero(dict->buckets, align * sizeof(wuy_hlist_head_t));
}

static wuy_hlist_node_t *__item_to_node(wuy_dict_t *dict, const void *item)
{
	return (wuy_hlist_node_t *)((char *)item - dict->node_offset);
}
static void *__node_to_item(wuy_dict_t *dict, wuy_hlist_node_t *node)
{
	return (char *)node + dict->node_offset;
}

static uint32_t __dict_index(wuy_dict_t *dict, const void *item)
{
	return dict->hash(item) & (dict->bucket_size - 1);
}

static void __dict_expasion(wuy_dict_t *dict)
{
	if (!dict->expansion) {
		return;
	}

	/* expansion */
	if (dict->count >= dict->bucket_size * WUY_DICT_EXPANSION_FACTOR
			&& dict->prev_buckets == NULL
			&& dict->bucket_size < WUY_DICT_SIZE_MAX) {

		wuy_hlist_head_t *newb = calloc(dict->bucket_size * 2,
				sizeof(wuy_hlist_head_t));
		if (newb == NULL) {
			/* if calloc fails, do nothing */
			return;
		}
		dict->bucket_size *= 2;
		dict->prev_buckets = dict->buckets;
		dict->buckets = newb;
		dict->split = 0;
	}

	/* split a bucket */
	if (dict->prev_buckets != NULL) {

		wuy_hlist_node_t *node, *safe;
		wuy_hlist_iter_safe(node, safe, &dict->prev_buckets[dict->split]) {
			void *item = __node_to_item(dict, node);
			wuy_hlist_add(node, &dict->buckets[__dict_index(dict, item)]);
		}

		dict->split++;
		if (dict->split == dict->bucket_size / 2) {
			/* expansion finish */
			free(dict->prev_buckets);
			dict->prev_buckets = NULL;
		}
	}
}

void wuy_dict_add(wuy_dict_t *dict, void *item)
{
	uint32_t index = __dict_index(dict, item);
	wuy_hlist_add(__item_to_node(dict, item), &dict->buckets[index]);

	dict->count++;
	__dict_expasion(dict);
}

void *wuy_dict_get(wuy_dict_t *dict, const void *key)
{
	__dict_expasion(dict);

	uint32_t index = __dict_index(dict, key);

	/* search from dict->buckets */
	wuy_hlist_node_t *node;
	wuy_hlist_iter(node, &dict->buckets[index]) {
		void *item = __node_to_item(dict, node);
		if (dict->equal(item, key)) {
			return item;
		}
	}

	/* search from dict->prev_buckets */
	if (dict->prev_buckets != NULL) {
		uint32_t prev_size = dict->bucket_size / 2;
		if (index >= prev_size) {
			index -= prev_size;
		}
		wuy_hlist_iter(node, &dict->prev_buckets[index]) {
			void *item = __node_to_item(dict, node);
			if (dict->equal(item, key)) {
				return item;
			}
		}
	}

	/* miss */
	return NULL;
}

void wuy_dict_delete(wuy_dict_t *dict, void *item)
{
	wuy_hlist_delete(__item_to_node(dict, item));
	dict->count--;
}

void *wuy_dict_del_key(wuy_dict_t *dict, const void *key)
{
	void *item = wuy_dict_get(dict, key);
	if (item == NULL) {
		return NULL;
	}
	wuy_dict_delete(dict, item);
	return item;
}

/*
 * Here we can not update dict->count which is used for
 * expansion, so you MUST call wuy_dict_disable_expasion()
 * before. Besides, you MUST NOT call wuy_dict_count()
 * which also use dict->count.
 */
void wuy_dict_del_node(wuy_hlist_node_t *node)
{
	wuy_hlist_delete(node);
}

long wuy_dict_count(wuy_dict_t *dict)
{
	return dict->count;
}
