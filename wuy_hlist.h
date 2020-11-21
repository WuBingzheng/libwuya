/**
 * @file     wuy_hlist.h
 * @author   Wu Bingzheng <wubingzheng@gmail.com>
 * @date     2018-7-18
 *
 * @section LICENSE
 * GPLv2
 *
 * @section DESCRIPTION
 *
 * Double linked lists with a single pointer list head.
 * Mostly useful for hash tables where the two pointer list head is
 * too wasteful.
 * You lose the ability to access the tail in O(1).
 *
 * Copied from include/linux/list.h. We remove and rename some structs
 * and functions for consistent with the library.
 */

#ifndef WUY_HLIST_H
#define WUY_HLIST_H

#include <stdio.h>
#include <stddef.h>

#include "wuy_container.h"

/**
 * @brief The list.
 */
typedef struct wuy_hlist_s wuy_hlist_t;

/**
 * @brief Embed this node into your data struct in order to use this lib.
 */
typedef struct wuy_hlist_node_s wuy_hlist_node_t;

struct wuy_hlist_s {
	wuy_hlist_node_t *first;
};
struct wuy_hlist_node_s {
	wuy_hlist_node_t *next, **pprev;
};

/**
 * @brief Initialize at declare.
 */
#define WUY_HLIST_INIT { .first = NULL }

/**
 * @brief Declare and initialize.
 */
#define WUY_HLIST(name) wuy_hlist_t name = { .first = NULL }

/**
 * @brief Initialize a list.
 */
static inline void wuy_hlist_init(wuy_hlist_t *list)
{
	list->first = NULL;
}

/**
 * @brief Return if the list is empty.
 */
static inline int wuy_hlist_empty(const wuy_hlist_t *list)
{
	return list->first == NULL;
}

/**
 * @brief Add node after head.
 */
static inline void wuy_hlist_insert(wuy_hlist_t *list, wuy_hlist_node_t *node)
{
	wuy_hlist_node_t *first = list->first;
	node->next = first;
	if (first)
		first->pprev = &node->next;
	list->first = node;
	node->pprev = &list->first;
}

/**
 * @brief Delete node from its list.
 */
static inline void wuy_hlist_delete(wuy_hlist_node_t *node)
{
	wuy_hlist_node_t *next = node->next;
	wuy_hlist_node_t **pprev = node->pprev;
	*pprev = next;
	if (next)
		next->pprev = pprev;
}

/**
 * @brief Iterate over a list, while it's NOT safe to delete node.
 */
#define wuy_hlist_iter(list, pos) \
	for (pos = (list)->first; pos; pos = pos->next)

/**
 * @brief Iterate over a list, while it's safe to delete node.
 */
#define wuy_hlist_iter_safe(list, pos, n) \
	for (pos = (list)->first, n = pos?pos->next:NULL; pos; \
		pos = n, n = pos?pos->next:NULL)

/**
 * @brief Iterate over a list. It's safe to delete node during it.
 *
 * @param p  a pointer of user's struct type;
 * @param member  name of wuy_hlist_node_t member in user's struct.
 *
 * Compared to @wuy_hlist_iter_safe, this macro transfer hlist node
 * to user's struct type pointer. This it more convenient.
 * Use @wuy_hlist_iter and @wuy_hlist_iter_safe only when you do not
 * know the container's type, e.g. in wuy_dict.c .
 */
#define wuy_hlist_iter_type(list, p, member) \
	for (wuy_hlist_node_t *_next, *_iter = (list)->first; \
		_iter != NULL && (_next = _iter->next, \
			p = wuy_containerof(_iter, typeof(*p), member)); \
		_iter = _next)

#endif
