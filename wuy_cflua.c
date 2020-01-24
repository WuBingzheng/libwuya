#include <string.h>
#include <stdlib.h>
#include <lua5.1/lua.h>
#include <lua5.1/lauxlib.h>

#include "wuy_dict.h"
#include "wuy_array.h"

#include "wuy_cflua.h"

/* to reuse table or function */
struct wuy_cflua_reuse_node {
	wuy_dict_node_t		hash_node;
	const void		*pointer;
	union {
		void			*container;
		wuy_cflua_function_t	func;
	} u;
};

static wuy_dict_t *wuy_cflua_reuse_dict;


#if 0

/* command stack */
typedef struct {
	struct wuy_cflua_command		*cmd;
	void			*container;
} wuy_cflua_command_stack_t;

#define WUY_CFLUA_COMMAND_STACK_MAXSIZE 100
static int wuy_cflua_command_stack_depth;
static wuy_cflua_command_stack_t wuy_cflua_command_stack[WUY_CFLUA_COMMAND_STACK_MAXSIZE];


/* remember all table to destroy */
static wuy_array_t wuy_cflua_tables;
#endif


#define wuy_cflua_assign_value(cmd, container, value, type) \
	do { \
		void *target = ((char *)container) + cmd->offset; \
		if (cmd->name != NULL) { \
			*(type *)target = (type)value; \
		} else if ((cmd->flags & WUY_CFLUA_FLAG_UNIQ_MEMBER) != 0) { \
			if (*(type *)target) { \
				return WUY_CFLUA_ERR_DEPLICATE_MEMBER; \
			} \
			*(type *)target = (type)value; \
		} else { \
			if (!wuy_array_yet_init(target)) \
				wuy_array_init(target, sizeof(type)); \
			type *p = wuy_array_push(target); \
			*p = value; \
		} \
	} while (0)

static int wuy_cflua_set_function(lua_State *L, struct wuy_cflua_command *cmd, void *container)
{
	const void *pointer = lua_topointer(L, -1);
	struct wuy_cflua_reuse_node *node = wuy_dict_get(wuy_cflua_reuse_dict, pointer);
	if (node == NULL) {
		node = malloc(sizeof(struct wuy_cflua_reuse_node));
		node->pointer = pointer;
		lua_pushvalue(L, -1); /* luaL_ref() will pop it */
		node->u.func = luaL_ref(L, LUA_REGISTRYINDEX);
		wuy_dict_add(wuy_cflua_reuse_dict, node);
	}

	wuy_cflua_assign_value(cmd, container, node->u.func, wuy_cflua_function_t);
	return 0;
}
static int wuy_cflua_set_string(lua_State *L, struct wuy_cflua_command *cmd, void *container)
{
	char *value = strdup(lua_tostring(L, -1));
	wuy_cflua_assign_value(cmd, container, value, char *);
	return 0;
}
static int wuy_cflua_set_boolean(lua_State *L, struct wuy_cflua_command *cmd, void *container)
{
	bool value = lua_toboolean(L, -1);
	wuy_cflua_assign_value(cmd, container, value, bool);
	return 0;
}
static int wuy_cflua_set_integer(lua_State *L, struct wuy_cflua_command *cmd, void *container)
{
	lua_Number value = lua_tonumber(L, -1);
	wuy_cflua_assign_value(cmd, container, value, int);
	return 0;
}
static int wuy_cflua_set_float(lua_State *L, struct wuy_cflua_command *cmd, void *container)
{
	lua_Number value = lua_tonumber(L, -1);
	wuy_cflua_assign_value(cmd, container, value, double);
	return 0;
}

static int wuy_cflua_set_types(lua_State *L, struct wuy_cflua_command *cmd, void *container);

static int wuy_cflua_set_array_members(lua_State *L, struct wuy_cflua_command *cmd, void *container)
{
	int i = 1;
	while (1) {
		lua_rawgeti(L, -1, i++);
		if (lua_isnil(L, -1)) {
			lua_pop(L, 1);
			break;
		}
		int ret = wuy_cflua_set_types(L, cmd, container);
		if (ret != 0) {
			return ret;
		}
		lua_pop(L, 1);
	}

	/* return if raw member exist; otherwise try metatable */
	if (i > 2) {
		return 0;
	}

	i = 1;
	while (1) {
		lua_pushinteger(L, i++);
		lua_gettable(L, -2);
		if (lua_isnil(L, -1)) {
			lua_pop(L, 1);
			break;
		}
		int ret = wuy_cflua_set_types(L, cmd, container);
		if (ret != 0) {
			return ret;
		}
		lua_pop(L, 1);
	}
	return 0;
}

