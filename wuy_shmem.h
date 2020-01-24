#ifndef WUY_SHMEM_H
#define WUY_SHMEM_H

#include <sys/mman.h>

static inline void *wuy_shmem_alloc(size_t size)
{
	static void *last_addr = NULL;
	static size_t last_size = 0;

	if (size > last_size) {
		last_size = size > 4096 ? size : 4096;
		last_addr = mmap(NULL, last_size, PROT_READ|PROT_WRITE,
				MAP_ANONYMOUS|MAP_SHARED, -1, 0);
	}

	void *ret = last_addr;
	last_addr += size;
	last_size -= size;
	return ret;
}


#endif
