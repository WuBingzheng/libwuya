#ifndef WUY_SKIPLIST_H
#define WUY_SKIPLIST_H

#include <stdbool.h>
#include "wuy_container.h"

typedef struct wuy_skiplist_pool_s wuy_skiplist_pool_t;

typedef struct wuy_skiplist_s wuy_skiplist_t;

typedef struct wuy_skiplist_node_s wuy_skiplist_node_t;
struct wuy_skiplist_node_s {
	wuy_skiplist_node_t	*nexts[1];
};

/**
 * @brief Return if the former is less than latter.
 *
 * Used in wuy_skiplist_new_func().
 */
typedef bool wuy_skiplist_less_f(const void *, const void *);

/**
 * @brief List of your item's comparison key type.
 *
 * Used in wuy_skiplist_new_type().
 */
typedef enum {
	WUY_SKIPLIST_KEY_INT32,
	WUY_SKIPLIST_KEY_UINT32,
	WUY_SKIPLIST_KEY_INT64,
	WUY_SKIPLIST_KEY_UINT64,
	WUY_SKIPLIST_KEY_FLOAT,
	WUY_SKIPLIST_KEY_DOUBLE,
	WUY_SKIPLIST_KEY_STRING,
} wuy_skiplist_key_type_e;

wuy_skiplist_pool_t *wuy_skiplist_pool_new(int max);

/* make sure no equal key! */
wuy_skiplist_t *wuy_skiplist_new_func(wuy_skiplist_less_f *key_less,
		size_t node_offset, wuy_skiplist_pool_t *skpool);

wuy_skiplist_t *wuy_skiplist_new_type(wuy_skiplist_key_type_e key_type,
		size_t key_offset, bool key_reverse,
		size_t node_offset, wuy_skiplist_pool_t *skpool);

bool wuy_skiplist_add(wuy_skiplist_t *skiplist, void *item);

bool wuy_skiplist_delete(wuy_skiplist_t *skiplist, void *item);

void *wuy_skiplist_search(wuy_skiplist_t *skiplist, const void *key);

void *wuy_skiplist_del_key(wuy_skiplist_t *skiplist, const void *key);

long wuy_skiplist_count(wuy_skiplist_t *skiplist);

void *wuy_skiplist_first(wuy_skiplist_t *skiplist);
void *wuy_skiplist_next(wuy_skiplist_t *skiplist, void *item);

#define wuy_skiplist_iter(item, skiplist) \
	for (item = wuy_skiplist_first(skiplist); item != NULL; \
			item = wuy_skiplist_next(skiplist, item))

#define wuy_skiplist_iter_safe(item, next, skiplist) \
	for (item = wuy_skiplist_first(skiplist), next = wuy_skiplist_next(skiplist, item); \
			item != NULL; \
			item = next, next = wuy_skiplist_next(skiplist, item))

#endif
