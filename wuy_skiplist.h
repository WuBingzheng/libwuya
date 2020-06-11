/**
 * @file     wuy_skiplist.h
 * @author   Wu Bingzheng <wubingzheng@gmail.com>
 * @date     2018-7-19
 *
 * @section LICENSE
 * GPLv2
 *
 * @section DESCRIPTION
 *
 * A skip list.
 */

#ifndef WUY_SKIPLIST_H
#define WUY_SKIPLIST_H

#include <stdbool.h>
#include <stdint.h>
#include "wuy_container.h"

/**
 * @brief Memory pool for skiplist node.
 *
 * A pool should be shared amount all skiplists in a thread, but not cross thread.
 */
typedef struct wuy_skiplist_pool_s wuy_skiplist_pool_t;

/**
 * @brief The skiplist.
 */
typedef struct wuy_skiplist_s wuy_skiplist_t;

/**
 * @brief Embed this node into your data struct in order to use this lib.
 *
 * Pass its offset in your data struct to wuy_skiplist_new_xxx(), and
 * you need not use it later.
 *
 * You need not initialize it before using it.
 */
typedef struct wuy_skiplist_node_s wuy_skiplist_node_t;
struct wuy_skiplist_node_s {
	wuy_skiplist_node_t	*nexts[1];
};

/**
 * @brief Return if the former is less than latter.
 *
 * Make sure that if less(A, B) is true, then less(B, A) must be false,
 * which means that there should not be 2 items with same key value.
 * If there are same key values, then compare a second key.
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

/**
 * @brief Create a new memory pool for skiplist.
 */
wuy_skiplist_pool_t *wuy_skiplist_pool_new(int max);

/**
 * @brief Create a new skiplist, with the user-defined comparison function.
 *
 * @param key_less use-defined compare function.
 * @param node_offset the offset of wuy_skiplist_node_t in your data struct.
 * @param skpool the memory pool.
 *
 * @return the new skiplist. It aborts the program if memory allocation fails.
 */
wuy_skiplist_t *wuy_skiplist_new_func(wuy_skiplist_less_f *key_less,
		size_t node_offset, wuy_skiplist_pool_t *skpool);

/**
 * @brief Create a new skiplist, with general comparison key type.
 *
 * @param key_type see wuy_skiplist_key_type_e.
 * @param key_offset the offset of key in your data struct.
 * @param key_reverse if reverse the comparison.
 * @param node_offset the offset of wuy_skiplist_node_t in your data struct.
 * @param skpool the memory pool.
 *
 * @return the new skiplist. It aborts the program if memory allocation fails.
 */
wuy_skiplist_t *wuy_skiplist_new_type(wuy_skiplist_key_type_e key_type,
		size_t key_offset, bool key_reverse,
		size_t node_offset, wuy_skiplist_pool_t *skpool);

/**
 * @brief Destroy a skiplist.
 *
 * It just frees the skiplist, but do not releases the nodes.
 */
void wuy_skiplist_destroy(wuy_skiplist_t *skiplist);

/**
 * @brief Insert an item to skiplist.
 *
 * @return true if success, or false if memory allocation fails.
 */
bool wuy_skiplist_insert(wuy_skiplist_t *skiplist, void *item);

/**
 * @brief Delete an item from skiplist.
 *
 * @return true if success, or false if the item is not linked.
 */
bool wuy_skiplist_delete(wuy_skiplist_t *skiplist, void *item);

#define wuy_skiplist_search(skiplist, key) \
	_wuy_skiplist_search(skiplist, (const void *)(uintptr_t)key)
void *_wuy_skiplist_search(wuy_skiplist_t *skiplist, const void *key);

#define wuy_skiplist_del_key(skiplist, key) \
	_wuy_skiplist_del_key(skiplist, (const void *)(uintptr_t)key)
void *_wuy_skiplist_del_key(wuy_skiplist_t *skiplist, const void *key);

/**
 * @brief Return the count of nodes in skiplist.
 */
long wuy_skiplist_count(wuy_skiplist_t *skiplist);

wuy_skiplist_node_t *wuy_skiplist_iter_new(wuy_skiplist_t *skiplist);
void *wuy_skiplist_iter_next(wuy_skiplist_t *skiplist, wuy_skiplist_node_t **iter);
bool wuy_skiplist_iter_less(wuy_skiplist_t *skiplist,
		const void *item, const void *stop_key);

/**
 * @brief Iterate over a skiplist.
 */
#define wuy_skiplist_iter(skiplist, item) \
	for (wuy_skiplist_node_t *_sk_iter = wuy_skiplist_iter_new(skiplist); \
		(item = wuy_skiplist_iter_next(skiplist, &_sk_iter)) != NULL; )

/**
 * @brief Iterate over a skiplist and stop at @stop.
 */
#define wuy_skiplist_iter_stop(skiplist, item, stop) \
	for (wuy_skiplist_node_t *_sk_iter = wuy_skiplist_iter_new(skiplist); \
		(item = wuy_skiplist_iter_next(skiplist, &_sk_iter)) != NULL \
			&& (stop == NULL || wuy_skiplist_iter_less(skiplist, item, stop)); )

/**
 * @brief Return the first item.
 */
void *wuy_skiplist_first(wuy_skiplist_t *skiplist);

/**
 * @brief Iterate over a skiplist, always getting the first.
 */
#define wuy_skiplist_iter_first(skiplist, item) \
	while ((item = wuy_skiplist_first(skiplist)) != NULL)

#endif
