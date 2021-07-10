#ifndef WUY_LUATAB_H
#define WUY_LUATAB_H

#include <lua5.1/lua.h>

/* Create a safe Lua ENV, put it at top of stack, and return the top index */
int wuy_luatab_safe_env(lua_State *L);

/* Add a member to table at stack-top */
void wuy_luatab_add(lua_State *L, const char *pkg);

/* Add a package partially to table at stack-top */
void wuy_luatab_add_package_filter(lua_State *L, const char *pkg,
		const char **names, int len);

/* Set table readonly at stack-top */
void wuy_luatab_set_readonly(lua_State *L);

#endif
