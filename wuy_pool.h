#ifndef WUY_POOL_H
#define WUY_POOL_H

typedef struct wuy_pool wuy_pool_t;

wuy_pool_t *wuy_pool_new(size_t block_size);

void wuy_pool_destroy(wuy_pool_t *pool);

void *wuy_pool_alloc_align(wuy_pool_t *pool, size_t size, size_t align);

void *wuy_pool_alloc(wuy_pool_t *pool, size_t size);

void *wuy_pool_realloc(wuy_pool_t *pool, void *old, size_t size);

void wuy_pool_add_free(wuy_pool_t *pool, void (*handler)(void *), void *data);


#include <string.h>
static inline char *wuy_pool_strndup(wuy_pool_t *pool, const char *s, int len)
{
	char *d = wuy_pool_alloc_align(pool, len + 1, 1);
	return memcpy(d, s, len);
}
static inline char *wuy_pool_strdup(wuy_pool_t *pool, const char *s)
{
	return wuy_pool_strndup(pool, s, strlen(s));
}

#endif
