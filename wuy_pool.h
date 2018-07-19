/**
 * @file     wuy_pool.h
 * @author   Wu Bingzheng <wubingzheng@gmail.com>
 * @date     2018-7-19
 *
 * @section LICENSE
 * GPLv2
 *
 * @section DESCRIPTION
 *
 * A memory pool for fixed-size item, which is a part of libwuya.
 */

#ifndef WUY_POOL_H
#define WUY_POOL_H

/**
 * @brief The memory pool.
 *
 * You should always use its pointer, and can not touch inside.
 */
typedef struct wuy_pool_s wuy_pool_t;

/**
 * @brief Create a new memory pool by the item type.
 */
#define wuy_pool_new_type(type) wuy_pool_new(sizeof(type))

/**
 * @brief Create a new memory pool by the item size.
 */
wuy_pool_t *wuy_pool_new(size_t item_size);

/**
 * @brief Allocate an item from pool.
 */
void *wuy_pool_alloc(wuy_pool_t *pool);

/**
 * @brief Return the item back to pool.
 */
void wuy_pool_free(void *p);

#endif
