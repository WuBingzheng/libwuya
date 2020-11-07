#ifndef WUY_SHMPOOL_H
#define WUY_SHMPOOL_H

#include <stdint.h>
#include <stdbool.h>

void wuy_shmpool_init(void);

void wuy_shmpool_new(uint64_t key);
void wuy_shmpool_release(uint64_t key);

void *wuy_shmpool_alloc(size_t size);
bool wuy_shmpool_check(void);

#endif
