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
#define WUY_CFLUA_ERR_LIMIT		-6
#define WUY_CFLUA_ERR_NO_ARRAY		-7
#define WUY_CFLUA_ERR_INVALID_TYPE	-8
#define WUY_CFLUA_ERR_INVALID_CMD	-9
#define WUY_CFLUA_ERR_ARBITRARY		-10

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

struct wuy_cflua_table;
struct wuy_cflua_command {
	/* name of command.
	 * NULL for array member command, and non-NULL for key-value options. */
	const char		*name;

	/* is single array member? */
	bool			is_single_array;

	bool			is_extra_commands;

	enum wuy_cflua_type	type;

	/* offset of target member in container. */
	off_t			offset;

	/* extra settings for types */
	union {
		/* only for WUY_CFLUA_TYPE_STRING */
		off_t				length_offset;

		/* only for WUY_CFLUA_TYPE_TABLE */
		struct wuy_cflua_table		*table;

		/* only for WUY_CFLUA_TYPE_END */
		struct wuy_cflua_command	*(*next)(struct wuy_cflua_command *);
	} u;

	/* -- OPTIONAL -- */

	/* offset of meta_level(int) */
	off_t			meta_level_offset;

	/* only for multi-member array command. */
	off_t			array_number_offset;
	size_t			array_member_size;

	/* default values, only for key-value options, but not array members */
	union {
		bool			b;
		int			n;
		double			d;
		wuy_cflua_function_t	f;
		const char		*s;
	} default_value;

	/* limits, only for WUY_CFLUA_TYPE_INTEGER and WUY_CFLUA_TYPE_DOUBLE */
	union {
		struct wuy_cflua_command_limits_int {
			bool	is_lower, is_upper;
			int	lower, upper;
		} n;
		struct wuy_cflua_command_limits_double {
			bool	is_lower, is_upper;
			double	lower, upper;
		} d;
	} limits;

	/* used internal */
	struct wuy_cflua_command	*real;
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

	/* handler for arbitrary key-value options */
	bool			(*arbitrary)(lua_State *);
};

/* parse */
int wuy_cflua_parse(lua_State *L, struct wuy_cflua_table *table, void *container);

/* param err  is returned by wuy_cflua_parse(). */
const char *wuy_cflua_strerror(lua_State *L, int err);

struct wuy_cflua_table *wuy_cflua_copy_table_default(const struct wuy_cflua_table *src,
		const void *default_container);
void wuy_cflua_free_copied_table(struct wuy_cflua_table *table);

static inline bool wuy_cflua_is_function_set(wuy_cflua_function_t f)
{
	return f != 0;
}

#define WUY_CFLUA_ARRAY_STRING_TABLE &(struct wuy_cflua_table) { \
	.commands = (struct wuy_cflua_command[2]) { { .type = WUY_CFLUA_TYPE_STRING } } \
}

#define WUY_CFLUA_ARRAY_INTEGER_TABLE &(struct wuy_cflua_table) { \
	.commands = (struct wuy_cflua_command[2]) { { .type = WUY_CFLUA_TYPE_INTEGER } } \
}

#define WUY_CFLUA_LIMITS(l, u)		{ .is_lower = true, .is_upper = true, .lower = l, .upper = u }
#define WUY_CFLUA_LIMITS_LOWER(n)	{ .is_lower = true, .lower = n }
#define WUY_CFLUA_LIMITS_UPPER(n)	{ .is_upper = true, .upper = n }
#define WUY_CFLUA_LIMITS_POSITIVE	WUY_CFLUA_LIMITS_LOWER(1)
#define WUY_CFLUA_LIMITS_NON_NEGATIVE	WUY_CFLUA_LIMITS_LOWER(0)

void wuy_cflua_build_tables(lua_State *L, struct wuy_cflua_table *table);

#endif
