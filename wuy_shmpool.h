#ifndef WUY_SHMPOOL_H
#define WUY_SHMPOOL_H

#include <stdint.h>

typedef struct wuy_shmpool wuy_shmpool_t;

void wuy_shmpool_init(void);

wuy_shmpool_t *wuy_shmpool_new(uint64_t key);
void wuy_shmpool_release(wuy_shmpool_t *pool);

void *wuy_shmpool_alloc(size_t size);
void wuy_shmpool_finish(wuy_shmpool_t *pool);

#endif
