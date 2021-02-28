#include <lua5.1/lua.h>
#include <lua5.1/lauxlib.h>
#include <lua5.1/lualib.h>

#include "wuy_luastr.h"

static lua_State *wuy_luastr_L = NULL;

static void wuy_luastr_init(void)
{
	printf("wuy_luastr_init\n");
	if (wuy_luastr_L == NULL) {
		return;
	}
	wuy_luastr_L = lua_open();
	luaL_openlibs(wuy_luastr_L);
	printf(" 1\n");
	lua_getglobal(wuy_luastr_L, "string");
	printf(" 2\n");
}

const char *wuy_luastr_gsub(const char *s, const char *pattern, const char *repl)
{
	wuy_luastr_init();

	printf("before %d\n", lua_gettop(wuy_luastr_L));
	lua_getfield(wuy_luastr_L, -1, "gsub");
	printf("after %d\n", lua_gettop(wuy_luastr_L));

	lua_pushstring(wuy_luastr_L, s);
	lua_pushstring(wuy_luastr_L, pattern);
	lua_pushstring(wuy_luastr_L, repl);
	if (lua_pcall(wuy_luastr_L, 3, 2, 0) != 0){
		return NULL;
	}

	const char *out = lua_tostring(wuy_luastr_L, -2);
	int n = lua_tointeger(wuy_luastr_L, -1);

	/* 2 return values and 1 function */
	lua_pop(wuy_luastr_L, 3);

	return n != 0 ? out : NULL;
}

bool wuy_luastr_find(const char *s, const char *pattern)
{
	wuy_luastr_init();

	printf("before %d\n", lua_gettop(wuy_luastr_L));
	lua_getfield(wuy_luastr_L, -1, "find");
	printf("after %d\n", lua_gettop(wuy_luastr_L));

	lua_pushstring(wuy_luastr_L, s);
	lua_pushstring(wuy_luastr_L, pattern);
	if (lua_pcall(wuy_luastr_L, 2, 2, 0) != 0){
		return NULL;
	}

	bool found = lua_isnumber(wuy_luastr_L, -1);

	/* 2 return values and 1 function */
	lua_pop(wuy_luastr_L, 3);

	return found;
}
