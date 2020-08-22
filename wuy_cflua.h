#ifndef WUY_CFLUA_H
#define WUY_CFLUA_H

#include <stdbool.h>
#include <lua5.1/lua.h>

/* return error code */
#define WUY_CFLUA_ERR_NO_MEM		-1
#define WUY_CFLUA_ERR_DEPLICATE_MEMBER	-2
#define WUY_CFLUA_ERR_POST		-3
#define WUY_CFLUA_ERR_TOO_DEEP_META	-4
#define WUY_CFLUA_ERR_WRONG_TYPE	-5

/* `type` in `struct wuy_cflua_command` */
enum wuy_cflua_type {
	WUY_CFLUA_TYPE_END = 0,
	WUY_CFLUA_TYPE_BOOLEAN = LUA_TBOOLEAN,
	WUY_CFLUA_TYPE_INTEGER = 100, /* new type */
	WUY_CFLUA_TYPE_DOUBLE = LUA_TNUMBER,
	WUY_CFLUA_TYPE_STRING = LUA_TSTRING,
	WUY_CFLUA_TYPE_FUNCTION = LUA_TFUNCTION,
	WUY_CFLUA_TYPE_TABLE = LUA_TTABLE,
};

/* for WUY_CFLUA_TYPE_FUNCTION */
typedef int wuy_cflua_function_t;

/* `flags` in `struct wuy_cflua_command` */
enum wuy_cflua_flag {
	/* only for WUY_CFLUA_TYPE_TABLE, reuse table with the same address */
	WUY_CFLUA_FLAG_TABLE_REUSE = 0x1,

	/* only for WUY_CFLUA_TYPE_TABLE, allow arbitrary key */
	WUY_CFLUA_FLAG_ARBITRARY_KEY = 0x2,

	/* only for array member commands.
	 * If set, at most 1 member is allowed, and assigned to container directly.
	 * If not set, more members are allowed, and an array is allocated to
	 * save the members, and the array is assigned to container. The array size
	 * is (#member+1) where the last one is zero-value. Set @array_number_offset
	 * to get #member. */
	WUY_CFLUA_FLAG_UNIQ_MEMBER = 0x4,
};

struct wuy_cflua_table;
struct wuy_cflua_command {
	/* name of command.
	 * NULL for array member command, and non-NULL for key-value options. */
	const char		*name;

	enum wuy_cflua_type	type;

	enum wuy_cflua_flag	flags;

	/* offset of target member in container. */
	int			offset;

	/* only for multi-member array command. */
	int			array_number_offset;
	int			array_member_size;

	/* used internal */
	struct wuy_cflua_command	*real;

	union {
		/* only for WUY_CFLUA_TYPE_STRING */
		int				length_offset;

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

	/* print the table's name, for wuy_cflua_strerror() to print table-stack.
	 * If this is not set and command->name is set (not array member command),
	 * command->name is used. Otherwise "{?}" is used. */
	int			(*name)(void *, char *, int);

	/* handler called before parsing */
	void			(*init)(void *);

	/* handler called after parsing */
	bool			(*post)(void *);
};

/* parse */
int wuy_cflua_parse(lua_State *L, struct wuy_cflua_command *cmd, void *container);

/* param err  is returned by wuy_cflua_parse(). */
const char *wuy_cflua_strerror(lua_State *L, int err);

#define WUY_CFLUA_ARRAY_STRING_TABLE &(struct wuy_cflua_table) { \
	.commands = (struct wuy_cflua_command[2]) { { .type = WUY_CFLUA_TYPE_STRING } } \
}

#define WUY_CFLUA_ARRAY_INTEGER_TABLE &(struct wuy_cflua_table) { \
	.commands = (struct wuy_cflua_command[2]) { { .type = WUY_CFLUA_TYPE_INTEGER } } \
}

#endif
