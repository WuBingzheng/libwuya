#ifndef WUY_LUASTR_H
#define WUY_LUASTR_H

#include <stdbool.h>

const char *wuy_luastr_gsub(const char *s, const char *pattern, const char *repl);

bool wuy_luastr_find(const char *s, const char *pattern);

#endif
