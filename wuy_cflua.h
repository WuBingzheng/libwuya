#ifndef WUY_CFLUA_H
#define WUY_CFLUA_H

#include <stdbool.h>
#include <lua5.1/lua.h>

#define WUY_CFLUA_ERR_NO_MEM		-1
#define WUY_CFLUA_ERR_DEPLICATE_MEMBER	-2
#define WUY_CFLUA_ERR_POST		-3
#define WUY_CFLUA_ERR_INVALID_MEMBER	-4
#define WUY_CFLUA_ERR_INVALID_TABLE	-5
#define WUY_CFLUA_ERR_TOO_DEEP_META	-6

/* `type` in `struct wuy_cflua_command` */
enum wuy_cflua_type {
	WUY_CFLUA_TYPE_END = 0,
	WUY_CFLUA_TYPE_BOOLEAN,
	WUY_CFLUA_TYPE_INTEGER,
	WUY_CFLUA_TYPE_DOUBLE,
	WUY_CFLUA_TYPE_STRING,
	WUY_CFLUA_TYPE_FUNCTION,
	WUY_CFLUA_TYPE_TABLE,
};

/* for WUY_CFLUA_TYPE_FUNCTION */
typedef int wuy_cflua_function_t;

/* `flags` in `struct wuy_cflua_command` */
enum wuy_cflua_flag {
	/* only for WUY_CFLUA_TYPE_TABLE, reuse table with the same address */
	WUY_CFLUA_FLAG_TABLE_REUSE = 0x1,

	/* only for WUY_CFLUA_TYPE_TABLE, allow arbitrary key */
	WUY_CFLUA_FLAG_ARBITRARY_KEY = 0x2,

	/* only for array member commands, accept 1 member only */
	WUY_CFLUA_FLAG_UNIQ_MEMBER = 0x4,
};

struct wuy_cflua_table;
struct wuy_cflua_command {
	/* name of command.
	 * NULL for array member command, and non-NULL for key-value options.
	 * Set it at the 1st place if array member. */
	const char		*name;

	enum wuy_cflua_type	type;

	enum wuy_cflua_flag	flags;

	/* offset of target member in container. */
	int			offset;

	/* offset of number of array members. only for array member command. */
	int			array_num_offset;

	union {
		/* only for WUY_CFLUA_TYPE_TABLE */
		struct wuy_cflua_table		*table;

		/* only for WUY_CFLUA_TYPE_END */
		struct wuy_cflua_command	*(*next)(struct wuy_cflua_command *);
	} u;
};

struct wuy_cflua_table {
	struct wuy_cflua_command	*commands;

	/* size of new container.
	 * If set 0, do not allocate new container and use the current container. */
	unsigned		size;

	/* container handlers. */
	void			(*init)(void *);
	bool			(*post)(void *);
	void			(*cleanup)(void *);
};

int wuy_cflua_parse(lua_State *L, struct wuy_cflua_command *cmd, void *container);

#define WUY_CFLUA_ARRAY_STRING_TABLE &(struct wuy_cflua_table) { \
	.commands = (struct wuy_cflua_command[2]) { { .type = WUY_CFLUA_TYPE_STRING } } \
}

#endif
