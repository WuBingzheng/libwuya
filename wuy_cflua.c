#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <lua5.1/lua.h>
#include <lua5.1/lauxlib.h>

#include "wuy_cflua.h"

enum wuy_cflua_error {
	WUY_CFLUA_ERR_OK = 0,
	WUY_CFLUA_ERR_NO_MEM,
	WUY_CFLUA_ERR_DEPLICATE_MEMBER,
	WUY_CFLUA_ERR_POST,
	WUY_CFLUA_ERR_TOO_DEEP_META,
	WUY_CFLUA_ERR_WRONG_TYPE,
	WUY_CFLUA_ERR_LIMIT,
	WUY_CFLUA_ERR_NO_ARRAY,
	WUY_CFLUA_ERR_INVALID_TYPE,
	WUY_CFLUA_ERR_INVALID_CMD,
	WUY_CFLUA_ERR_ARBITRARY,
};

/* table-stack for wuy_cflua_strerror() */
static struct wuy_cflua_stack {
	struct wuy_cflua_command	*cmd;
	void				*container;
} wuy_cflua_stacks[100];
static int wuy_cflua_stack_index = 0;

static const char *wuy_cflua_post_err;
const char *wuy_cflua_post_arg; /* set by user */

static const char *wuy_cflua_arbitrary_err;
const char *wuy_cflua_arbitrary_arg; /* set by user */

static struct wuy_cflua_command	*wuy_cflua_current_cmd;

#define WUY_CFLUA_PINT(container, offset)	*(int *)((char *)(container) + offset)

/* assign value for different types */
#define wuy_cflua_assign_value(cmd, container, value) \
	*(typeof(value) *)(((char *)container) + (cmd)->offset) = value

static int wuy_cflua_set_function(lua_State *L, struct wuy_cflua_command *cmd, void *container)
{
	lua_pushvalue(L, -1);
	wuy_cflua_function_t f = luaL_ref(L, LUA_REGISTRYINDEX);
	wuy_cflua_assign_value(cmd, container, f);
	return 0;
}
static int wuy_cflua_set_string(lua_State *L, struct wuy_cflua_command *cmd, void *container)
{
	size_t len;
	char *value = strdup(lua_tolstring(L, -1, &len));
	wuy_cflua_assign_value(cmd, container, value);
	if (cmd->u.length_offset != 0) {
		WUY_CFLUA_PINT(container, cmd->u.length_offset) = len;
	}
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
	int value = lua_tointeger(L, -1);
	if (cmd->limits.n.is_lower && value < cmd->limits.n.lower) {
		return WUY_CFLUA_ERR_LIMIT;
	}
	if (cmd->limits.n.is_upper && value > cmd->limits.n.upper) {
		return WUY_CFLUA_ERR_LIMIT;
	}

	wuy_cflua_assign_value(cmd, container, value);
	return 0;
}
static int wuy_cflua_set_double(lua_State *L, struct wuy_cflua_command *cmd, void *container)
{
	/* we assume that lua_Number is double */
	lua_Number value = lua_tonumber(L, -1);

	if (cmd->limits.d.is_lower && value < cmd->limits.d.lower) {
		return WUY_CFLUA_ERR_LIMIT;
	}
	if (cmd->limits.d.is_upper && value > cmd->limits.d.upper) {
		return WUY_CFLUA_ERR_LIMIT;
	}

	wuy_cflua_assign_value(cmd, container, value);
	return 0;
}

static int wuy_cflua_set_types(lua_State *L, struct wuy_cflua_command *cmd, void *container);
static void wuy_cflua_set_default_value(lua_State *L, struct wuy_cflua_command *cmd, void *container);

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

