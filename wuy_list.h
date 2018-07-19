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

/**
 * @brief Embed this node into your data struct in order to use this lib.
 *
 * This can used as list head or node both.
 */
typedef struct wuy_list_head_s wuy_list_head_t;

struct wuy_list_head_s {
	wuy_list_head_t *next, *prev;
};

/**
 * @brief Initialize at declare.
 */
#define WUY_LIST_HEAD_INIT(name) { &(name), &(name) }

/**
 * @brief Declare and initialize.
 */
#define WUY_LIST_HEAD(name) \
	wuy_list_head_t name = WUY_LIST_HEAD_INIT(name)

/**
 * @brief Initialize a head or node.
 */
static inline void wuy_list_init(wuy_list_head_t *head)
{
	head->next = head;
	head->prev = head;
}

static inline void __list_add(wuy_list_head_t *node,
	wuy_list_head_t *prev, wuy_list_head_t *next)
{
	node->next = next;
	node->prev = prev;
	// TODO add memory barrier here
	next->prev = node;
	prev->next = node;
}

/**
 * @brief Add node after head.
 */
static inline void wuy_list_add(wuy_list_head_t *node,
		wuy_list_head_t *head)
{
	__list_add(node, head, head->next);
}

/**
 * @brief Add node before head.
 */
static inline void wuy_list_add_tail(wuy_list_head_t *node,
		wuy_list_head_t *head)
{
	__list_add(node, head->prev, head);
}

static inline void __list_del(wuy_list_head_t *prev,
		wuy_list_head_t *next)
{
	next->prev = prev;
	prev->next = next;
}

/**
 * @brief Delete node from its list.
 */
static inline void wuy_list_delete(wuy_list_head_t *node)
{
	__list_del(node->prev, node->next);
	node->next = node->prev = NULL;
}

/**
 * @brief Delete node from its list, and initialize it.
 */
static inline void wuy_list_del_init(wuy_list_head_t *node)
{
	__list_del(node->prev, node->next);
	wuy_list_init(node); 
}

/**
 * @brief Return if the list head is empty.
 */
static inline int wuy_list_empty(wuy_list_head_t *head)
{
	return head->next == head;
}

/**
 * @brief Iterate over a list, while it's NOT safe to delete node.
 */
#define wuy_list_iter(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

/**
 * @brief Iterate over a list, while it's safe to delete node.
 */
#define wuy_list_iter_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); \
		pos = n, n = pos->next)

/**
 * @brief Iterate over a list reverse, while it's NOT safe to delete node.
 */
#define wuy_list_iter_reverse(pos, head) \
	for (pos = (head)->prev; pos != (head); pos = pos->prev)

/**
 * @brief Iterate over a list reverse, while it's safe to delete node.
 */
#define wuy_list_iter_reverse_safe(pos, n, head) \
	for (pos = (head)->prev, n = pos->prev; pos != (head); \
		pos = n, n = pos->prev)

#endif
