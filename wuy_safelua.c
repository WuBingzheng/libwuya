#include <lua5.1/lua.h>

static const char *wuy_safelua_global_names[] = {
	"ipairs",
	"next",
	"pairs",
	"pcall",
	"tonumber",
	"tostring",
	"type",
	"unpack",
};
static const char *wuy_safelua_global_packages[] = {
	"coroutine",
	"string",
	"table",
	"math",
};
static const char *wuy_safelua_pkgos_safe_names[] = {
	"clock",
	"difftime",
	"time",
};

#define ARRAYLEN(a) (sizeof(a) / sizeof(a[0]))

void wuy_safelua_add_package_names(lua_State *L,
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

static int wuy_safelua_meta_index(lua_State *L)
{
	lua_getfield(L, -2, "_index_data");
	lua_pushvalue(L, -2); /* dup key for lua_gettable() */
	lua_gettable(L, -2);
	if (lua_isnil(L, -1)) {
		return 0;
	}

	/* set the value for later using */
	// TODO if table, call wuy_safelua_add_package() for safe, recursive
	lua_pushvalue(L, 2); /* key */
	lua_pushvalue(L, -2); /* value */
	lua_settable(L, 1);

	return 1;
}

/* use metamethod to duplicate the table as-necessary lazy */
void wuy_safelua_add_package(lua_State *L, const char *name)
{
	lua_getglobal(L, name);

	lua_newtable(L);

	lua_insert(L, -2);
	lua_setfield(L, -2, "_index_data"); /* also pop the original table */

	lua_pushcfunction(L, wuy_safelua_meta_index);
	lua_setfield(L, -2, "__index");

	lua_pushvalue(L, -1); /* self-meta */
	lua_setmetatable(L, -2);

	lua_setfield(L, -2, name);
}

int wuy_safelua_new(lua_State *L)
{
	lua_newtable(L);

	for (int i = 0; i < ARRAYLEN(wuy_safelua_global_names); i++) {
		const char *name = wuy_safelua_global_names[i];
		lua_getglobal(L, name);
		lua_setfield(L, -2, name);
	}
	for (int i = 0; i < ARRAYLEN(wuy_safelua_global_packages); i++) {
		const char *name = wuy_safelua_global_packages[i];
		wuy_safelua_add_package(L, name);
	}
	wuy_safelua_add_package_names(L, "os", wuy_safelua_pkgos_safe_names,
			ARRAYLEN(wuy_safelua_pkgos_safe_names));

	return lua_gettop(L);
}
