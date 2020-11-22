/**
 * @file     wuy_nop_hlist.h
 * @author   Wu Bingzheng <wubingzheng@gmail.com>
 * @date     2020-11-21
 *
 * @section LICENSE
 * GPLv2
 *
 * @section DESCRIPTION
 *
 * This is similar to wuy_nop_hlist.h without pointer.
 * It uses relative offset as pointer. So it's suitable for shared-memory
 * mapped for different addresses amount processes.
 */

#ifndef WUY_NOP_HLIST_H
#define WUY_NOP_HLIST_H

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

#include "wuy_container.h"

typedef uint32_t wuy_nop_hlist_addr_t;

/**
 * @brief The list.
 */
typedef struct {
	wuy_nop_hlist_addr_t	first;
} wuy_nop_hlist_t;

/**
 * @brief Embed this node into your data struct in order to use this lib.
 */
typedef struct {
	wuy_nop_hlist_addr_t	next;
	wuy_nop_hlist_addr_t	pprev;
} wuy_nop_hlist_node_t;

/**
 * @brief Initialize a list.
 */
static inline void wuy_nop_hlist_init(wuy_nop_hlist_t *list)
{
	list->first = 0;
}

/**
 * @brief Return if the list is empty.
 */
static inline int wuy_nop_hlist_empty(const wuy_nop_hlist_t *list)
{
	return list->first == 0;
}

static inline wuy_nop_hlist_addr_t _nop_hlist_2addr(const wuy_nop_hlist_node_t *node,
		const wuy_nop_hlist_t *list)
{
	return (uintptr_t)node - (uintptr_t)list;
}
static inline wuy_nop_hlist_node_t *_nop_hlist_2node(wuy_nop_hlist_addr_t addr,
		const wuy_nop_hlist_t *list)
{
	return (wuy_nop_hlist_node_t *)((char *)list + addr);
}

/**
 * @brief Add node after head.
 */
static inline void wuy_nop_hlist_insert(wuy_nop_hlist_t *list, wuy_nop_hlist_node_t *node)
{
	wuy_nop_hlist_addr_t first_addr = list->first;
	wuy_nop_hlist_addr_t node_addr = _nop_hlist_2addr(node, list);

	node->next = first_addr;
	if (first_addr != 0) {
		wuy_nop_hlist_node_t *first = _nop_hlist_2node(first_addr, list);
		first->pprev = node_addr + offsetof(wuy_nop_hlist_node_t, next);
	}
	list->first = node_addr;
	node->pprev = 0;
}

/**
 * @brief Delete node from its list.
 */
static inline void wuy_nop_hlist_delete(wuy_nop_hlist_t *list, wuy_nop_hlist_node_t *node)
{
	if (node->next != 0) {
		wuy_nop_hlist_node_t *next = _nop_hlist_2node(node->next, list);
		next->pprev = node->pprev;
	}

	wuy_nop_hlist_addr_t *pprev = (wuy_nop_hlist_addr_t *)_nop_hlist_2node(node->pprev, list);
	*pprev = node->next;
}

/**
 * @brief Iterate over a list, while it's NOT safe to delete node.
 */
#define wuy_nop_hlist_iter(list, pos) \
	for (pos = _nop_hlist_2node((list)->first, list); \
			pos != (wuy_nop_hlist_node_t *)list; \
			pos = _nop_hlist_2node(pos->next, list))

/**
 * @brief Iterate over a list, while it's safe to delete node.
 */
#define wuy_nop_hlist_iter_safe(list, pos, n) \
	for (pos = _nop_hlist_2node((list)->first, list), n = _nop_hlist_2node(pos->next, list); \
			pos != (wuy_nop_hlist_node_t *)list; \
			pos = n, n = _nop_hlist_2node(pos->next, list))

/**
 * @brief Iterate over a list. It's safe to delete node during it.
 *
 * @param p  a pointer of user's struct type;
 * @param member  name of wuy_nop_hlist_node_t member in user's struct.
 *
 * Compared to @wuy_nop_hlist_iter_safe, this macro transfer hlist node
 * to user's struct type pointer. This it more convenient.
 * Use @wuy_nop_hlist_iter and @wuy_nop_hlist_iter_safe only when you do not
 * know the container's type, e.g. in wuy_dict.c .
 */
#define wuy_nop_hlist_iter_type(list, p, member) \
	for (wuy_nop_hlist_addr_t _next_addr, _iter_addr = (list)->first; \
		_iter_addr != 0 && (_next_addr = _iter_addr->next, \
			p = wuy_containerof(_nop_hlist_2node(_iter_addr, list), typeof(*p), member)); \
		_iter_addr = _next_addr)

#endif
