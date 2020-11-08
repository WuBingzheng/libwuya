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

struct wuy_shmpool_shared {
	char		name[100];
	size_t		big_size;
	int		big_max;
	int		big_index;
	size_t		small_size;
	size_t		small_pos;
	char		small_buffer[0];
};

struct wuy_shmpool_map_info {
	void		*address;
	size_t		length;
};
struct wuy_shmpool_local {
	struct wuy_shmpool_shared	*shared_pool;
	int				big_index;
	size_t				small_pos;
	wuy_list_node_t			list_node;
	struct wuy_shmpool_map_info	map_infos[0];
};

static pthread_mutex_t *wuy_shmpool_lock;

static WUY_LIST(wuy_shmpool_local_head);
static struct wuy_shmpool_local *wuy_shmpool_current;

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
		ftruncate(fd, size);
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

struct wuy_shmpool_local *wuy_shmpool_new(const char *name, size_t small_size,
		size_t big_size, int big_max)
{
	_debug("wuy_shmpool: new pool name=%s\n", name);

	/* shared */
	pthread_mutex_lock(wuy_shmpool_lock);

	struct wuy_shmpool_shared *shared_pool = wuy_shmpool_open_mmap(name, sizeof(struct wuy_shmpool_shared) + small_size);

	if (shared_pool->name[0] == '\0') {
		strncpy(shared_pool->name, name, sizeof(shared_pool->name));
		shared_pool->small_size = small_size;
		shared_pool->big_size = big_size;
		shared_pool->big_max = big_max;

	} else if (strcmp(shared_pool->name, name) != 0) {
		printf("wuy_shmpool: fail in match!!!\n");
		pthread_mutex_unlock(wuy_shmpool_lock);
		return NULL;
	}

	pthread_mutex_unlock(wuy_shmpool_lock);

	/* local */
	struct wuy_shmpool_local *local_pool = calloc(1, sizeof(struct wuy_shmpool_local)
			+ sizeof(struct wuy_shmpool_map_info) * big_max);
	local_pool->shared_pool = shared_pool;
	wuy_list_append(&wuy_shmpool_local_head, &local_pool->list_node);

	wuy_shmpool_current = local_pool;
	return local_pool;
}

// XXX need lock?
void wuy_shmpool_release(struct wuy_shmpool_local *local_pool)
{
	struct wuy_shmpool_shared *shared_pool = local_pool->shared_pool;
	const char *name = shared_pool->name;

	_debug("wuy_shmpool: release pool %s\n", name);

	for (int i = 0; i < local_pool->big_index; i++) {
		char name[1000];
		sprintf(name, "%s-big-%d", name, i);
		shm_unlink(name);

		struct wuy_shmpool_map_info *info = &local_pool->map_infos[i];
		munmap(info->address, info->length);
	}
	shm_unlink(name);
	munmap(shared_pool, sizeof(struct wuy_shmpool_shared) + shared_pool->small_size);
}

static void *wuy_shmpool_alloc_small(size_t size)
{
	struct wuy_shmpool_shared *shared_pool = wuy_shmpool_current->shared_pool;

	if (shared_pool->small_size - wuy_shmpool_current->small_pos < size) {
		_debug("wuy_shmpool: fail in alloc small total=%lu pos=%lu size=%lu\n",
				shared_pool->small_size, wuy_shmpool_current->small_pos, size);
		return NULL;
	}

	void *ret = shared_pool->small_buffer + wuy_shmpool_current->small_pos;
	wuy_shmpool_current->small_pos += size;
	return ret;
}

static void *wuy_shmpool_alloc_big(size_t size)
{
	struct wuy_shmpool_shared *shared_pool = wuy_shmpool_current->shared_pool;

	_debug("wuy_shmpool big alloc: %s index=%d size=%lu\n",
			shared_pool->name, wuy_shmpool_current->big_index, size);

	if (wuy_shmpool_current->big_index >= shared_pool->big_max) {
		return NULL;
	}

	int big_index = wuy_shmpool_current->big_index++;

	char name[1000];
	sprintf(name, "%s-big-%d", shared_pool->name, big_index);

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

	if (size > wuy_shmpool_current->shared_pool->big_size) {
		return wuy_shmpool_alloc_big(size);
	} else {
		return wuy_shmpool_alloc_small(size);
	}
}

bool wuy_shmpool_finish(struct wuy_shmpool_local *local_pool)
{
	struct wuy_shmpool_shared *shared_pool = local_pool->shared_pool;

	pthread_mutex_lock(wuy_shmpool_lock);

	bool check = true;

	if (shared_pool->small_pos == 0) {
		shared_pool->small_pos = local_pool->small_pos;
	} else if (shared_pool->small_pos != local_pool->small_pos) {
		check = false;
	}

	if (shared_pool->big_index == 0) {
		shared_pool->big_index = local_pool->big_index;
	} else if (shared_pool->big_index != local_pool->big_index) {
		check = false;
	}

	pthread_mutex_unlock(wuy_shmpool_lock);

	wuy_shmpool_current = NULL;
	return check;
}

static void wuy_shmpool_local_exit_handler(void)
{
	struct wuy_shmpool_local *local_pool;
	while (wuy_list_pop_type(&wuy_shmpool_local_head, local_pool, list_node)) {
		wuy_shmpool_release(local_pool);
	}
}
void wuy_shmpool_init(void)
{
	wuy_shmpool_lock = wuy_shmpool_alloc_permanent(sizeof(pthread_mutex_t));

	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_setpshared(&attr, 1);
	pthread_mutex_init(wuy_shmpool_lock, &attr);
	pthread_mutexattr_destroy(&attr);

	atexit(wuy_shmpool_local_exit_handler);
}
