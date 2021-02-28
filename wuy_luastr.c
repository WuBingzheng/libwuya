#include <lua5.1/lua.h>
#include <lua5.1/lauxlib.h>
#include <lua5.1/lualib.h>
#include <assert.h>

#include "wuy_luastr.h"

static lua_State *wuy_luastr_L = NULL;

static void wuy_luastr_init(void)
{
	if (wuy_luastr_L == NULL) {
		wuy_luastr_L = lua_open();
		luaL_openlibs(wuy_luastr_L);
		lua_getglobal(wuy_luastr_L, "string");
	}
}

const char *wuy_luastr_gsub(const char *s, const char *pattern, const char *repl)
{
	wuy_luastr_init();

	lua_getfield(wuy_luastr_L, -1, "gsub");

	/* arguments */
	lua_pushstring(wuy_luastr_L, s);
	lua_pushstring(wuy_luastr_L, pattern);
	lua_pushstring(wuy_luastr_L, repl);

	/* run */
	assert(lua_pcall(wuy_luastr_L, 3, 2, 0) == 0);

	/* 2 return values */
	const char *out = lua_tostring(wuy_luastr_L, -2);
	int n = lua_tointeger(wuy_luastr_L, -1);

	lua_pop(wuy_luastr_L, 2);

	return n != 0 ? out : NULL;
}

int wuy_luastr_find(const char *s, const char *pattern, int *p_end)
{
	wuy_luastr_init();

	lua_getfield(wuy_luastr_L, -1, "find");

	/* arguments */
	lua_pushstring(wuy_luastr_L, s);
	lua_pushstring(wuy_luastr_L, pattern);

	/* run */
	assert(lua_pcall(wuy_luastr_L, 2, 2, 0) == 0);

	/* 2 return values */
	int start = lua_tonumber(wuy_luastr_L, -2) - 1;
	*p_end = lua_tonumber(wuy_luastr_L, -1) - 1;

	lua_pop(wuy_luastr_L, 2);

	return start;
}
