#include <lua5.1/lua.h>

static const char *wuy_luatab_safe_globals[] = {
	"ipairs",
	"next",
	"pairs",
	"pcall",
	"tonumber",
	"tostring",
	"type",
	"unpack",

	"coroutine",
	"string",
	"table",
	"math",
};
static const char *wuy_luatab_pkgos_safe_names[] = {
	"clock",
	"difftime",
	"time",
};

#define ARRAYLEN(a) (sizeof(a) / sizeof(a[0]))

static void wuy_luatab_dup_table(lua_State *L, int t)
{
	lua_newtable(L);
	lua_pushnil(L);
	while (lua_next(L, t) != 0) {
		lua_pushvalue(L, -2); /* dup the key */
		lua_insert(L, -2); /* move the key down */

		if (lua_type(L, -1) == LUA_TTABLE) {
			wuy_luatab_dup_table(L, -4);
		}

		/* set and pop key&value, and left one key */
		lua_settable(L, -4);
	}

	lua_replace(L, -2); /* cover the original table */
}

void wuy_luatab_add(lua_State *L, const char *name)
{
	lua_getglobal(L, name);
	if (lua_type(L, -1) == LUA_TTABLE) {
		wuy_luatab_dup_table(L, -2);
	}
	lua_setfield(L, -2, name);
}

void wuy_luatab_add_package_filter(lua_State *L,
		const char *pkg, const char **names, int len)
{
	lua_getglobal(L, pkg);
	lua_newtable(L);
	for (int i = 0; i < len; i++) {
		const char *name = names[i];
		lua_getfield(L, -2, name);
		lua_setfield(L, -2, name);
	}
	lua_setfield(L, -3, pkg); /* set and pop the new table */
	lua_pop(L, 1); /* pop the original table */
}

static int wuy_luatab_meta_newindex(lua_State *L)
{
	lua_pushstring(L, "update readonly table");
	return lua_error(L);
}

void wuy_luatab_set_readonly(lua_State *L)
{
	/* set readonly */
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	lua_pushcfunction(L, wuy_luatab_meta_newindex);
	lua_setfield(L, -2, "__newindex");
	lua_pushvalue(L, -1);
	lua_setmetatable(L, -2);
}

int wuy_luatab_safe_env(lua_State *L)
{
	lua_newtable(L);

	for (int i = 0; i < ARRAYLEN(wuy_luatab_safe_globals); i++) {
		const char *name = wuy_luatab_safe_globals[i];
		wuy_luatab_add(L, name);
	}

	wuy_luatab_add_package_filter(L, "os", wuy_luatab_pkgos_safe_names,
			ARRAYLEN(wuy_luatab_pkgos_safe_names));

	return lua_gettop(L);
}
