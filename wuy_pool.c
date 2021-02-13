#include <stdlib.h>
#include <stdio.h>

#include "wuy_pool.h"

struct wuy_pool_block {
	struct wuy_pool_block	*next;
	size_t			offset;
	char			data[0];
};

struct wuy_pool_big {
	struct wuy_pool_big	*next;
	void			*data;
};

struct wuy_pool_free {
	struct wuy_pool_free	*next;
	void			(*handler)(void *);
	void			*data;
};

struct wuy_pool {
	size_t			block_size;
	struct wuy_pool_block	*block_head;
	struct wuy_pool_big	*big_head;
	struct wuy_pool_big	*realloc_head;
	struct wuy_pool_free	*free_head;
};

wuy_pool_t *wuy_pool_new(size_t block_size)
{
	wuy_pool_t *pool = calloc(1, sizeof(struct wuy_pool));
	pool->block_size = block_size;
	return pool;
}

void wuy_pool_release(wuy_pool_t *pool)
{
	for (struct wuy_pool_free *fr = pool->free_head; fr != NULL; fr = fr->next) {
		fr->handler(fr->data);
	}
	for (struct wuy_pool_big *big = pool->big_head; big != NULL; big = big->next) {
		free(big->data);
	}
	for (struct wuy_pool_big *big = pool->realloc_head; big != NULL; big = big->next) {
		free(big->data);
	}
	for (struct wuy_pool_block *next, *block = pool->block_head; block != NULL; block = next) {
		next = block->next;
		free(block);
	}
	free(pool);
}

static struct wuy_pool_block *wuy_pool_block_new(wuy_pool_t *pool)
{
	struct wuy_pool_block *block = calloc(1, pool->block_size);
	block->next = pool->block_head;
	block->offset = sizeof(struct wuy_pool_block);
	pool->block_head = block;
	return block;
}

static void *wuy_pool_alloc_big(wuy_pool_t *pool, size_t size, struct wuy_pool_big **head)
{
	void *data = calloc(1, size);

	struct wuy_pool_big *big = wuy_pool_alloc(pool, sizeof(struct wuy_pool_big));
	big->next = *head;
	big->data = data;
	*head = big;

	return data;
}

void *wuy_pool_alloc_align(wuy_pool_t *pool, size_t size, size_t align)
{
	if (size >= pool->block_size / 2) {
		return wuy_pool_alloc_big(pool, size, &pool->big_head);
	}

	struct wuy_pool_block *block = pool->block_head;
	if (block == NULL) {
		block = wuy_pool_block_new(pool);
	} else {
		if ((block->offset % align) != 0) {
			block->offset += align - (block->offset % align);
		}
		if (block->offset + size > pool->block_size) {
			block = wuy_pool_block_new(pool);
		}
	}

	char *ret = (char *)block + block->offset;
	block->offset += size;
	return ret;
}

void *wuy_pool_alloc(wuy_pool_t *pool, size_t size)
{
	return wuy_pool_alloc_align(pool, size, sizeof(void *));
}

void *wuy_pool_realloc(wuy_pool_t *pool, void *old, size_t size)
{
	if (old == NULL) {
		return wuy_pool_alloc_big(pool, size, &pool->realloc_head);
	}
	for (struct wuy_pool_big *big = pool->realloc_head; big != NULL; big = big->next) {
		if (big->data == old) {
			big->data = realloc(old, size);
			return big->data;
		}
	}
	abort();
}

void wuy_pool_add_free(wuy_pool_t *pool, void (*handler)(void *), void *data)
{
	struct wuy_pool_free *fr = wuy_pool_alloc(pool, sizeof(struct wuy_pool_free));
	fr->handler = handler;
	fr->data = data;
	fr->next = pool->free_head;
	pool->free_head = fr;
}
