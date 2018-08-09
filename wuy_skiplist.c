#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "wuy_pool.h"
#include "wuy_skiplist.h"

struct wuy_skiplist_pool_s {
	int		max;
	wuy_pool_t	*pools[0];
}; 

struct wuy_skiplist_s {
	wuy_skiplist_key_type_e	key_type;
	wuy_skiplist_less_f	*key_less;
	size_t			key_offset;
	bool			key_reverse;

	size_t			node_offset;

	int			level;

	long			count;

	wuy_skiplist_pool_t	*skpool;

	wuy_skiplist_node_t	header;
};

wuy_skiplist_pool_t *wuy_skiplist_pool_new(int max)
{
	wuy_skiplist_pool_t *skpool = malloc(sizeof(wuy_skiplist_pool_t)
			+ sizeof(wuy_pool_t *) * max);
	assert(skpool != NULL);

	skpool->max = max;
	for (int i = 0; i < max - 1; i++) {
		skpool->pools[i] = wuy_pool_new(sizeof(wuy_skiplist_node_t *) * (i+2));
		assert(skpool->pools[i] != NULL);
	}
	return skpool;
}

static wuy_skiplist_t *wuy_skiplist_new(size_t node_offset, wuy_skiplist_pool_t *skpool)
{
	wuy_skiplist_t *skiplist = malloc(sizeof(wuy_skiplist_t)
			+ sizeof(wuy_skiplist_node_t *) * skpool->max);
	assert(skiplist != NULL);

	bzero(&skiplist->header, sizeof(wuy_skiplist_node_t *) * skpool->max);
	skiplist->skpool = skpool;
	skiplist->node_offset = node_offset;
	return skiplist;
}

wuy_skiplist_t *wuy_skiplist_new_func(wuy_skiplist_less_f *key_less,
		size_t node_offset, wuy_skiplist_pool_t *skpool)
{
	wuy_skiplist_t *skiplist = wuy_skiplist_new(node_offset, skpool);
	skiplist->key_less = key_less;
	return skiplist;
}

wuy_skiplist_t *wuy_skiplist_new_type(wuy_skiplist_key_type_e key_type,
		size_t key_offset, bool key_reverse,
		size_t node_offset, wuy_skiplist_pool_t *skpool)
{
	wuy_skiplist_t *skiplist = wuy_skiplist_new(node_offset, skpool);
	skiplist->key_less = NULL;
	skiplist->key_type = key_type;
	skiplist->key_offset = key_offset;
	skiplist->key_reverse = key_reverse;
	return skiplist;
}

static wuy_skiplist_node_t *_item_to_node(wuy_skiplist_t *skiplist, const void *item)
{
	return (wuy_skiplist_node_t *)((char *)item + skiplist->node_offset);
}
static void *_node_to_item(wuy_skiplist_t *skiplist, wuy_skiplist_node_t *node)
{
	return (char *)node - skiplist->node_offset;
}

static bool wuy_skiplist_less(wuy_skiplist_t *skiplist, const void *a, const void *b)
{
	if (skiplist->key_less != NULL) {
		return skiplist->key_less(a, b);
	}

	bool ret;
	switch (skiplist->key_type) {
	case WUY_SKIPLIST_KEY_INT32:
		ret = *(const int32_t *)a < *(const int32_t *)b;
		break;
	case WUY_SKIPLIST_KEY_UINT32:
		ret = *(const uint32_t *)a < *(const uint32_t *)b;
		break;
	case WUY_SKIPLIST_KEY_INT64:
		ret = *(const int64_t *)a < *(const int64_t *)b;
		break;
	case WUY_SKIPLIST_KEY_UINT64:
		ret = *(const uint64_t *)a < *(const uint64_t *)b;
		break;
	case WUY_SKIPLIST_KEY_FLOAT:
		ret = *(const float *)a < *(const float *)b;
		break;
	case WUY_SKIPLIST_KEY_DOUBLE:
		ret = *(const double *)a < *(const double *)b;
		break;
	case WUY_SKIPLIST_KEY_STRING:
		ret = strcmp(a, b) < 0;
		break;
	default:
		abort();
	}

	return skiplist->key_reverse ? !ret : ret;
}

static bool wuy_skiplist_next_less(wuy_skiplist_t *skiplist,
		wuy_skiplist_node_t *node, const void *item)
{
	return wuy_skiplist_less(skiplist, _node_to_item(skiplist, node->nexts[0]), item);
}

