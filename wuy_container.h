#ifndef WUY_CONTAINER_H
#define WUY_CONTAINER_H

#include <stddef.h>

/**
 * @brief Return the container of member.
 */
#define wuy_containerof(pos, type, member) \
	((type *)((char *)(pos) - offsetof(type, member)))

/**
 * @brief Return the container of member if pos != NULL; or NULL.
 */
#define wuy_containerof_check(pos, type, member) \
	({ typeof(pos) _wcc_pos_ = (pos); \
	   _wcc_pos_ ? wuy_containerof(_wcc_pos_, type, member) : NULL; \
	})

#endif
