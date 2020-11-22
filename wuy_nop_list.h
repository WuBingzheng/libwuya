/**
 * @file     wuy_nop_list.h
 * @author   Wu Bingzheng <wubingzheng@gmail.com>
 * @date     2020-11-21
 *
 * @section LICENSE
 * GPLv2
 *
 * @section DESCRIPTION
 *
 * This is similar to wuy_list.h without pointer.
 * It uses relative offset as pointer. So it's suitable for shared-memory
 * mapped for different addresses amount processes.
 */

#ifndef WUY_NOP_LIST_H
#define WUY_NOP_LIST_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "wuy_container.h"

typedef uint32_t wuy_nop_list_addr_t;


/**
 * @brief Embed this node into your data struct.
 */
typedef struct {
	wuy_nop_list_addr_t	next;
	wuy_nop_list_addr_t	prev;
} wuy_nop_list_node_t;

/**
 * @brief The list.
 */
typedef struct {
	wuy_nop_list_node_t head;
} wuy_nop_list_t;


/**
 * @brief Initialize a list.
 */
static inline void wuy_nop_list_init(wuy_nop_list_t *list)
{
	list->head.next = 0;
	list->head.prev = 0;
}

static inline wuy_nop_list_addr_t _nop_list_2addr(const wuy_nop_list_node_t *node,
		const wuy_nop_list_t *list)
{
	return (uintptr_t)node - (uintptr_t)list;
}
static inline wuy_nop_list_node_t *_nop_list_2node(wuy_nop_list_addr_t addr,
		const wuy_nop_list_t *list)
{
	return (wuy_nop_list_node_t *)((char *)list + addr);
}

static inline void _nop_list_add(wuy_nop_list_node_t *node,
	wuy_nop_list_addr_t prev_addr, wuy_nop_list_addr_t next_addr,
	wuy_nop_list_t *list)
{
	wuy_nop_list_node_t *prev = _nop_list_2node(prev_addr, list);
	wuy_nop_list_node_t *next = _nop_list_2node(next_addr, list);
	wuy_nop_list_addr_t node_addr = _nop_list_2addr(node, list);

	node->next = next_addr,
	node->prev = prev_addr;
	next->prev = node_addr;
	prev->next = node_addr;
}

/**
 * @brief Insert node to the begin of list.
 */
static inline void wuy_nop_list_insert(wuy_nop_list_t *list,
		wuy_nop_list_node_t *node)
{
	_nop_list_add(node, 0, list->head.next, list);
}

/**
 * @brief Append node to the end of list.
 */
static inline void wuy_nop_list_append(wuy_nop_list_t *list,
		wuy_nop_list_node_t *node)
{
	_nop_list_add(node, list->head.prev, 0, list);
}

static inline void _nop_list_del(wuy_nop_list_addr_t prev_addr,
		wuy_nop_list_addr_t next_addr, wuy_nop_list_t *list)
{
	wuy_nop_list_node_t *prev = _nop_list_2node(prev_addr, list);
	wuy_nop_list_node_t *next = _nop_list_2node(next_addr, list);

	next->prev = prev_addr;
	prev->next = next_addr;
}

/**
 * @brief Delete node from its list.
 */
static inline void wuy_nop_list_delete(wuy_nop_list_t *list, wuy_nop_list_node_t *node)
{
	_nop_list_del(node->prev, node->next, list);
	node->next = node->prev = 0;
}

/**
 * @brief Return if the list head is empty.
 */
static inline bool wuy_nop_list_empty(const wuy_nop_list_t *list)
{
	return list->head.next == 0;
}

/**
 * @brief Return first node, or NULL if empty.
 */
static inline wuy_nop_list_node_t *wuy_nop_list_first(const wuy_nop_list_t *list)
{
	return wuy_nop_list_empty(list) ? NULL : _nop_list_2node(list->head.next, list);
}

#define wuy_nop_list_first_type(list, p, member) \
	(p = wuy_containerof_check(wuy_nop_list_first(list), typeof(*p), member))

/**
 * @brief Remove and return first node, or NULL if empty.
 */
static inline wuy_nop_list_node_t *wuy_nop_list_pop(wuy_nop_list_t *list)
{
	if (wuy_nop_list_empty(list)) {
		return NULL;
	}
	wuy_nop_list_node_t *node = _nop_list_2node(list->head.next, list);
	wuy_nop_list_delete(list, node);
	return node;
}