static void wuy_skiplist_get_previous(wuy_skiplist_t *skiplist,
		wuy_skiplist_node_t **previous, const void *key)
{
	if (skiplist->level == 0) {
		previous[0] = NULL;
		return;
	}

	wuy_skiplist_node_t *node = &skiplist->header;
	for (int i = skiplist->level - 1; i > 0; i--) {
		while (node->nexts[i] != NULL && wuy_skiplist_next_less(skiplist,
					node->nexts[i], key)) {
			node = node->nexts[i];
		}
		previous[i] = node;
	}

	while (node->nexts[0] != NULL && wuy_skiplist_next_less(skiplist, node, key)) {
		node = node->nexts[0];
	}
	previous[0] = node;
}

static int wuy_skiplist_random_level(int max)
{
	int level = 1;
	while (1) {
		long r = random();
		for (int i = 0; i < 15; i++) {
			if ((r & 0x3) != 0) {
				return level;
			}
			level++;
			if (level == max) {
				return max;
			}
			r >>= 2;
		}
	}
}

static wuy_skiplist_node_t *wuy_skiplist_node_new(wuy_skiplist_t *skiplist, int level)
{
	return wuy_pool_alloc(skiplist->skpool->pools[level - 2]);
}

bool wuy_skiplist_add(wuy_skiplist_t *skiplist, void *item)
{
	wuy_skiplist_node_t *previous[skiplist->skpool->max];
	wuy_skiplist_get_previous(skiplist, previous, item);

	int level = wuy_skiplist_random_level(skiplist->skpool->max);

	/* increase skiplist->level if need */
	while (skiplist->level < level) {
		previous[skiplist->level++] = &skiplist->header;
	}

	wuy_skiplist_node_t *item_node = _item_to_node(skiplist, item);
	item_node->nexts[0] = previous[0]->nexts[0];
	previous[0]->nexts[0] = item_node;

	if (level > 1) {
		wuy_skiplist_node_t *new_node = wuy_skiplist_node_new(skiplist, level);
		if (new_node == NULL) {
			return false;
		}
		for (int i = level - 1; i > 0; i--) {
			new_node->nexts[i] = previous[i]->nexts[i];
			previous[i]->nexts[i] = new_node;
		}
		new_node->nexts[0] = item_node;
	}

	skiplist->count++;

	return true;
}

static void wuy_skiplist_delete_node(wuy_skiplist_t *skiplist,
		wuy_skiplist_node_t **previous, wuy_skiplist_node_t *node)
{
	int i;
	for (i = skiplist->level - 1; i > 0; i--) {
		wuy_skiplist_node_t *next = previous[i]->nexts[i];
		if (next != NULL && next->nexts[0] == node) {
			break;
		}
	}
	wuy_skiplist_node_t *ex_node = previous[i]->nexts[i];
	for (; i >= 0; i--) {
		previous[i]->nexts[i] = previous[i]->nexts[i]->nexts[i];
	}

	if (ex_node != node) {
		wuy_pool_free(ex_node);
	}

	/* decrease skiplist->level if need */
	while (skiplist->header.nexts[skiplist->level - 1] == NULL) {
		skiplist->level--;
	}

	skiplist->count--;
}

bool wuy_skiplist_delete(wuy_skiplist_t *skiplist, void *item)
{
	if (skiplist->level == 0) {
		return false;
	}

	wuy_skiplist_node_t *previous[skiplist->skpool->max];
	wuy_skiplist_get_previous(skiplist, previous, item);

	wuy_skiplist_node_t *node = _item_to_node(skiplist, item);
	if (previous[0]->nexts[0] != node) {
		return false;
	}

	wuy_skiplist_delete_node(skiplist, previous, node);
	return true;
}

static void *wuy_skiplist_search_key(wuy_skiplist_t *skiplist,
		wuy_skiplist_node_t **previous, const void *key)
{
	if (skiplist->level == 0) {
		return NULL;
	}

	wuy_skiplist_get_previous(skiplist, previous, key);

	void *item = _node_to_item(skiplist, previous[0]->nexts[0]);
	if (wuy_skiplist_less(skiplist, key, item)) {
		return NULL;
	}

	return item;
}

void *wuy_skiplist_search(wuy_skiplist_t *skiplist, const void *key)
{
	wuy_skiplist_node_t *previous[skiplist->skpool->max];
	return wuy_skiplist_search_key(skiplist, previous, key);
}

void *wuy_skiplist_del_key(wuy_skiplist_t *skiplist, const void *key)
{
	wuy_skiplist_node_t *previous[skiplist->skpool->max];
	void *item = wuy_skiplist_search_key(skiplist, previous, key);
	if (item == NULL) {
		return NULL;
	}

	wuy_skiplist_delete_node(skiplist, previous, _item_to_node(skiplist, item));
	return item;
}

wuy_skiplist_node_t *wuy_skiplist_first(wuy_skiplist_t *skiplist)
{
	return skiplist->header.nexts[0];
}

long wuy_skiplist_count(wuy_skiplist_t *skiplist)
{
	return skiplist->count;
}
