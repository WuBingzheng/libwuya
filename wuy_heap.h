/**
 * @file     wuy_heap.h
 * @author   Wu Bingzheng <wubingzheng@gmail.com>
 * @date     2018-7-19
 *
 * @section LICENSE
 * GPLv2
 *
 * @section DESCRIPTION
 *
 * A binary heap, which is a part of libwuya.
 */

#ifndef WUY_HEAP_H
#define WUY_HEAP_H

#include <stdbool.h>
#include <stddef.h>

/**
 * @brief The heap.
 *
 * You should always use its pointer, and can not touch inside.
 */
typedef struct wuy_heap_s wuy_heap_t;

/**
 * @brief Embed this node into your data struct in order to use this lib.
 *
 * Pass its offset in your data struct to wuy_heap_new_xxx(), and
 * you need not use it later.
 *
 * You need not initialize it before using it.
 */
typedef struct {
	size_t index;
} wuy_heap_node_t;

/**
 * @brief Return if the former is less than latter.
 *
 * Used in wuy_heap_new_func().
 */
typedef bool wuy_heap_less_f(const void *, const void *);

/**
 * @brief List of your item's comparison key type.
 *
 * Used in wuy_heap_new_type().
 */
typedef enum {
	WUY_HEAP_KEY_INT32,
	WUY_HEAP_KEY_UINT32,
	WUY_HEAP_KEY_INT64,
	WUY_HEAP_KEY_UINT64,
	WUY_HEAP_KEY_FLOAT,
	WUY_HEAP_KEY_DOUBLE,
	WUY_HEAP_KEY_STRING,
} wuy_heap_key_type_e;

/**
 * @brief Create a new heap, with the user-defined comparison function.
 *
 * @param key_less use-defined compare function.
 * @param node_offset the offset of wuy_heap_node_t in your data struct.
 *
 * @return the new heap. It aborts the program if memory allocation fails.
 */
wuy_heap_t *wuy_heap_new_func(wuy_heap_less_f *key_less, size_t node_offset);

/**
 * @brief Create a new heap, with general comparison key type.
 *
 * @param key_type see wuy_heap_key_type_e.
 * @param key_offset the offset of key in your data struct.
 * @param key_reverse if reverse the comparison.
 * @param node_offset the offset of wuy_heap_node_t in your data struct.
 *
 * @return the new heap. It aborts the program if memory allocation fails.
 */
wuy_heap_t *wuy_heap_new_type(wuy_heap_key_type_e key_type, size_t key_offset,
		bool key_reverse, size_t node_offset);

/**
 * @brief Add an item into the heap.
 *
 * @note: It aborts if item is in the heap already.
 *
 * @return true if success, or false if memory allocation fails.
 */
bool wuy_heap_push(wuy_heap_t *heap, void *item);

/**
 * @brief Pop and return the min item.
 */
void *wuy_heap_pop(wuy_heap_t *heap);

/**
 * @brief Fix an item in the heap.
 *
 * @note: It aborts if item is not in the heap yet.
 */
void wuy_heap_fix(wuy_heap_t *heap, void *item);

/**
 * @brief Push item into heap if not exist yet, or fix it.
 *
 * @return true if success, or false if memory allocation fails.
 */
bool wuy_heap_push_or_fix(wuy_heap_t *heap, void *item);

/**
 * @brief Return the min item but not delete it.
 */
void *wuy_heap_min(wuy_heap_t *heap);

/**
 * @brief Delete the item from heap.
 *
 * @return true if success, or false if the item is not in the heap.
 */
bool wuy_heap_delete(wuy_heap_t *heap, void *item);

/**
 * @brief Return the count of items in heap.
 */
size_t wuy_heap_count(wuy_heap_t *heap);

#endif
