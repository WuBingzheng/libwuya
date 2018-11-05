#ifndef WUY_JSON_H
#define WUY_JSON_H

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct {
	char	*start;
	char	*end;
	char	*pos;
} wuy_json_ctx_t;

#define WUY_JSON_CTX_INIT(buf, size) \
	{ .start = buf, .pos = buf, .end = buf + size, }

#define WUY_JSON_CTX(name, buf, size) \
	wuy_json_ctx_t name = WUY_JSON_CTX_INIT(buf, size)

#define WUY_JSON_BUILD(ctx, fmt, ...) \
	if (ctx->pos >= ctx->end) return; \
	ctx->pos += snprintf(ctx->pos, ctx->end - ctx->pos, fmt, ## __VA_ARGS__)

static inline void wuy_json_ctx_init(wuy_json_ctx_t *ctx, char *buf, size_t size)
{
	ctx->start = ctx->pos = buf;
	ctx->end = buf + size;
}

static inline void wuy_json_new_object(wuy_json_ctx_t *ctx)
{
	WUY_JSON_BUILD(ctx, "{");
}

static inline void wuy_json_new_array(wuy_json_ctx_t *ctx)
{
	WUY_JSON_BUILD(ctx, "[");
}

static inline void wuy_json_object_close(wuy_json_ctx_t *ctx)
{
	if (ctx->pos >= ctx->end) {
		return;
	}
	switch (ctx->pos[-1]) {
	case ',':
		ctx->pos--;
	case '{':
		WUY_JSON_BUILD(ctx, "},");
		break;
	default:
		abort();
	}
}

static inline void wuy_json_array_close(wuy_json_ctx_t *ctx)
{
	if (ctx->pos >= ctx->end) {
		return;
	}
	switch (ctx->pos[-1]) {
	case ',':
		ctx->pos--;
	case '[':
		WUY_JSON_BUILD(ctx, "],");
		break;
	default:
		abort();
	}
}

static inline int wuy_json_done(wuy_json_ctx_t *ctx)
{
	if (ctx->pos > ctx->end) {
		return ctx->end - ctx->start;
	}

	if (ctx->pos[-1] != ',') {
		abort();
	}
	ctx->pos[-1] = '\n';
	return ctx->pos - ctx->start;
}

static inline void wuy_json_object_uint(wuy_json_ctx_t *ctx, const char *key, unsigned long n)
{
	WUY_JSON_BUILD(ctx, "\"%s\":%lu,", key, n);
}
static inline void wuy_json_object_int(wuy_json_ctx_t *ctx, const char *key, long n)
{
	WUY_JSON_BUILD(ctx, "\"%s\":%ld,", key, n);
}
static inline void wuy_json_object_double(wuy_json_ctx_t *ctx, const char *key, double n)
{
	WUY_JSON_BUILD(ctx, "\"%s\":%f,", key, n);
}
static inline void wuy_json_object_string(wuy_json_ctx_t *ctx, const char *key, const char *s)
{
	WUY_JSON_BUILD(ctx, "\"%s\":\"%s\",", key, s);
}
static inline void wuy_json_object_raw(wuy_json_ctx_t *ctx, const char *key, const char *s)
{
	WUY_JSON_BUILD(ctx, "\"%s\":%s,", key, s);
}
static inline void wuy_json_object_null(wuy_json_ctx_t *ctx, const char *key)
{
	WUY_JSON_BUILD(ctx, "\"%s\":null,", key);
}
static inline void wuy_json_object_bool(wuy_json_ctx_t *ctx, const char *key, bool b)
{
	WUY_JSON_BUILD(ctx, "\"%s\":%s,", key, b ? "true" : "false");
}
static inline void wuy_json_object_array(wuy_json_ctx_t *ctx, const char *key)
{
	WUY_JSON_BUILD(ctx, "\"%s\":[", key);
}
static inline void wuy_json_object_object(wuy_json_ctx_t *ctx, const char *key)
{
	WUY_JSON_BUILD(ctx, "\"%s\":{", key);
}

static inline void wuy_json_array_uint(wuy_json_ctx_t *ctx, unsigned long n)
{
	WUY_JSON_BUILD(ctx, "%lu,", n);
}
static inline void wuy_json_array_int(wuy_json_ctx_t *ctx, long n)
{
	WUY_JSON_BUILD(ctx, "%ld,", n);
}
static inline void wuy_json_array_double(wuy_json_ctx_t *ctx, double n)
{
	WUY_JSON_BUILD(ctx, "%f,", n);
}
static inline void wuy_json_array_string(wuy_json_ctx_t *ctx, const char *s)
{
	WUY_JSON_BUILD(ctx, "\"%s\",", s);
}
static inline void wuy_json_array_raw(wuy_json_ctx_t *ctx, const char *s)
{
	WUY_JSON_BUILD(ctx, "%s,", s);
}
static inline void wuy_json_array_null(wuy_json_ctx_t *ctx)
{
	WUY_JSON_BUILD(ctx, "null,");
}
static inline void wuy_json_array_bool(wuy_json_ctx_t *ctx, bool b)
{
	WUY_JSON_BUILD(ctx, b ? "true," : "false,");
}
static inline void wuy_json_array_array(wuy_json_ctx_t *ctx)
{
	WUY_JSON_BUILD(ctx, "[");
}
static inline void wuy_json_array_object(wuy_json_ctx_t *ctx)
{
	WUY_JSON_BUILD(ctx, "{");
}

#endif
