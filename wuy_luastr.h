#ifndef WUY_LUASTR_H
#define WUY_LUASTR_H

#include <stdbool.h>

const char *wuy_luastr_gsub(const char *s, const char *pattern, const char *repl);

int wuy_luastr_find(const char *s, const char *pattern, int *p_end);

static inline bool wuy_luastr_find2(const char *s, const char *pattern)
{
	int end;
	int start = wuy_luastr_find(s, pattern, &end);
	return start >= 0;
}

#endif
