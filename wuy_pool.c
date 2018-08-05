/*
 * pool       block           block
 *  +----+     +------+<--\    +------+<--\
 *  |head|---->|      |---+--->|      |   |
 *  |    |     |      |   |    |      |   |
 *  |    |     |      |   |    |      |   |
 *  +----+     +======+   |    +======+   |  
 *             |leader|--/     |leader|--/  
 *    return-->+ - - -|        + - - -|
 *             |      |        |      |    
 *             |      |        |      |   
 *             +------+        +------+  
 *             |leader|        |leader| 
 *             + - - -|        + - - -|
 *             |      |        |      |
 *             |      |        |      |
 *             +------+        +------+ 
 *             |      |        |      |
 *               ...             ...
 *             |      |        |      |
 *             +------+        +------+
 */

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <strings.h>

#include "wuy_hlist.h"
#include "wuy_pool.h"

struct wuy_pool_s {
	wuy_hlist_head_t	block_head;
	size_t			block_items;
	size_t			block_size;
	size_t			item_size;
};

typedef struct {
	wuy_hlist_node_t	block_node;
	wuy_hlist_head_t	item_head;
	wuy_pool_t		*pool;
	int			frees;
} wuy_pool_block_t;

static size_t _item_size_leader(wuy_pool_t *pool)
{
	return pool->item_size + sizeof(wuy_pool_block_t *);
}

wuy_pool_t *wuy_pool_new(size_t item_size)
{
	wuy_pool_t *pool = malloc(sizeof(wuy_pool_t));
	assert(pool != NULL);

	assert(item_size >= sizeof(wuy_hlist_node_t));
	pool->item_size = item_size;
	wuy_hlist_head_init(&pool->block_head);

	/* malloc use @brk if size<128K */
	pool->block_items = (128*1024 - sizeof(wuy_pool_block_t) - 24) / _item_size_leader(pool);
	return pool;
}

static wuy_pool_block_t *wuy_pool_block_new(wuy_pool_t *pool)
{
	wuy_pool_block_t *block = malloc(sizeof(wuy_pool_block_t)
			+ _item_size_leader(pool) * pool->block_items);
	if (block == NULL) {
		return NULL;
	}

	block->pool = pool;
	block->frees = pool->block_items;
	wuy_hlist_add(&block->block_node, &pool->block_head);
	wuy_hlist_head_init(&block->item_head);

	int i;
	uintptr_t leader = (uintptr_t)block + sizeof(wuy_pool_block_t);
	for (i = 0; i < pool->block_items; i++) {
		*((wuy_pool_block_t **)leader) = block;
		wuy_hlist_node_t *p = (wuy_hlist_node_t *)(leader + sizeof(wuy_pool_block_t *));
		wuy_hlist_add(p, &block->item_head);
		leader += _item_size_leader(pool);
	}

	return block;
}

void *wuy_pool_alloc(wuy_pool_t *pool)
{
	wuy_pool_block_t *block;

	if (wuy_hlist_empty(&pool->block_head)) {
		block = wuy_pool_block_new(pool);
		if (block == NULL) {
			return NULL;
		}
	} else {
		block = wuy_containerof(pool->block_head.first, wuy_pool_block_t, block_node);
	}

	wuy_hlist_node_t *node = block->item_head.first;
	wuy_hlist_delete(node);

	block->frees--;
	if (block->frees == 0) {
		/* if no free items, we throw the block away */
		wuy_hlist_delete(&block->block_node);
	}

	return node;
}

void wuy_pool_free(void *p)
{
	if (p == NULL) {
		return;
	}

	wuy_pool_block_t *block = *((wuy_pool_block_t **)p - 1);
	wuy_pool_t *pool = block->pool;

	bzero(p, pool->item_size);
	wuy_hlist_add((wuy_hlist_node_t *)p, &block->item_head);

	block->frees++;
	if (block->frees == 1) {
		/* if there WAS no free item in this block, we catch it again */
		wuy_hlist_add(&block->block_node, &pool->block_head);

	} else if (block->frees == pool->block_items && block->block_node.next) {
		/* never free the first pool-block for buffer */
		wuy_hlist_delete(&block->block_node);
		free(block);
	}
}