#define wuy_nop_list_pop_type(list, p, member) \
	(p = wuy_containerof_check(wuy_nop_list_pop(list), typeof(*p), member))

/**
 * @brief Return last node, or NULL if empty.
 */
static inline wuy_nop_list_node_t *wuy_nop_list_last(const wuy_nop_list_t *list)
{
	return wuy_nop_list_empty(list) ? NULL : _nop_list_2node(list->head.prev, list);
}

/**
 * @brief Iterate over a list.
 *
 * If you may delete node during iteration and continue the iteration,
 * use @wuy_nop_list_iter_safe instead.
 */
#define wuy_nop_list_iter(list, node) \
	for (node = _nop_list_2node((list)->head.next, list); node != &((list)->head); \
			node = _nop_list_2node(node->next, list))

/**
 * @brief Iterate over a list, while it's safe to delete node.
 */
#define wuy_nop_list_iter_safe(list, node, safe) \
	for (node = _nop_list_2node((list)->head.next, list), safe = _nop_list_2node(node->next, list); \
			node != &((list)->head); \
			node = safe, safe = _nop_list_2node(node->next, list))

/**
 * @brief Iterate over a list reverse.
 *
 * If you may delete node during iteration and continue the iteration,
 * use @wuy_nop_list_iter_safe instead.
 */
#define wuy_nop_list_iter_reverse(list, node) \
	for (node = _nop_list_2node((list)->head.prev, list); node != &((list)->head); \
			node = _nop_list_2node(node->prev, list))

/**
 * @brief Iterate over a list reverse, while it's safe to delete node.
 */
#define wuy_nop_list_iter_reverse_safe(list, node, safe) \
	for (node = _nop_list_2node((list)->head.prev, list), safe = _nop_list_2node(node->prev, list); \
			node != &((list)->head); \
			node = safe, safe = _nop_list_2node(node->prev, list))

/* used internal */
#define wuy_nop_list_next_type(p, member, list) \
	wuy_containerof(_nop_list_2node(p->member.next, list), typeof(*p), member)
#define wuy_nop_list_prev_type(p, member, list) \
	wuy_containerof(_nop_list_2node(p->member.prev, list), typeof(*p), member)

/**
 * @brief Iterate over a list.
 *
 * @param p  a pointer of user's struct type;
 * @param member  name of wuy_nop_list_node_t member in user's struct.
 */
#define wuy_nop_list_iter_type(list, p, member) \
	for (p = wuy_containerof(_nop_list_2node((list)->head.next, list), typeof(*p), member);\
			&p->member != &(list)->head; \
			p = wuy_nop_list_next_type(p, member, list))

/**
 * @brief Iterate over a list, while it's safe to delete node.
 *
 * @param p  a pointer of user's struct type;
 * @param member  name of wuy_nop_list_node_t member in user's struct.
 */
#define wuy_nop_list_iter_safe_type(list, p, safe, member) \
	for (p = wuy_containerof(_nop_list_2node((list)->head.next, list), typeof(*p), member), \
				safe = wuy_nop_list_next_type(p, member, list); \
			&p->member != &(list)->head; \
			p = safe, safe = wuy_nop_list_next_type(p, member, list))

/**
 * @brief Iterate over a list reverse.
 *
 * @param p  a pointer of user's struct type;
 * @param member  name of wuy_nop_list_node_t member in user's struct.
 */
#define wuy_nop_list_iter_reverse_type(list, p, member) \
	for (p = wuy_containerof(_nop_list_2node((list)->head.prev, list), typeof(*p), member);\
			&p->member != &(list)->head; \
			p = wuy_nop_list_prev_type(p, member, list))

/**
 * @brief Iterate over a list reverse, while it's safe to delete node.
 *
 * @param p  a pointer of user's struct type;
 * @param member  name of wuy_nop_list_node_t member in user's struct.
 */
#define wuy_nop_list_iter_reverse_safe_type(list, p, safe, member) \
	for (p = wuy_containerof(_nop_list_2node((list)->head.prev, list), typeof(*p), member), \
				safe = wuy_nop_list_prev_type(p, member, list); \
			&p->member != &(list)->head; \
			p = safe, safe = wuy_nop_list_prev_type(p, member, list))

#endif