static int wuy_cflua_set_option(lua_State *L, struct wuy_cflua_command *cmd, void *container)
{
	lua_getfield(L, -1, cmd->name);
	if (lua_isnil(L, -1)) {
		printf("invalid table: %s\n", cmd->name);
		return WUY_CFLUA_ERR_INVALID_TABLE;
	}
	int ret = wuy_cflua_set_types(L, cmd, container);
	if (ret != 0) {
		return ret;
	}
	lua_pop(L, 1);
	return 0;
}

static int wuy_cflua_set_table(lua_State *L, struct wuy_cflua_command *cmd, void *container)
{
	struct wuy_cflua_table *table = cmd->u.table;

	/* create new container */
	if (table->size != 0) {
		/* try to reuse */
		const void *pointer = lua_topointer(L, -1);
		if ((cmd->flags & WUY_CFLUA_FLAG_TABLE_REUSE) != 0) {
			struct wuy_cflua_reuse_node *node = wuy_dict_get(wuy_cflua_reuse_dict, pointer);
			if (node != NULL) {
				wuy_cflua_assign_value(cmd, container, node->u.container, void *);
				return 0;
			}
		}

		void *out = container;

		container = calloc(1, table->size);
		if (container == NULL) {
			return WUY_CFLUA_ERR_NO_MEM;
		}

		wuy_cflua_assign_value(cmd, out, container, void *);

		if ((cmd->flags & WUY_CFLUA_FLAG_TABLE_REUSE) != 0) {
			struct wuy_cflua_reuse_node *node = malloc(sizeof(struct wuy_cflua_reuse_node));
			node->pointer = pointer;
			node->u.container = container;
			wuy_dict_add(wuy_cflua_reuse_dict, node);
		}
	}

	if (table->init != NULL) {
		table->init(container);
	}

	for (cmd = table->commands; cmd->type != WUY_CFLUA_TYPE_END; cmd++) {
		int ret;
		if (cmd->name == NULL) {
			ret = wuy_cflua_set_array_members(L, cmd, container);
		} else {
			ret = wuy_cflua_set_option(L, cmd, container);
		}
		if (ret != 0) {
			return ret;
		}
	}
	if (cmd->u.next != NULL) {
		struct wuy_cflua_command *(*next)(struct wuy_cflua_command *) = cmd->u.next;
		while ((cmd = next(cmd)) != NULL) {
			int ret = wuy_cflua_set_option(L, cmd, container);
			if (ret != 0) {
				return ret;
			}
		}
	}

	if (table->post != NULL) {
		if (!table->post(container)) {
			return WUY_CFLUA_ERR_POST;
		}
	}

	return 0;
}

static int wuy_cflua_set_types(lua_State *L, struct wuy_cflua_command *cmd, void *container)
{
	switch (cmd->type) {
	case WUY_CFLUA_TYPE_BOOLEAN:
		return wuy_cflua_set_boolean(L, cmd, container);
	case WUY_CFLUA_TYPE_INTEGER:
		return wuy_cflua_set_integer(L, cmd, container);
	case WUY_CFLUA_TYPE_FLOAT:
		return wuy_cflua_set_float(L, cmd, container);
	case WUY_CFLUA_TYPE_STRING:
		return wuy_cflua_set_string(L, cmd, container);
	case WUY_CFLUA_TYPE_FUNCTION:
		return wuy_cflua_set_function(L, cmd, container);
	case WUY_CFLUA_TYPE_TABLE:
		return wuy_cflua_set_table(L, cmd, container);
	default:
		abort();
	}
}

int wuy_cflua_parse(lua_State *L, struct wuy_cflua_command *cmd, void *container)
{
	wuy_cflua_reuse_dict = wuy_dict_new_type(WUY_DICT_KEY_POINTER,
			offsetof(struct wuy_cflua_reuse_node, pointer),
			offsetof(struct wuy_cflua_reuse_node, hash_node));

	int ret = wuy_cflua_set_table(L, cmd, container);

	wuy_dict_destroy(wuy_cflua_reuse_dict);

	return ret;
}
