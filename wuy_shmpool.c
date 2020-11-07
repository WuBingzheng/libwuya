#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/mman.h>

#include "wuy_shmpool.h"

#define WUY_SHMPOOL_PAGE_SIZE	4096
#define WUY_SHMPOOL_NHASH	1024
#define WUY_SHMPOOL_MAX		10000
#define WUY_SHMPOOL_SIZE	64*1024

struct wuy_shmpool {
	uint64_t		key;
	int			refs;
	struct wuy_shmpool	*hash_next;
	char			buffer[WUY_SHMPOOL_SIZE];
};

static pthread_mutex_t wuy_shmpool_lock;
static struct wuy_shmpool *wuy_shmpools;
static struct wuy_shmpool **wuy_shmpool_hash_buckets;
static struct wuy_shmpool *wuy_shmpool_current = NULL;
static char *wuy_shmpool_current_pos = NULL;

static void *wuy_shmpool_alloc_permanent(size_t size)
{
	static char *last_addr = NULL;
	static size_t last_size = 0;

	if (size > last_size) {
		last_size = size > WUY_SHMPOOL_PAGE_SIZE ? size : WUY_SHMPOOL_PAGE_SIZE;
		last_addr = mmap(NULL, last_size, PROT_READ | PROT_WRITE,
				MAP_ANONYMOUS | MAP_SHARED, -1, 0);
	}

	void *ret = last_addr;
	last_addr += size;
	last_size -= size;
	return ret;
}

void wuy_shmpool_init(void)
{
	wuy_shmpool_hash_buckets = wuy_shmpool_alloc_permanent(
			sizeof(struct wuy_shmpool *) * WUY_SHMPOOL_NHASH);

	wuy_shmpools = wuy_shmpool_alloc_permanent(
			sizeof(struct wuy_shmpool) * WUY_SHMPOOL_MAX);

	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_setpshared(&attr, 1);
	pthread_mutex_init(&wuy_shmpool_lock, &attr);
	pthread_mutexattr_destroy(&attr);
}

static struct wuy_shmpool *wuy_shmpool_hash_get(uint64_t key)
{
	struct wuy_shmpool *bucket = wuy_shmpool_hash_buckets[key % WUY_SHMPOOL_NHASH];
	for (struct wuy_shmpool *pool = bucket; pool != NULL; pool = pool->hash_next) {
		if (pool->key == key) {
			return pool;
		}
	}
	return NULL;
}

static struct wuy_shmpool *wuy_shmpool_hash_pop(uint64_t key)
{
	struct wuy_shmpool *bucket = wuy_shmpool_hash_buckets[key % WUY_SHMPOOL_NHASH];
	for (struct wuy_shmpool **p_prev = &bucket; *p_prev != NULL; p_prev = &((*p_prev)->hash_next)) {
		struct wuy_shmpool *pool = *p_prev;
		if (pool->key == key) {
			*p_prev = pool->hash_next;
			return pool;
		}
	}
	return NULL;
}

void wuy_shmpool_new(uint64_t key)
{
	pthread_mutex_lock(&wuy_shmpool_lock);

	struct wuy_shmpool *pool = wuy_shmpool_hash_get(key);
	if (pool == NULL) {
		for (int i = 0; i < WUY_SHMPOOL_MAX; i++) {
			if (wuy_shmpools[i].refs == 0) {
				pool = &wuy_shmpools[i];
				break;
			}
		}
	}

	pool->refs++;
	wuy_shmpool_current = pool;
	wuy_shmpool_current_pos = pool->buffer;

	pthread_mutex_unlock(&wuy_shmpool_lock);
}

void wuy_shmpool_release(uint64_t key)
{
	pthread_mutex_lock(&wuy_shmpool_lock);

	struct wuy_shmpool *pool = wuy_shmpool_hash_get(key);
	if (--pool->refs == 0) {
		pool->key = 0;
		wuy_shmpool_hash_pop(key);
	}

	pthread_mutex_unlock(&wuy_shmpool_lock);
}

static void *wuy_shmpool_alloc_small(size_t size)
{
	if (wuy_shmpool_current->buffer + WUY_SHMPOOL_SIZE - wuy_shmpool_current_pos > size) {
		return NULL;
	}

	void *ret = wuy_shmpool_current_pos;
	wuy_shmpool_current_pos += size;
	return ret;
}

void *wuy_shmpool_alloc(size_t size)
{
	size = (size + 7) / 8 * 8;

	if (wuy_shmpool_current == NULL) {
		return wuy_shmpool_alloc_permanent(size);
	}

	return wuy_shmpool_alloc_small(size);
	/*
	if (size > WUY_SHMPOOL_PAGE_SIZE) {
		return wuy_shmpool_alloc_big(size);
	} else {
		return wuy_shmpool_alloc_small(size);
	}
	*/
}

bool wuy_shmpool_check(void)
{
	wuy_shmpool_current_pos = NULL;
	return true;
}
