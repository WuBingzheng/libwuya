#ifndef WUY_SKIPLIST_H
#define WUY_SKIPLIST_H

#include <stdbool.h>
#include <stdint.h>
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

bool wuy_skiplist_insert(wuy_skiplist_t *skiplist, void *item);

bool wuy_skiplist_delete(wuy_skiplist_t *skiplist, void *item);

#define wuy_skiplist_search(skiplist, key) \
	_wuy_skiplist_search(skiplist, (const void *)(uintptr_t)key)
void *_wuy_skiplist_search(wuy_skiplist_t *skiplist, const void *key);

#define wuy_skiplist_del_key(skiplist, key) \
	_wuy_skiplist_del_key(skiplist, (const void *)(uintptr_t)key)
void *_wuy_skiplist_del_key(wuy_skiplist_t *skiplist, const void *key);

long wuy_skiplist_count(wuy_skiplist_t *skiplist);

wuy_skiplist_node_t *wuy_skiplist_iter_new(wuy_skiplist_t *skiplist);
void *wuy_skiplist_iter_next(wuy_skiplist_t *skiplist, wuy_skiplist_node_t **iter);
bool wuy_skiplist_iter_less(wuy_skiplist_t *skiplist,
		const void *item, const void *stop_key);

#define wuy_skiplist_iter(skiplist, item) \
	for (wuy_skiplist_node_t *_sk_iter = wuy_skiplist_iter_new(skiplist); \
		(item = wuy_skiplist_iter_next(skiplist, &_sk_iter)) != NULL; )

#define wuy_skiplist_iter_stop(skiplist, item, stop) \
	for (wuy_skiplist_node_t *_sk_iter = wuy_skiplist_iter_new(skiplist); \
		(item = wuy_skiplist_iter_next(skiplist, &_sk_iter)) != NULL \
			&& (stop == NULL || wuy_skiplist_iter_less(skiplist, item, stop)); )

void *wuy_skiplist_first(wuy_skiplist_t *skiplist);
#define wuy_skiplist_iter_first(skiplist, item) \
	while ((item = wuy_skiplist_first(skiplist)) != NULL)

#endif
