/**
 * @file     wuy_slist.h
 * @author   Wu Bingzheng <wubingzheng@gmail.com>
 * @date     2020-7-18
 *
 * @section LICENSE
 * GPLv2
 *
 * @section DESCRIPTION
 *
 * A singly linked list, which is a part of libwuya.
 */

#ifndef WUY_SLIST_H
#define WUY_SLIST_H

#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>

#include "wuy_container.h"

/**
 * @brief Embed this node into your data struct.
 */
typedef struct wuy_slist_node_s wuy_slist_node_t;

struct wuy_slist_node_s {
	wuy_slist_node_t *next;
};

/**
 * @brief The list.
 */
typedef struct {
	wuy_slist_node_t *first;
	wuy_slist_node_t *last;
} wuy_slist_t;

/**
 * @brief Initialize a list.
 */
static inline void wuy_slist_init(wuy_slist_t *list)
{
	list->first = NULL;
	list->last = NULL;
}

/**
 * @brief Return if the list is empty.
 */
static inline bool wuy_slist_empty(const wuy_slist_t *list)
{
	return list->first == NULL;
}

/**
 * @brief Insert node to the begin of list.
 */
static inline void wuy_slist_insert(wuy_slist_t *list,
		wuy_slist_node_t *node)
{
	node->next = list->first;
	list->first = node;
	if (list->last == NULL) {
		list->last = node;
	}
}

/**
 * @brief Append node to the end of list.
 */
static inline void wuy_slist_append(wuy_slist_t *list,
		wuy_slist_node_t *node)
{
	if (list->first == NULL) {
		list->first = node;
	} else {
		list->last->next = node;
	}
	node->next = NULL;
	list->last = node;
}

/**
 * @brief Return first node, or NULL if empty.
 */
static inline wuy_slist_node_t *wuy_slist_first(const wuy_slist_t *list)
{
	return list->first;
}

/**
 * @brief Return last node, or NULL if empty.
 */
static inline wuy_slist_node_t *wuy_slist_last(const wuy_slist_t *list)
{
	return list->last;
}

/**
 * @brief Remove and return first node, or NULL if empty.
 */
static inline wuy_slist_node_t *wuy_slist_pop(wuy_slist_t *list)
{
        if (wuy_slist_empty(list)) {
                return NULL;
        }
        wuy_slist_node_t *node = list->first;
	list->first = node->next;
	if (node->next == NULL) {
		list->last = NULL;
	} else {
		node->next = NULL;
	}
        return node;
}

#define wuy_slist_pop_type(list, p, member) \
	(p = wuy_containerof_check(wuy_slist_pop(list), typeof(*p), member))

/**
 * @brief Iterate over a list.
 */
#define wuy_slist_iter(list, node) \
	for (node = (list)->first; node != NULL; node = node->next)

/**
 * @brief Iterate over a list.
 *
 * @param pprev  (type: wuy_slist_node_t**) used for wuy_slist_delete().
 */
#define wuy_slist_iter_prev(list, node, pprev) \
	for (pprev = &((list)->first); \
			(node = *pprev) != NULL; \
			pprev = (*pprev == node) ? &(node->next) : pprev)

/**
 * @brief Iterate over a list.
 *
 * @param p  a pointer of user's struct type;
 * @param member  name of wuy_slist_node_t member in user's struct.
 */
#define wuy_slist_iter_type(list, p, member) \
	for (p = wuy_containerof((list)->first, typeof(*p), member); \
			p->member.next != NULL; \
			p = wuy_containerof(p->member.next, typeof(*p), member))


/**
 * @brief Iterate over a list.
 *
 * @param p  a pointer of user's struct type;
 * @param member  name of wuy_slist_node_t member in user's struct.
 * @param pprev  (type: wuy_slist_node_t**) used for wuy_slist_delete().
 */
#define wuy_slist_iter_prev_type(list, p, member, pprev) \
	for (pprev = &((list)->first); \
			*pprev != NULL && (p = wuy_containerof(*pprev, typeof(*p), member)); \
			pprev = (*pprev == node) ? &(node->next) : pprev)

/**
 * @brief Delete node from its list.
 *
 * @param pprev  got from wuy_slist_iter_prev or wuy_slist_iter_prev_type.
 */
static inline void wuy_slist_delete(wuy_slist_t *list,
		wuy_slist_node_t *node, wuy_slist_node_t **pprev)
{
	*pprev = node->next;
	node->next = NULL;

	if (list->last == node) {
		list->last = (pprev == &(list->first)) ? NULL
			: wuy_containerof(pprev, typeof(wuy_slist_node_t), next);
	}
}

#endif
