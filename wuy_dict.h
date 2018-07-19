/**
 * @file     wuy_dict.h
 * @author   Wu Bingzheng <wubingzheng@gmail.com>
 * @date     2018-7-18
 *
 * @section LICENSE
 * GPLv2
 *
 * @section DESCRIPTION
 *
 * A general purpose dictionary, which is a part of libwuya.
 *
 * Unlike most other dictionary implementations where each
 * entry points to a key object and a value object, we keep
 * them together.
 * You put the key, value, and hash-node in your data struct
 * in order to use this.
 * See examples/dict.c for simple example.
 */

#ifndef WUY_DICT_H
#define WUY_DICT_H

#include <stdbool.h>
#include <stdint.h>

#include "wuy_hlist.h"

/**
 * @brief The dict.
 *
 * You should always use its pointer, and can not touch inside.
 */
typedef struct wuy_dict_s wuy_dict_t;

/**
 * @brief Embed this node into your data struct in order to use this lib.
 *
 * Pass its offset in your data struct to wuy_dict_new(), and
 * you need not use it later.
 *
 * You need not initialize it before using it.
 */
typedef wuy_hlist_node_t wuy_dict_node_t;

/**
 * @brief Return a 32 bit hash value for the key of item.
 */
typedef uint32_t wuy_dict_hash_f(const void *item);

/**
 * @brief Return if 2 items have the same key.
 */
typedef bool wuy_dict_equal_f(const char *a, const char *b);

/**
 * @brief Create a dict.
 *
 * @param hash return a 32 bit hash value for the key of item.
 * @param equal return if 2 items have the same key, used for searching.
 * @param node_offset the offset of wuy_dict_node_t in your data struct.
 *
 * @return the new dict. It aborts the program if memory allocation fails.
 */
wuy_dict_t *wuy_dict_new(wuy_dict_hash_f *hash, wuy_dict_equal_f *equal,
		size_t node_offset);

/**
 * @brief Destroy a dict.
 *
 * It just frees the dict, but do not releases the nodes.
 */
void wuy_dict_destroy(wuy_dict_t *dict);

/**
 * @brief Add the item into dict.
 */
void wuy_dict_add(wuy_dict_t *dict, void *item);

/**
 * @brief Search item from dict by the key.
 *
 * @return the item if found, or NULL.
 */
void *wuy_dict_get(wuy_dict_t *dict, const void *key);

/**
 * @brief Delete the item from dict.
 *
 * It just unlinks the item->hlist_node from its list,
 * but does not check if this item belongs to this dict.
 * So you MUST make sure that item is in this dict.
 *
 * Use wuy_dict_del_key() if you want to delete a key
 * from dict.
 */
void wuy_dict_delete(wuy_dict_t *dict, void *item);

/**
 * @brief Delete a key from dict.
 *
 * This is a combine of wuy_dict_get() and wuy_dict_delete().
 *
 * @return the item if found, or NULL.
 */
void *wuy_dict_del_key(wuy_dict_t *dict, const void *key);

/**
 * @brief Unlink the node from its dict, while you need not
 * know its dict.
 *
 * @warning Here we can not update dict->count which is used
 * for expansion, so you MUST call wuy_dict_disable_expasion()
 * before. Besides, you MUST NOT call wuy_dict_count() which
 * also uses dict->count.
 */
void wuy_dict_del_node(wuy_hlist_node_t *node);

/**
 * @brief Return the count of nodes in dict.
 */
long wuy_dict_count(wuy_dict_t *dict);

/**
 * @brief Disable expasion, and set the static bucket size.
 *
 * @note You MUST NOT call this after adding any node to the dict.
 */
void wuy_dict_disable_expasion(wuy_dict_t *dict, uint32_t bucket_size);

#endif
