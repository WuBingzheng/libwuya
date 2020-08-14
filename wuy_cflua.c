#include <string.h>
#include <stdlib.h>
#include <lua5.1/lua.h>
#include <lua5.1/lauxlib.h>

#include "wuy_dict.h"

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

/* table-stack for wuy_cflua_strerror() */
static struct wuy_cflua_stack {
	struct wuy_cflua_command	*cmd;
	void				*container;
} wuy_cflua_stacks[100];
static int wuy_cflua_stack_index = 0;

static struct wuy_cflua_command	*wuy_cflua_current_cmd;


/* assign value for different types */
#define wuy_cflua_assign_value(cmd, container, value) \
	*(typeof(value) *)(((char *)container) + (cmd)->offset) = value

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

	wuy_cflua_assign_value(cmd, container, node->u.func);
	return 0;
}
static int wuy_cflua_set_string(lua_State *L, struct wuy_cflua_command *cmd, void *container)
{
	char *value = strdup(lua_tostring(L, -1));
	wuy_cflua_assign_value(cmd, container, value);
	return 0;
}
static int wuy_cflua_set_boolean(lua_State *L, struct wuy_cflua_command *cmd, void *container)
{
	bool value = lua_toboolean(L, -1);
	wuy_cflua_assign_value(cmd, container, value);
	return 0;
}
static int wuy_cflua_set_integer(lua_State *L, struct wuy_cflua_command *cmd, void *container)
{
	/* we assume that lua_Number is double */
	lua_Number nvalue = lua_tonumber(L, -1);
	int ivalue = (int)nvalue;
	if (nvalue != (lua_Number)ivalue) {
		return WUY_CFLUA_ERR_WRONG_TYPE;
	}

	wuy_cflua_assign_value(cmd, container, ivalue);
	return 0;
}
static int wuy_cflua_set_double(lua_State *L, struct wuy_cflua_command *cmd, void *container)
{
	/* we assume that lua_Number is double */
	lua_Number value = lua_tonumber(L, -1);
	wuy_cflua_assign_value(cmd, container, value);
	return 0;
}

static int wuy_cflua_set_types(lua_State *L, struct wuy_cflua_command *cmd, void *container);

static size_t wuy_cflua_type_size(enum wuy_cflua_type type)
{
	switch (type) {
	case WUY_CFLUA_TYPE_BOOLEAN: return sizeof(bool);
	case WUY_CFLUA_TYPE_INTEGER: return sizeof(int);
	case WUY_CFLUA_TYPE_DOUBLE: return sizeof(double);
	case WUY_CFLUA_TYPE_STRING: return sizeof(char *);
	case WUY_CFLUA_TYPE_FUNCTION: return sizeof(wuy_cflua_function_t);
	case WUY_CFLUA_TYPE_TABLE: return sizeof(void *);
	default: abort();
	}
}

static int wuy_cflua_set_array_members(lua_State *L, struct wuy_cflua_command *cmd, void *container)
{
	/* check if there any members.
	 * use lua_gettable() which searchs metatable too */
	lua_pushinteger(L, 1);
	lua_gettable(L, -2);
	bool is_nil = lua_isnil(L, -1);
	lua_pop(L, 1);
	if (is_nil) {
		return 0;
	}

	/* find the real table */
	size_t objlen;
	int meta_level = 0;
	while ((objlen = lua_objlen(L, -1)) == 0) {
		lua_getmetatable(L, -1);

		meta_level++;
		if (meta_level > 10) {
			return WUY_CFLUA_ERR_TOO_DEEP_META;
		}
	}

	/* OK, now let's read the array members */

	/* unique-member, just assign the single value */
	if (cmd->flags & WUY_CFLUA_FLAG_UNIQ_MEMBER) {
		if (objlen != 1) {
			return WUY_CFLUA_ERR_DEPLICATE_MEMBER;
		}
		lua_rawgeti(L, -1, 1);
		int ret = wuy_cflua_set_types(L, cmd, container);
		if (ret != 0) {
			return ret;
		}
		lua_pop(L, 1);
		goto out;
	}

	/* multi-members array.
	 * allocate an array for the values, and assign the array to target */
	size_t type_size = cmd->array_member_size ? cmd->array_member_size : wuy_cflua_type_size(cmd->type);
	void *array = calloc(objlen + 1, type_size);
	wuy_cflua_assign_value(cmd, container, array);

	struct wuy_cflua_command fake = *cmd;
	fake.offset = 0;
	fake.real = cmd;

	int i = 1;
	while (1) {
		lua_rawgeti(L, -1, i++);
		if (lua_isnil(L, -1)) {
			lua_pop(L, 1);
			break;
		}
		int ret = wuy_cflua_set_types(L, &fake, array);
		if (ret != 0) {
			return ret;
		}
		lua_pop(L, 1);
		fake.offset += type_size;
	}

	if (cmd->array_number_offset != 0) {
		fake.type = WUY_CFLUA_TYPE_INTEGER;
		fake.offset = cmd->array_number_offset;
		wuy_cflua_assign_value(&fake, container, objlen);
	}

out:
	if (meta_level > 0) {
		lua_pop(L, meta_level);
	}
	return 0;
}

