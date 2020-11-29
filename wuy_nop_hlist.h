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
		const void *base)
{
	return (uintptr_t)node - (uintptr_t)base;
}
static inline wuy_nop_hlist_node_t *_nop_hlist_2node(wuy_nop_hlist_addr_t addr,
		const void *base)
{
	return (wuy_nop_hlist_node_t *)((char *)base + addr);
}

/**
 * @brief Add node after head.
 */
static inline void wuy_nop_hlist_insert(wuy_nop_hlist_t *list,
		wuy_nop_hlist_node_t *node, const void *base)
{
	wuy_nop_hlist_addr_t first_addr = list->first;
	wuy_nop_hlist_addr_t node_addr = _nop_hlist_2addr(node, base);

	node->next = first_addr;
	if (first_addr != 0) {
		wuy_nop_hlist_node_t *first = _nop_hlist_2node(first_addr, base);
		first->pprev = node_addr + offsetof(wuy_nop_hlist_node_t, next);
	}
	list->first = node_addr;
	node->pprev = 0;
}

/**
 * @brief Delete node from its list.
 */
static inline void wuy_nop_hlist_delete(wuy_nop_hlist_node_t *node, const void *base)
{
	if (node->next != 0) {
		wuy_nop_hlist_node_t *next = _nop_hlist_2node(node->next, base);
		next->pprev = node->pprev;
	}

	wuy_nop_hlist_addr_t *pprev = (wuy_nop_hlist_addr_t *)_nop_hlist_2node(node->pprev, base);
	*pprev = node->next;
}

/**
 * @brief Iterate over a list, while it's NOT safe to delete node.
 */
#define wuy_nop_hlist_iter(list, pos, base) \
	for (pos = _nop_hlist_2node((list)->first, base); pos != base; \
			pos = _nop_hlist_2node(pos->next, base))

/**
 * @brief Iterate over a list, while it's safe to delete node.
 */
#define wuy_nop_hlist_iter_safe(list, pos, n, base) \
	for (pos = _nop_hlist_2node((list)->first, base), n = _nop_hlist_2node(pos->next, base); \
			pos != base; \
			pos = n, n = _nop_hlist_2node(pos->next, base))

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
#define wuy_nop_hlist_iter_type(list, p, member, base) \
	for (wuy_nop_hlist_node_t *_node = _nop_hlist_2node((list)->first, base); \
			_node != (void *)base && (p = wuy_containerof(_node, typeof(*p), member)); \
			_node = _nop_hlist_2node(_node->next, base))

#endif
