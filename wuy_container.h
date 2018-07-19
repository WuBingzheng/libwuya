#ifndef WUY_CONTAINER_H
#define WUY_CONTAINER_H

#include <stddef.h>

/**
 * @brief Return the container of member.
 */
#define wuy_containerof(pos, type, member) \
	((type *)((char *)(pos) - offsetof(type, member)))

#endif
