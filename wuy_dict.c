#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <assert.h>

#include "wuy_dict.h"

struct wuy_dict_s {
	wuy_dict_hash_f		*key_hash;
	wuy_dict_equal_f	*key_equal;
	wuy_dict_key_type_e	key_type;
	size_t			key_offset;

	wuy_hlist_head_t	*buckets;
	wuy_hlist_head_t	*prev_buckets;

	uint32_t		bucket_size;
	uint32_t		split;

	size_t			node_offset;

	size_t			count;

	bool			expansion;
};

#define WUY_DICT_SIZE_INIT		64
#define WUY_DICT_SIZE_MAX		(64*1024*1024)
#define WUY_DICT_EXPANSION_FACTOR	2

static wuy_dict_t *wuy_dict_new(size_t node_offset)
{
	wuy_dict_t *dict = malloc(sizeof(wuy_dict_t));
	assert(dict != NULL);

	dict->bucket_size = WUY_DICT_SIZE_INIT;
	dict->buckets = calloc(dict->bucket_size, sizeof(wuy_hlist_head_t));
	assert(dict->buckets != NULL);

	dict->node_offset = node_offset;
	dict->count = 0;
	dict->split = 0;
	dict->prev_buckets = NULL;
	dict->expansion = true;
	return dict;
}

wuy_dict_t *wuy_dict_new_func(wuy_dict_hash_f *key_hash,
		wuy_dict_equal_f *key_equal, size_t node_offset)
{
	wuy_dict_t *dict = wuy_dict_new(node_offset);
	dict->key_hash = key_hash;
	dict->key_equal = key_equal;
	dict->key_type = 100;
	dict->key_offset = 0;
	return dict;
}

wuy_dict_t *wuy_dict_new_type(wuy_dict_key_type_e key_type,
		size_t key_offset, size_t node_offset)
{
	wuy_dict_t *dict = wuy_dict_new(node_offset);
	dict->key_hash = NULL;
	dict->key_equal = NULL;
	dict->key_type = key_type;
	dict->key_offset = key_offset;
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

static const void *_item_to_key(wuy_dict_t *dict, const void *item)
{
	return (const char *)item + dict->key_offset;
}
static wuy_hlist_node_t *_item_to_node(wuy_dict_t *dict, const void *item)
{
	return (wuy_hlist_node_t *)((char *)item + dict->node_offset);
}
static void *_node_to_item(wuy_dict_t *dict, wuy_hlist_node_t *node)
{
	return (char *)node - dict->node_offset;
}

static uint32_t wuy_dict_hash_key(wuy_dict_t *dict, const void *key)
{
	if (dict->key_hash != NULL) {
		return dict->key_hash(key);
	}

	switch (dict->key_type) {
	case WUY_DICT_KEY_UINT32:
		return (uint32_t)(uintptr_t)key;
	case WUY_DICT_KEY_UINT64:
		return (uint64_t)key;
	case WUY_DICT_KEY_STRING:
		return wuy_dict_hash_string(key);
	default:
		abort();
	}
}
static uint32_t wuy_dict_hash_item(wuy_dict_t *dict, const void *item)
{
	if (dict->key_hash != NULL) {
		return dict->key_hash(item);
	}

	const void *item_key = _item_to_key(dict, item);
	switch (dict->key_type) {
	case WUY_DICT_KEY_UINT32:
		return *(uint32_t *)item_key;
	case WUY_DICT_KEY_UINT64:
		return (uint32_t)(*(uint64_t *)item_key);
	case WUY_DICT_KEY_STRING:
		return wuy_dict_hash_string(*(const char **)item_key);
	default:
		abort();
	}
}
static uint32_t wuy_dict_index_key(wuy_dict_t *dict, const void *key)
{
	return wuy_dict_hash_key(dict, key) & (dict->bucket_size - 1);
}
static uint32_t wuy_dict_index_item(wuy_dict_t *dict, const void *item)
{
	return wuy_dict_hash_item(dict, item) & (dict->bucket_size - 1);
}

static bool wuy_dict_equal_key(wuy_dict_t *dict, const void *item, const void *key)
{
	if (dict->key_equal != NULL) {
		return dict->key_equal(item, key);
	}

	const void *item_key = _item_to_key(dict, item);
	switch (dict->key_type) {
	case WUY_DICT_KEY_UINT32:
		return *(uint32_t *)item_key == (uint32_t)(uintptr_t)key;
	case WUY_DICT_KEY_UINT64:
		return *(uint64_t *)item_key == (uint64_t)key;
	case WUY_DICT_KEY_STRING:
		return strcmp(*(const char **)item_key, key) == 0;
	default:
		abort();
	}
}

static void wuy_dict_expasion(wuy_dict_t *dict)
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
		wuy_hlist_iter_safe(&dict->prev_buckets[dict->split], node, safe) {
			void *item = _node_to_item(dict, node);
			uint32_t index = wuy_dict_index_item(dict, item);
			wuy_hlist_insert(&dict->buckets[index], node);
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
	uint32_t index = wuy_dict_index_item(dict, item);
	wuy_hlist_insert(&dict->buckets[index], _item_to_node(dict, item));

	dict->count++;
	wuy_dict_expasion(dict);
}

void *_wuy_dict_get(wuy_dict_t *dict, const void *key)
{
	wuy_dict_expasion(dict);

	uint32_t index = wuy_dict_index_key(dict, key);

	/* search from dict->buckets */
	wuy_hlist_node_t *node;
	wuy_hlist_iter(&dict->buckets[index], node) {
		void *item = _node_to_item(dict, node);
		if (wuy_dict_equal_key(dict, item, key)) {
			return item;
		}
	}

	/* search from dict->prev_buckets */
	if (dict->prev_buckets != NULL) {
		uint32_t prev_size = dict->bucket_size / 2;
		if (index >= prev_size) {
			index -= prev_size;
		}
		wuy_hlist_iter(&dict->prev_buckets[index], node) {
			void *item = _node_to_item(dict, node);
			if (wuy_dict_equal_key(dict, item, key)) {
				return item;
			}
		}
	}

	/* miss */
	return NULL;
}

void wuy_dict_delete(wuy_dict_t *dict, void *item)
{
	wuy_hlist_delete(_item_to_node(dict, item));
	dict->count--;
}

void *_wuy_dict_del_key(wuy_dict_t *dict, const void *key)
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

size_t wuy_dict_count(wuy_dict_t *dict)
{
	return dict->count;
}