static int wuy_cflua_valid_command(lua_State *L, struct wuy_cflua_table *table, void *container)
{
	struct wuy_cflua_command *cmd;

	switch (lua_type(L, -2)) {
	case LUA_TNUMBER:
		for (cmd = table->commands; cmd->type != WUY_CFLUA_TYPE_END; cmd++) {
			if (cmd->name == NULL) {
				return 0;
			}
		}
		if (table->arbitrary == NULL) {
			return WUY_CFLUA_ERR_NO_ARRAY;
		}
		break;
	case LUA_TSTRING:;
		const char *name = lua_tostring(L, -2);
		if (name[0] == '_') {
			return 0;
		}
		for (cmd = table->commands; cmd->type != WUY_CFLUA_TYPE_END; cmd++) {
			if (cmd->name != NULL) {
				if (strcmp(cmd->name, name) == 0) {
					return 0;
				}
			} else if (cmd->is_extra_commands) {
				assert(cmd->type == WUY_CFLUA_TYPE_TABLE);
				if (wuy_cflua_valid_command(L, cmd->u.table, container) == 0) {
					return 0;
				}
			}
		}
		if (cmd->u.next != NULL) {
			struct wuy_cflua_command *(*next)(struct wuy_cflua_command *) = cmd->u.next;
			while ((cmd = next(cmd)) != NULL) {
				if (strcmp(cmd->name, name) == 0) {
					return 0;
				}
			}
		}

		if (table->arbitrary == NULL) {
			return WUY_CFLUA_ERR_INVALID_CMD;
		}
		break;
	default:
		return WUY_CFLUA_ERR_INVALID_TYPE;
	}

	errno = 0;
	wuy_cflua_arbitrary_err = NULL;
	const char *err = table->arbitrary(L, container);
	if (err != WUY_CFLUA_OK) {
		wuy_cflua_arbitrary_err = err;
		return WUY_CFLUA_ERR_ARBITRARY;
	}
	return 0;
}

static int wuy_cflua_set_array_members(lua_State *L, struct wuy_cflua_command *cmd, void *container)
{
	if (lua_isnil(L, -1)) {
		wuy_cflua_set_default_value(L, cmd, container);
		return 0;
	}

	/* find the real table */
	size_t objlen;
	int meta_level = 0;
	while ((objlen = lua_objlen(L, -1)) == 0) {
		if (!lua_getmetatable(L, -1)) {
			wuy_cflua_set_default_value(L, cmd, container);
			lua_pop(L, meta_level);
			return 0;
		}

		meta_level++;
		if (meta_level > 10) {
			return WUY_CFLUA_ERR_TOO_DEEP_META;
		}
	}

	/* OK, now let's read the array members */

	/* unique-member, just assign the single value */
	if (cmd->is_single_array) {
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
		WUY_CFLUA_PINT(container, cmd->array_number_offset) = objlen;
	}
out:
	if (cmd->meta_level_offset != 0) {
		WUY_CFLUA_PINT(container, cmd->meta_level_offset) = meta_level;
	}

	if (meta_level > 0) {
		lua_pop(L, meta_level);
	}
	return 0;
}

static int wuy_cflua_set_option(lua_State *L, struct wuy_cflua_command *cmd, void *container)
{
	if (lua_isnil(L, -1)) {
		wuy_cflua_set_default_value(L, cmd, container);
		return 0;
	}

	lua_getfield(L, -1, cmd->name);
	if (lua_isnil(L, -1)) {
		wuy_cflua_set_default_value(L, cmd, container);
		lua_pop(L, 1);
		return 0;
	}

	int ret = wuy_cflua_set_types(L, cmd, container);
	if (ret != 0) {
		return ret;
	}
	lua_pop(L, 1);

	/* calculate meta_level */
	if (cmd->meta_level_offset != 0) {
		int meta_level = 0;
		while (1) {
			lua_pushstring(L, cmd->name);
			lua_rawget(L, -2);
			if (!lua_isnil(L, -1)) {
				lua_pop(L, 1);
				break;
			}
			lua_pop(L, 1);

			meta_level++;
			lua_getmetatable(L, -1);
		}
		if (meta_level > 0) {
			lua_pop(L, meta_level);
		}

		WUY_CFLUA_PINT(container, cmd->meta_level_offset) = meta_level;
	}

	return 0;
}

