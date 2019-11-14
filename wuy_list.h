/**
 * @file     wuy_list.h
 * @author   Wu Bingzheng <wubingzheng@gmail.com>
 * @date     2018-7-18
 *
 * @section LICENSE
 * GPLv2
 *
 * @section DESCRIPTION
 *
 * A doubly linked list, which is a part of libwuya.
 *
 * Copied from include/linux/list.h. We remove and rename some structs
 * and functions for consistent with the library.
 */

#ifndef WUY_LIST_H
#define WUY_LIST_H

#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>

#include "wuy_container.h"

/**
 * @brief Embed this node into your data struct.
 */
typedef struct wuy_list_node_s wuy_list_node_t;

struct wuy_list_node_s {
	wuy_list_node_t *next, *prev;
};

/**
 * @brief The list.
 */
typedef struct {
	wuy_list_node_t head;
} wuy_list_t;

/**
 * @brief Initialize a node at declare.
 */
#define WUY_LIST_NODE_INIT(name) { &(name), &(name) }

/**
 * @brief Initialize a list at declare.
 */
#define WUY_LIST_INIT(name) { .head = WUY_LIST_NODE_INIT(name.head) }

/**
 * @brief Declare and initialize a list.
 */
#define WUY_LIST(name) \
	wuy_list_t name = WUY_LIST_INIT(name)

/**
 * @brief Initialize a node.
 */
static inline void wuy_list_node_init(wuy_list_node_t *node)
{
	node->next = node;
	node->prev = node;
}

/**
 * @brief Initialize a list.
 */
static inline void wuy_list_init(wuy_list_t *list)
{
	wuy_list_node_init(&list->head);
}

static inline void _list_add(wuy_list_node_t *node,
	wuy_list_node_t *prev, wuy_list_node_t *next)
{
	node->next = next;
	node->prev = prev;
	// TODO add memory barrier here
	next->prev = node;
	prev->next = node;
}

/**
 * @brief Insert node to the begin of list.
 */
static inline void wuy_list_insert(wuy_list_t *list,
		wuy_list_node_t *node)
{
	_list_add(node, &list->head, list->head.next);
}

/**
 * @brief Append node to the end of list.
 */
static inline void wuy_list_append(wuy_list_t *list,
		wuy_list_node_t *node)
{
	_list_add(node, list->head.prev, &list->head);
}

/**
 * @brief Add node after the dest.
 */
static inline void wuy_list_add_after(wuy_list_node_t *dest,
		wuy_list_node_t *node)
{
	_list_add(node, dest, dest->next);
}

/**
 * @brief Add node before the dest.
 */
static inline void wuy_list_add_before(wuy_list_node_t *dest,
		wuy_list_node_t *node)
{
	_list_add(node, dest->prev, dest);
}

static inline void _list_del(wuy_list_node_t *prev,
		wuy_list_node_t *next)
{
	next->prev = prev;
	prev->next = next;
}

/**
 * @brief Delete node from its list.
 */
static inline void wuy_list_delete(wuy_list_node_t *node)
{
	_list_del(node->prev, node->next);
	node->next = node->prev = NULL;
}

/**
 * @brief Delete node from its list, and initialize it.
 */
static inline void wuy_list_del_init(wuy_list_node_t *node)
{
	_list_del(node->prev, node->next);
	wuy_list_node_init(node); 
}

/**
 * @brief Return if the list head is empty.
 */
static inline bool wuy_list_empty(const wuy_list_t *list)
{
	const wuy_list_node_t *head = &list->head;
	return head->next == head;
}

/**
 * @brief Return first node, or NULL if empty.
 */
static inline wuy_list_node_t *wuy_list_first(const wuy_list_t *list)
{
	return wuy_list_empty(list) ? NULL : list->head.next;
}

/**
 * @brief Return last node, or NULL if empty.
 */
static inline wuy_list_node_t *wuy_list_last(const wuy_list_t *list)
{
	return wuy_list_empty(list) ? NULL : list->head.prev;
}

/**
 * @brief Iterate over a list, while it's NOT safe to delete node.
 */
#define wuy_list_iter(list, node) \
	for (node = (list)->head.next; node != &((list)->head); \
			node = node->next)

/**
 * @brief Iterate over a list, while it's safe to delete node.
 */
#define wuy_list_iter_safe(list, node, safe) \
	for (node = (list)->head.next, safe = node->next; \
			node != &((list)->head); \
			node = safe, safe = node->next)

/**
 * @brief Iterate over a list reverse, while it's NOT safe to delete node.
 */
#define wuy_list_iter_reverse(list, node) \
	for (node = (list)->head.prev; node != &((list)->head); \
			node = node->prev)

/**
 * @brief Iterate over a list reverse, while it's safe to delete node.
 */
#define wuy_list_iter_reverse_safe(list, node, safe) \
	for (node = (list)->head.prev, safe = node->prev; \
			node != &((list)->head); \
			node = safe, safe = node->prev)

/**
 * @brief Iterate over a list, always getting the first.
 */
#define wuy_list_iter_first(list, node) \
	while ((node = (list)->head.next) != &((list)->head))

#endif
