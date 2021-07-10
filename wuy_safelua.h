#ifndef WUY_SAFELUA_H
#define WUY_SAFELUA_H

#include <lua5.1/lua.h>

/* Create a safe Lua ENV, put it at top of stack, and return the top index */
int wuy_safelua_new(lua_State *L);

/* Add a packet. */
void wuy_safelua_add_package(lua_State *L, const char *name);

/* Add a packet only names. */
void wuy_safelua_add_package_names(lua_State *L,
		const char *pkg, const char **names, int len);

#endif