static int wuy_cflua_set_table(lua_State *L, struct wuy_cflua_command *cmd, void *container)
{
	struct wuy_cflua_table *table = cmd->u.table;
	const char *name = cmd->name;

	if (table->size == 0) { /* use this container with offset */
		container = (char *)container + cmd->offset;

	} else { /* reuse or allocate new container */

		/* try to reuse the exist container */
		lua_pushvalue(L, -1);
		lua_gettable(L, LUA_REGISTRYINDEX);
		void *hit = lua_touserdata(L, -1);
		lua_pop(L, 1);
		if (hit != NULL) {
			wuy_cflua_assign_value(cmd, container, hit);
			return 0;
		}

		/* allocate new container */
		void *new_container = calloc(1, table->size);
		if (new_container == NULL) {
			return WUY_CFLUA_ERR_NO_MEM;
		}

		wuy_cflua_assign_value(cmd, container, new_container);
		container = new_container;

		/* cache it for reuse later */
		lua_pushvalue(L, -1);
		lua_pushlightuserdata(L, container);
		lua_settable(L, LUA_REGISTRYINDEX);
	}

	wuy_cflua_stacks[wuy_cflua_stack_index].cmd = cmd->real ? cmd->real : cmd;
	wuy_cflua_stacks[wuy_cflua_stack_index].container = container;
	wuy_cflua_stack_index++;

	if (table->init != NULL) {
		table->init(container);
	}

	/* walk through table->commands against config */
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

	/* walk through config against table->commands to validate, only if non-internal */
	if (name == NULL || name[0] != '_') {
		lua_pushnil(L);
		while (lua_next(L, -2) != 0) {
			int ret = wuy_cflua_valid_command(L, table, container);
			if (ret != 0) {
				return ret;
			}
			lua_pop(L, 1); /* value */
		}
	}

	if (table->post != NULL) {
		errno = 0;
		wuy_cflua_post_arg = NULL;
		const char *err = table->post(container);
		if (err != WUY_CFLUA_OK) {
			wuy_cflua_post_err = err;
			return WUY_CFLUA_ERR_POST;
		}
	}

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

static void wuy_cflua_set_default_value(lua_State *L, struct wuy_cflua_command *cmd, void *container)
{
	if (cmd->meta_level_offset != 0) {
		WUY_CFLUA_PINT(container, cmd->meta_level_offset) = -1;
	}

	if (cmd->name == NULL && !cmd->is_single_array) {
		wuy_cflua_assign_value(cmd, container, cmd->default_value.p);
		return;
	}

	switch (cmd->type) {
	case WUY_CFLUA_TYPE_BOOLEAN:
		wuy_cflua_assign_value(cmd, container, cmd->default_value.b);
		return;
	case WUY_CFLUA_TYPE_INTEGER:
		wuy_cflua_assign_value(cmd, container, cmd->default_value.n);
		return;
	case WUY_CFLUA_TYPE_DOUBLE:
		wuy_cflua_assign_value(cmd, container, cmd->default_value.d);
		return;
	case WUY_CFLUA_TYPE_STRING:
		wuy_cflua_assign_value(cmd, container, cmd->default_value.s);
		if (cmd->u.length_offset != 0 && cmd->default_value.s != NULL) {
			WUY_CFLUA_PINT(container, cmd->u.length_offset) = strlen(cmd->default_value.s);
		}
		return;
	case WUY_CFLUA_TYPE_FUNCTION:
		wuy_cflua_assign_value(cmd, container, cmd->default_value.f);
		return;
	case WUY_CFLUA_TYPE_TABLE:
		if (cmd->default_value.t != NULL) {
			wuy_cflua_assign_value(cmd, container, cmd->default_value.t);
		} else {
			lua_newtable(L);
			wuy_cflua_set_table(L, cmd, container);
			lua_pop(L, 1);
		}
		return;
	default:
		abort();
	}
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
static const char *wuy_cflua_strerror(lua_State *L, int err)
{
	if (err == WUY_CFLUA_ERR_OK) {
		return WUY_CFLUA_OK;
	}

	static char buffer[3000];
	char *p = buffer, *end = buffer + sizeof(buffer);

	/* stack */
	for (int i = 0; i < wuy_cflua_stack_index; i++) {
		struct wuy_cflua_stack *stack = &wuy_cflua_stacks[i];
		struct wuy_cflua_command *cmd = stack->cmd;
		if (cmd->u.table->name != NULL) {
			p += cmd->u.table->name(stack->container, p, end - p);
		} else if (cmd->name != NULL) {
			p += snprintf(p, end - p, "%s>", cmd->name);
		} else {
			p += snprintf(p, end - p, "[ARRAY]>");
		}
	}

	/* delete the last '>' */
	if (p > buffer && p[-1] == '>') {
		p[-1] = ':';
	}
	*p++ = ' ';

	/* reason */
	switch (err) {
	case WUY_CFLUA_ERR_OK:
		return WUY_CFLUA_OK;
	case WUY_CFLUA_ERR_NO_MEM:
		p += sprintf(p, "no-memory");
		break;
	case WUY_CFLUA_ERR_DEPLICATE_MEMBER:
		p += sprintf(p, "duplicated member");
		break;
	case WUY_CFLUA_ERR_POST:
		p += sprintf(p, "post handler fails: %s", wuy_cflua_post_err);
		if (wuy_cflua_post_arg != NULL) {
			p += sprintf(p, " `%s`", wuy_cflua_post_arg);
		}
		if (errno != 0) {
			p += sprintf(p, ": %s", strerror(errno));
		}
		break;
	case WUY_CFLUA_ERR_ARBITRARY:
		p += sprintf(p, "arbitrary handler fails for %s: %s",
				lua_tostring(L, -2), wuy_cflua_arbitrary_err);
		if (wuy_cflua_arbitrary_arg != NULL) {
			p += sprintf(p, " `%s`", wuy_cflua_arbitrary_arg);
		}
		if (errno != 0) {
			p += sprintf(p, ": %s", strerror(errno));
		}
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
	case WUY_CFLUA_ERR_LIMIT:
		p += sprintf(p, "%s out of range: ", wuy_cflua_current_cmd->name);
		if (wuy_cflua_current_cmd->type == WUY_CFLUA_TYPE_INTEGER) {
			struct wuy_cflua_command_limits_int *limits = &wuy_cflua_current_cmd->limits.n;
			if (limits->is_lower && limits->is_upper) {
				p += sprintf(p, "[%d, %d]", limits->lower, limits->upper);
			} else if (limits->is_lower) {
				p += sprintf(p, "[%d, -)", limits->lower);
			} else {
				p += sprintf(p, "(-, %d]", limits->upper);
			}
		} else {
			struct wuy_cflua_command_limits_double *limits = &wuy_cflua_current_cmd->limits.d;
			if (limits->is_lower && limits->is_upper) {
				p += sprintf(p, "[%g, %g]", limits->lower, limits->upper);
			} else if (limits->is_lower) {
				p += sprintf(p, "[%g, -)", limits->lower);
			} else {
				p += sprintf(p, "(-, %g]", limits->upper);
			}
		}
		break;
	case WUY_CFLUA_ERR_NO_ARRAY:
		p += sprintf(p, "array member is not allowed");
		break;
	case WUY_CFLUA_ERR_INVALID_TYPE:
		p += sprintf(p, "invalid command key type: %s", lua_typename(L, lua_type(L, -2)));
		break;
	case WUY_CFLUA_ERR_INVALID_CMD:
		p += sprintf(p, "invalid command: %s", lua_tostring(L, -2));
		break;
	default:
		p += sprintf(p, "!!! impossible error code: %d", err);
		return buffer;
	}

	return buffer;
}

const char *wuy_cflua_parse(lua_State *L, struct wuy_cflua_table *table, void *container)
{
	struct wuy_cflua_command tmp = {
		.type = WUY_CFLUA_TYPE_TABLE,
		.u.table = table,
	};

	wuy_cflua_stack_index = 0;
	int ret = wuy_cflua_set_table(L, &tmp, container);

	return wuy_cflua_strerror(L, ret);
}


void wuy_cflua_build_tables(lua_State *L, struct wuy_cflua_table *table)
{
	lua_newtable(L);

	struct wuy_cflua_command *cmd;
	for (cmd = table->commands; cmd->type != WUY_CFLUA_TYPE_END; cmd++) {
		if (cmd->type != WUY_CFLUA_TYPE_TABLE) {
			continue;
		}
		wuy_cflua_build_tables(L, cmd->u.table);
		if (cmd->name == NULL) {
			lua_rawseti(L, -2, 1);
		} else {
			lua_setfield(L, -2, cmd->name);
		}
	}
	if (cmd->u.next != NULL) {
		struct wuy_cflua_command *(*next)(struct wuy_cflua_command *) = cmd->u.next;
		while ((cmd = next(cmd)) != NULL) {
			if (cmd->type != WUY_CFLUA_TYPE_TABLE) {
				continue;
			}
			wuy_cflua_build_tables(L, cmd->u.table);
			lua_setfield(L, -2, cmd->name);
		}
	}
}

static void wuy_cflua_copy_cmd_default(struct wuy_cflua_command *dcmd,
		const struct wuy_cflua_command *scmd, const void *default_container)
{
	*dcmd = *scmd;

	const void *def = (const char *)default_container + scmd->offset;
	if (scmd->type != WUY_CFLUA_TYPE_TABLE) {
		memcpy(&dcmd->default_value, def, wuy_cflua_type_size(scmd->type));
	} else if (scmd->u.table->size == 0) {
		dcmd->u.table = wuy_cflua_copy_table_default(scmd->u.table, def);
	} else if (*(const void **)def != NULL) {
		def = *(const void **)def;
		dcmd->u.table = wuy_cflua_copy_table_default(scmd->u.table, def);
		dcmd->default_value.t = def;
	} else {
		dcmd->real = (void *)1;
	}
}
struct wuy_cflua_table *wuy_cflua_copy_table_default(const struct wuy_cflua_table *src,
		const void *default_container)
{
	struct wuy_cflua_table *dest = malloc(sizeof(struct wuy_cflua_table));
	*dest = *src;

	int cmd_num = 0;
	struct wuy_cflua_command *scmd;
	for (scmd = src->commands; scmd->type != WUY_CFLUA_TYPE_END; scmd++) {
		cmd_num++;
	}
	if (scmd->u.next != NULL) {
		struct wuy_cflua_command *(*next)(struct wuy_cflua_command *) = scmd->u.next;
		while ((scmd = next(scmd)) != NULL) {
			cmd_num++;
		}
	}

	dest->commands = calloc(cmd_num + 1, sizeof(struct wuy_cflua_command));
	struct wuy_cflua_command *dcmd;
	for (scmd = src->commands, dcmd = dest->commands; scmd->type != WUY_CFLUA_TYPE_END; scmd++, dcmd++) {
		wuy_cflua_copy_cmd_default(dcmd, scmd, default_container);
	}
	if (scmd->u.next != NULL) {
		struct wuy_cflua_command *(*next)(struct wuy_cflua_command *) = scmd->u.next;
		while ((scmd = next(scmd)) != NULL) {
			wuy_cflua_copy_cmd_default(dcmd, scmd, default_container);
			dcmd++;
		}
	}
	dcmd->type = WUY_CFLUA_TYPE_END;

	return dest;
}
void wuy_cflua_free_copied_table(struct wuy_cflua_table *table)
{
	struct wuy_cflua_command *cmd;
	for (cmd = table->commands; cmd->type != WUY_CFLUA_TYPE_END; cmd++) {
		if (cmd->type == WUY_CFLUA_TYPE_TABLE && cmd->real == NULL) {
			wuy_cflua_free_copied_table(cmd->u.table);
		}
	}
	assert(cmd->u.next == NULL);
	free(table->commands);
	free(table);
}


/* === dump to markdown === */

static void wuy_cflua_dump_indent(int level)
{
	for (int i = 0; i < level; i++) {
		printf("    ");
	}
}
static void wuy_cflua_dump_command_markdown(struct wuy_cflua_command *cmd, int level)
{
	static char leaders[] = {'+', '-', '*', '=', '>', ':', 'o'};

	wuy_cflua_dump_indent(level);
	printf("%c `%s` _(%s", leaders[level],
			cmd->name ? cmd->name : cmd->is_single_array ? "SINGLE_ARRAY_MEMBER" : "MULTIPLE_ARRAY_MEMBER",
			wuy_cflua_strtype(cmd->type));

	switch (cmd->type) {
	case WUY_CFLUA_TYPE_BOOLEAN:
		if (cmd->default_value.b) {
			printf(":true");
		}
		break;
	case WUY_CFLUA_TYPE_INTEGER:
		if (cmd->default_value.n != 0) {
			printf(": %d", cmd->default_value.n);
		}
		if (cmd->limits.n.is_lower) {
			printf(", min=%d", cmd->limits.n.lower);
		}
		if (cmd->limits.n.is_upper) {
			printf(", max=%d", cmd->limits.n.upper);
		}
		break;
	case WUY_CFLUA_TYPE_DOUBLE:
		if (cmd->default_value.d != 0) {
			printf(": %g", cmd->default_value.d);
		}
		if (cmd->limits.d.is_lower) {
			printf(", min=%g", cmd->limits.d.lower);
		}
		if (cmd->limits.d.is_upper) {
			printf(", max=%g", cmd->limits.d.upper);
		}
		break;
	case WUY_CFLUA_TYPE_STRING:
		if (cmd->default_value.s != NULL) {
			printf(": \"%s\"", cmd->default_value.s);
		}
		break;
	case WUY_CFLUA_TYPE_FUNCTION:
		break;
	case WUY_CFLUA_TYPE_TABLE:
		if (cmd->u.table->refer_name != NULL) {
			printf(".%s", cmd->u.table->refer_name);
		}
		break;
	default:
		abort();
	}

	printf(")_\n\n");

	if (cmd->description != NULL) {
		wuy_cflua_dump_indent(level + 1);
		printf("%s\n\n", cmd->description);
	}

	if (cmd->type == WUY_CFLUA_TYPE_TABLE && cmd->u.table->refer_name == NULL) {
		wuy_cflua_dump_table_markdown(cmd->u.table, level + 1);
	}
}

void wuy_cflua_dump_table_markdown(const struct wuy_cflua_table *table, int level)
{
	struct wuy_cflua_command *cmd;
	for (cmd = table->commands; cmd->type != WUY_CFLUA_TYPE_END; cmd++) {
		if (cmd->name != NULL && cmd->name[0] == '_') {
			continue;
		}

		wuy_cflua_dump_command_markdown(cmd, level);
	}

	int count = 0;
	if (cmd->u.next != NULL) {
		struct wuy_cflua_command *(*next)(struct wuy_cflua_command *) = cmd->u.next;
		while ((cmd = next(cmd)) != NULL) {
			if (count++ > 10) break;
			wuy_cflua_dump_command_markdown(cmd, level);
		}
	}
}
