/**
 * @file     wuy_json.h
 * @author   Wu Bingzheng <wubingzheng@gmail.com>
 * @date     2018-7-19
 *
 * @section LICENSE
 * GPLv2
 *
 * @section DESCRIPTION
 *
 * Some simple json dump utils.
 */

#ifndef WUY_JSON_H
#define WUY_JSON_H

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

/**
 * @brief The context.
 */
typedef struct {
	char	*start;
	char	*end;
	char	*pos;
	void	*app_data;
} wuy_json_ctx_t;

/**
 * @brief Context initialization.
 */
#define WUY_JSON_CTX_INIT(buf, size) \
	{ .start = buf, .pos = buf, .end = buf + size, .app_data = NULL }

/**
 * @brief Context define and initialization.
 */
#define WUY_JSON_CTX(name, buf, size) \
	wuy_json_ctx_t name = WUY_JSON_CTX_INIT(buf, size)

#define WUY_JSON_BUILD(ctx, fmt, ...) \
	if (ctx->pos >= ctx->end) return; \
	ctx->pos += snprintf(ctx->pos, ctx->end - ctx->pos, fmt, ## __VA_ARGS__)

/**
 * @brief Initialize a context.
 */
static inline void wuy_json_ctx_init(wuy_json_ctx_t *ctx, char *buf, size_t size)
{
	ctx->start = ctx->pos = buf;
	ctx->end = buf + size;
}

/**
 * @brief Start a new object.
 */
static inline void wuy_json_new_object(wuy_json_ctx_t *ctx)
{
	WUY_JSON_BUILD(ctx, "{");
}

/**
 * @brief Start a new array.
 */
static inline void wuy_json_new_array(wuy_json_ctx_t *ctx)
{
	WUY_JSON_BUILD(ctx, "[");
}

/**
 * @brief Close an object.
 */
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

/**
 * @brief Close an array.
 */
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

/**
 * @brief Finish the dump.
 */
static inline int wuy_json_done(wuy_json_ctx_t *ctx)
{
	if (ctx->pos >= ctx->end) {
		return ctx->end - ctx->start;
	}

	if (ctx->pos[-1] != ',') {
		abort();
	}
	ctx->pos[-1] = '\n';
	return ctx->pos - ctx->start;
}

/**
 * @brief Add an item with unsigned int type value to an object.
 */
static inline void wuy_json_object_uint(wuy_json_ctx_t *ctx, const char *key, unsigned long n)
{
	WUY_JSON_BUILD(ctx, "\"%s\":%lu,", key, n);
}
/**
 * @brief Add an item with int type value to an object.
 */
static inline void wuy_json_object_int(wuy_json_ctx_t *ctx, const char *key, long n)
{
	WUY_JSON_BUILD(ctx, "\"%s\":%ld,", key, n);
}
/**
 * @brief Add an item with double type value to an object.
 */
static inline void wuy_json_object_double(wuy_json_ctx_t *ctx, const char *key, double n)
{
	WUY_JSON_BUILD(ctx, "\"%s\":%f,", key, n);
}
/**
 * @brief Add an item with string type value to an object.
 */
static inline void wuy_json_object_string(wuy_json_ctx_t *ctx, const char *key, const char *s)
{
	WUY_JSON_BUILD(ctx, "\"%s\":\"%s\",", key, s);
}
/**
 * @brief Add an item with raw type value to an object.
 */
static inline void wuy_json_object_raw(wuy_json_ctx_t *ctx, const char *key, const char *s)
{
	WUY_JSON_BUILD(ctx, "\"%s\":%s,", key, s);
}
/**
 * @brief Add an item with null value to an object.
 */
static inline void wuy_json_object_null(wuy_json_ctx_t *ctx, const char *key)
{
	WUY_JSON_BUILD(ctx, "\"%s\":null,", key);
}
/**
 * @brief Add an item with bool type value to an object.
 */
static inline void wuy_json_object_bool(wuy_json_ctx_t *ctx, const char *key, bool b)
{
	WUY_JSON_BUILD(ctx, "\"%s\":%s,", key, b ? "true" : "false");
}
/**
 * @brief Add an item with array type value to an object.
 */
static inline void wuy_json_object_array(wuy_json_ctx_t *ctx, const char *key)
{
	WUY_JSON_BUILD(ctx, "\"%s\":[", key);
}
/**
 * @brief Add an item with object type value to an object.
 */
static inline void wuy_json_object_object(wuy_json_ctx_t *ctx, const char *key)
{
	WUY_JSON_BUILD(ctx, "\"%s\":{", key);
}

/**
 * @brief Append a unsigned int value to an array.
 */
static inline void wuy_json_array_uint(wuy_json_ctx_t *ctx, unsigned long n)
{
	WUY_JSON_BUILD(ctx, "%lu,", n);
}
/**
 * @brief Append a int value to an array.
 */
static inline void wuy_json_array_int(wuy_json_ctx_t *ctx, long n)
{
	WUY_JSON_BUILD(ctx, "%ld,", n);
}
/**
 * @brief Append a double value to an array.
 */
static inline void wuy_json_array_double(wuy_json_ctx_t *ctx, double n)
{
	WUY_JSON_BUILD(ctx, "%f,", n);
}
/**
 * @brief Append a string value to an array.
 */
static inline void wuy_json_array_string(wuy_json_ctx_t *ctx, const char *s)
{
	WUY_JSON_BUILD(ctx, "\"%s\",", s);
}
/**
 * @brief Append a raw value to an array.
 */
static inline void wuy_json_array_raw(wuy_json_ctx_t *ctx, const char *s)
{
	WUY_JSON_BUILD(ctx, "%s,", s);
}
/**
 * @brief Append a null value to an array.
 */
static inline void wuy_json_array_null(wuy_json_ctx_t *ctx)
{
	WUY_JSON_BUILD(ctx, "null,");
}
/**
 * @brief Append a bool value to an array.
 */
static inline void wuy_json_array_bool(wuy_json_ctx_t *ctx, bool b)
{
	WUY_JSON_BUILD(ctx, b ? "true," : "false,");
}
/**
 * @brief Append an array to an array.
 */
static inline void wuy_json_array_array(wuy_json_ctx_t *ctx)
{
	WUY_JSON_BUILD(ctx, "[");
}
/**
 * @brief Append an object to an array.
 */
static inline void wuy_json_array_object(wuy_json_ctx_t *ctx)
{
	WUY_JSON_BUILD(ctx, "{");
}

#endif
