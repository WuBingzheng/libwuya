#ifndef WUY_CFLUA_H
#define WUY_CFLUA_H

#include <stdbool.h>
#include <lua5.1/lua.h>

#include "wuy_pool.h"

/* `type` in `struct wuy_cflua_command` */
enum wuy_cflua_type {
	WUY_CFLUA_TYPE_END = 0,
	WUY_CFLUA_TYPE_BOOLEAN = LUA_TBOOLEAN,
	WUY_CFLUA_TYPE_INTEGER = 100, /* new type */
	WUY_CFLUA_TYPE_ENUMSTR = 101, /* new type */
	WUY_CFLUA_TYPE_FLOAT = LUA_TNUMBER,
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

	const char		*description;

	/* is single array member when name==NULL? */
	bool			is_single_array;

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

	/* only for multi-member array command. */
	off_t			array_number_offset;
	size_t			array_member_size;

	/* offset of default-values-container to inherit for multi-array-members */
	off_t			inherit_container_offset;

	/* offset of inherit count(int) */
	off_t			inherit_count_offset;

	/* default values, only for key-value options, but not array members */
	union wuy_cflua_default_value {
		bool			b;
		int			n;
		float			d;
		wuy_cflua_function_t	f;
		const char		*s;
		const void		*p;
		const void		*t;
	} default_value;

	/* limits, only for WUY_CFLUA_TYPE_INTEGER, WUY_CFLUA_TYPE_FLOAT, and WUY_CFLUA_TYPE_ENUMSTR */
	union {
		struct wuy_cflua_command_limits_int {
			bool	is_lower, is_upper;
			int	lower, upper;
		} n;
		struct wuy_cflua_command_limits_float {
			bool	is_lower, is_upper;
			float	lower, upper;
		} d;
		const char **e; /* available ENUMSTR candidates */
	} limits;

	/* corresponding values to limits.e */
	int				*enum_values;

	/* used internal */
	struct wuy_cflua_command	*real;
};

struct wuy_cflua_table {
	struct wuy_cflua_command	*commands;

	const char		*refer_name;

	/* size of new container.
	 * If set 0, do not allocate new container and use the current container. */
	unsigned		size;

	/* if this table-command is not set in configuration:
	 *   may_omit=true: omit it;
	 *   may_omit=false: allocate table and set members all by default values. */
	bool			may_omit;

	/* print the table's name, for wuy_cflua_strerror() to print table-stack.
	 * If this is not set and command->name is set (not array member command),
	 * command->name is used. Otherwise "{?}" is used. */
	int			(*name)(void *, char *, int);

	/* handler called before parsing */
	void			(*init)(void *);

	/* handler called after parsing, return same with wuy_cflua_parse() */
	const char *		(*post)(void *);

	/* handler called for extra free action */
	void			(*free)(void *);

	/* handler for arbitrary key-value options, return same with wuy_cflua_parse() */
	const char *		(*arbitrary)(lua_State *, void *);
};

struct wuy_cflua_options {
	wuy_pool_t	*pool;
	const void	*inherit_from;
	void		(*function_hook)(lua_State *, wuy_cflua_function_t);
};

/* return WUY_CFLUA_OK if successful, or error reason if fail */
#define WUY_CFLUA_OK (const char *)0

const char *wuy_cflua_parse_opts(lua_State *L, struct wuy_cflua_table *table,
		void *container, struct wuy_cflua_options *);

/* Use this as named-argument, `wuy_cflua_parse_opts(L, t, c, .pool=pool)` */
#define wuy_cflua_parse(L, table, container, ...) \
	wuy_cflua_parse_opts(L, table, container, &(struct wuy_cflua_options){__VA_ARGS__})

/* could be used by handlers in struct wuy_cflua_table */
extern wuy_pool_t *wuy_cflua_pool;

/* set by user if necessary when table's post/arbitrary hander fails */
extern const char *wuy_cflua_post_arg;
extern const char *wuy_cflua_arbitrary_arg;

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

void wuy_cflua_dump_table_markdown(const struct wuy_cflua_table *table, int indent);

#endif
