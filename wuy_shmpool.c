#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/mman.h>

#include "wuy_hlist.h"
#include "wuy_shmpool.h"

#define WUY_SHMPOOL_PAGE_SIZE	4096
#define WUY_SHMPOOL_NHASH	1024
#define WUY_SHMPOOL_MAX		10000
#define WUY_SHMPOOL_SIZE	64*1024

//#define _debug printf
#define _debug(...)

struct wuy_shmpool {
	uint64_t		key;
	int			refs;
	wuy_hlist_node_t	hash_node;
	char			buffer[WUY_SHMPOOL_SIZE];
};

/* on shared-memory */
static pthread_mutex_t *wuy_shmpool_lock;
static struct wuy_shmpool *wuy_shmpools;
static wuy_hlist_head_t *wuy_shmpool_hash_buckets;
static struct wuy_shmpool *wuy_shmpool_current = NULL;

/* on process local memory */
static char *wuy_shmpool_current_pos = NULL;

static void *wuy_shmpool_alloc_permanent(size_t size)
{
	static char *last_addr = NULL;
	static size_t last_size = 0;

	_debug("wuy_shmpool: alloc permanent %lu\n", size);

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
			sizeof(wuy_hlist_head_t) * WUY_SHMPOOL_NHASH);

	wuy_shmpools = wuy_shmpool_alloc_permanent(
			sizeof(struct wuy_shmpool) * WUY_SHMPOOL_MAX);

	wuy_shmpool_lock = wuy_shmpool_alloc_permanent(sizeof(pthread_mutex_t));

	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_setpshared(&attr, 1);
	pthread_mutex_init(wuy_shmpool_lock, &attr);
	pthread_mutexattr_destroy(&attr);
}

static struct wuy_shmpool *wuy_shmpool_hash_get(uint64_t key)
{
	wuy_hlist_head_t *bucket = &wuy_shmpool_hash_buckets[key % WUY_SHMPOOL_NHASH];

	struct wuy_shmpool *pool;
	wuy_hlist_iter_type(bucket, pool, hash_node) {
		if (pool->key == key) {
			return pool;
		}
	}
	return NULL;
}

static void wuy_shmpool_hash_add(struct wuy_shmpool *pool)
{
	wuy_hlist_head_t *bucket = &wuy_shmpool_hash_buckets[pool->key % WUY_SHMPOOL_NHASH];
	wuy_hlist_insert(bucket, &pool->hash_node);
}

struct wuy_shmpool *wuy_shmpool_new(uint64_t key)
{
	pthread_mutex_lock(wuy_shmpool_lock);

	struct wuy_shmpool *pool = wuy_shmpool_hash_get(key);
	if (pool == NULL) {
		for (int i = 0; i < WUY_SHMPOOL_MAX; i++) {
			if (wuy_shmpools[i].refs == 0) {
				_debug("wuy_shmpool: create new pool\n");
				pool = &wuy_shmpools[i];
				break;
			}
		}
		if (pool == NULL) {
			_debug("wuy_shmpool: fail to create new pool key=%lx\n", key);
			pthread_mutex_unlock(wuy_shmpool_lock);
			return false;
		}
		pool->key = key;
		wuy_shmpool_hash_add(pool);
	}

	_debug("wuy_shmpool: new pool key=%lx, i=%ld, refs=%d\n",
			key, pool - wuy_shmpools, pool->refs);

	pool->refs++;
	wuy_shmpool_current = pool;
	wuy_shmpool_current_pos = pool->buffer;

	pthread_mutex_unlock(wuy_shmpool_lock);
	return pool;
}

void wuy_shmpool_release(struct wuy_shmpool *pool)
{
	pthread_mutex_lock(wuy_shmpool_lock);

	_debug("wuy_shmpool: release pool %lx\n", pool->key);

	if (--pool->refs == 0) {
		pool->key = 0;
		wuy_hlist_delete(&pool->hash_node);
	}

	pthread_mutex_unlock(wuy_shmpool_lock);
}

static void *wuy_shmpool_alloc_small(size_t size)
{
	if (wuy_shmpool_current->buffer + WUY_SHMPOOL_SIZE - wuy_shmpool_current_pos < size) {
		_debug("wuy_shmpool: fail in alloc small total=%ld size=%ld\n",
				wuy_shmpool_current->buffer + WUY_SHMPOOL_SIZE
				- wuy_shmpool_current_pos, size);
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

void wuy_shmpool_finish(struct wuy_shmpool *pool)
{
	wuy_shmpool_current_pos = NULL;
}