static int wuy_cflua_set_option(lua_State *L, struct wuy_cflua_command *cmd, void *container)
{
	lua_getfield(L, -1, cmd->name);
	if (lua_isnil(L, -1)) {
		lua_pop(L, 1);
		return 0;
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
	wuy_cflua_stacks[wuy_cflua_stack_index].cmd = cmd->real ? cmd->real : cmd;
	wuy_cflua_stacks[wuy_cflua_stack_index].container = (char *)container + cmd->offset;
	wuy_cflua_stack_index++;

	struct wuy_cflua_table *table = cmd->u.table;

	/* create new container */
	if (table->size != 0) {
		/* try to reuse */
		const void *pointer = lua_topointer(L, -1);
		if ((cmd->flags & WUY_CFLUA_FLAG_TABLE_REUSE) != 0) {
			struct wuy_cflua_reuse_node *node = wuy_dict_get(wuy_cflua_reuse_dict, pointer);
			if (node != NULL) {
				wuy_cflua_assign_value(cmd, container, node->u.container);
				goto out;
			}
		}

		void *out = container;

		container = calloc(1, table->size);
		if (container == NULL) {
			return WUY_CFLUA_ERR_NO_MEM;
		}

		wuy_cflua_assign_value(cmd, out, container);

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

out:
	wuy_cflua_stack_index--;
	return 0;
}

static bool wuy_cflua_check_type(lua_State *L, struct wuy_cflua_command *cmd)
{
	int type = lua_type(L, -1);
	if (type == cmd->type) {
		return true;
	}
	if (type == LUA_TNUMBER && cmd->type == WUY_CFLUA_TYPE_INTEGER) {
		return true;
	}
	return false;
}
static int wuy_cflua_set_types(lua_State *L, struct wuy_cflua_command *cmd, void *container)
{
	wuy_cflua_current_cmd = cmd;

	if (!wuy_cflua_check_type(L, cmd)) {
		return WUY_CFLUA_ERR_WRONG_TYPE;
	}

	switch (cmd->type) {
	case WUY_CFLUA_TYPE_BOOLEAN:
		return wuy_cflua_set_boolean(L, cmd, container);
	case WUY_CFLUA_TYPE_INTEGER:
		return wuy_cflua_set_integer(L, cmd, container);
	case WUY_CFLUA_TYPE_DOUBLE:
		return wuy_cflua_set_double(L, cmd, container);
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

static const char *wuy_cflua_strtype(enum wuy_cflua_type type)
{
	switch (type) {
	case WUY_CFLUA_TYPE_END: return "!END";
	case WUY_CFLUA_TYPE_BOOLEAN: return "boolean";
	case WUY_CFLUA_TYPE_INTEGER: return "integer";
	case WUY_CFLUA_TYPE_DOUBLE: return "double";
	case WUY_CFLUA_TYPE_STRING: return "string";
	case WUY_CFLUA_TYPE_FUNCTION: return "function";
	case WUY_CFLUA_TYPE_TABLE: return "table";
	default: return "invalid type!";
	}
}
const char *wuy_cflua_strerror(lua_State *L, int err)
{
	static char buffer[3000];
	char *p = buffer, *end = buffer + sizeof(buffer);

	switch (err) {
	case WUY_CFLUA_ERR_NO_MEM:
		p += sprintf(p, "no-memory");
		break;
	case WUY_CFLUA_ERR_DEPLICATE_MEMBER:
		p += sprintf(p, "duplicated member");
		break;
	case WUY_CFLUA_ERR_POST:
		p += sprintf(p, "table post handler fails");
		break;
	case WUY_CFLUA_ERR_TOO_DEEP_META:
		p += sprintf(p, "too deep metatable");
		break;
	case WUY_CFLUA_ERR_WRONG_TYPE:
		p += sprintf(p, "wrong type of %s, get %s while expect %s",
				wuy_cflua_current_cmd->name ? wuy_cflua_current_cmd->name : "array member",
				lua_typename(L, lua_type(L, -1)),
				wuy_cflua_strtype(wuy_cflua_current_cmd->type));
		break;
	default:
		p += sprintf(p, "!!! impossible error code: %d", err);
		return buffer;
	}

	p += sprintf(p, " at ");

	for (int i = 0; i < wuy_cflua_stack_index; i++) {
		struct wuy_cflua_stack *stack = &wuy_cflua_stacks[i];
		struct wuy_cflua_command *cmd = stack->cmd;
		if (cmd->u.table->name != NULL) {
			p += cmd->u.table->name(*(void **)(stack->container), p, end - p);
		} else if (cmd->name != NULL) {
			p += snprintf(p, end - p, "%s>", cmd->name);
		} else {
			p += snprintf(p, end - p, "{?}>");
		}
	}

	/* delete the last '>' */
	if (p > buffer && p[-1] == '>') {
		p[-1] = '.';
	}

	return buffer;
}
