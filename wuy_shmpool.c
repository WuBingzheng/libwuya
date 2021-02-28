#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "wuy_list.h"

#include "wuy_shmpool.h"

#define WUY_SHMPOOL_PAGE_SIZE	4096
#define WUY_SHMPOOL_NHASH	1024

#define _debug printf
//#define _debug(...)

struct wuy_shmpool_map_info {
	void		*address;
	size_t		length;
};
struct wuy_shmpool {
	char		name[100];
	size_t		big_size;
	int		big_max;
	size_t		small_size;
	char		*small_buffer;

	int		big_index;
	size_t		small_pos;

	wuy_list_node_t	list_node;

	struct wuy_shmpool_map_info	map_infos[0];
};

static WUY_LIST(wuy_shmpool_head);

static struct wuy_shmpool *wuy_shmpool_current;

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

static void *wuy_shmpool_open_mmap(const char *name, size_t size)
{
	_debug("wuy_shmpool: open mmap %s %lu\n", name, size);

	int fd = shm_open(name, O_RDWR | O_CREAT, 0600);
	if (fd < 0) {
		return NULL;
	}

	struct stat buf;
	fstat(fd, &buf);
	if (buf.st_size == 0) {
		if (ftruncate(fd, size) < 0) {
			printf("wuy_shmpool: fail in ftruncate: %ld\n", size);
			return NULL;
		}
	} else if (buf.st_size != size) {
		printf("wuy_shmpool: fail in size: %ld %ld\n", size, buf.st_size);
		return NULL;
	}

	void *ret = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (ret == NULL) {
		_debug("wuy_shmpool: open mmap fail %s %lu\n", name, size);
	}

	close(fd);
	return ret;
}

struct wuy_shmpool *wuy_shmpool_new(const char *name, size_t small_size,
		size_t big_size, int big_max)
{
	_debug("wuy_shmpool: new pool name=%s\n", name);

	struct wuy_shmpool *pool = calloc(1, sizeof(struct wuy_shmpool)
			+ sizeof(struct wuy_shmpool_map_info) * big_max);
	strncpy(pool->name, name, sizeof(pool->name) - 1);
	pool->big_size = big_size;
	pool->big_max = big_max;
	pool->small_size = small_size;
	pool->small_buffer = wuy_shmpool_open_mmap(name, small_size);

	wuy_list_append(&wuy_shmpool_head, &pool->list_node);

	wuy_shmpool_current = pool;
	return pool;
}

void wuy_shmpool_destroy(struct wuy_shmpool *pool)
{
	_debug("wuy_shmpool: destroy pool %s\n", pool->name);

	shm_unlink(pool->name);
	munmap(pool->small_buffer, pool->small_size);

	for (int i = 0; i < pool->big_index; i++) {
		char big_name[1000];
		sprintf(big_name, "%s-big-%d", pool->name, i);
		shm_unlink(big_name);

		struct wuy_shmpool_map_info *info = &pool->map_infos[i];
		munmap(info->address, info->length);
	}

	free(pool);
}

static void *wuy_shmpool_alloc_small(size_t size)
{
	if (wuy_shmpool_current->small_size - wuy_shmpool_current->small_pos < size) {
		_debug("wuy_shmpool: fail in alloc small total=%lu pos=%lu size=%lu\n",
				wuy_shmpool_current->small_size, wuy_shmpool_current->small_pos, size);
		return NULL;
	}

	void *ret = wuy_shmpool_current->small_buffer + wuy_shmpool_current->small_pos;
	wuy_shmpool_current->small_pos += size;
	return ret;
}

static void *wuy_shmpool_alloc_big(size_t size)
{
	_debug("wuy_shmpool big alloc: %s index=%d size=%lu\n",
			wuy_shmpool_current->name, wuy_shmpool_current->big_index, size);

	if (wuy_shmpool_current->big_index >= wuy_shmpool_current->big_max) {
		return NULL;
	}

	int big_index = wuy_shmpool_current->big_index++;

	char name[200];
	sprintf(name, "%s-big-%d", wuy_shmpool_current->name, big_index);

	void *ret = wuy_shmpool_open_mmap(name, size);

	struct wuy_shmpool_map_info *info = &wuy_shmpool_current->map_infos[big_index];
	info->address = ret;
	info->length = size;

	return ret;
}

void *wuy_shmpool_alloc(size_t size)
{
	size = (size + 7) / 8 * 8;

	if (wuy_shmpool_current == NULL) {
		return wuy_shmpool_alloc_permanent(size);
	}

	if (size > wuy_shmpool_current->big_size) {
		return wuy_shmpool_alloc_big(size);
	} else {
		return wuy_shmpool_alloc_small(size);
	}
}

void wuy_shmpool_finish(struct wuy_shmpool *pool)
{
	wuy_shmpool_current = NULL;
}

void wuy_shmpool_cleanup(void)
{
	struct wuy_shmpool *pool;
	while (wuy_list_pop_type(&wuy_shmpool_head, pool, list_node)) {
		wuy_shmpool_destroy(pool);
	}
}
