#ifndef WUY_SHMPOOL_H
#define WUY_SHMPOOL_H

#include <stdint.h>

typedef struct wuy_shmpool wuy_shmpool_t;

wuy_shmpool_t *wuy_shmpool_new(const char *name, size_t small_size,
		size_t big_size, int big_max);

void wuy_shmpool_destroy(wuy_shmpool_t *pool);

void *wuy_shmpool_alloc(size_t size);

void wuy_shmpool_finish(wuy_shmpool_t *pool);

void wuy_shmpool_cleanup(void);

#endif
