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
} wuy_json_t;

/**
 * @brief Context initialization.
 */
#define WUY_JSON_INIT(buf, size) \
	{ .start = buf, .pos = buf, .end = buf + size, .app_data = NULL }

/**
 * @brief Context define and initialization.
 */
#define WUY_JSON(name, buf, size) \
	wuy_json_t name = WUY_JSON_INIT(buf, size)

#define WUY_JSON_BUILD(json, fmt, ...) \
	if (json->pos >= json->end) return; \
	json->pos += snprintf(json->pos, json->end - json->pos, fmt, ## __VA_ARGS__)

/**
 * @brief Initialize a context.
 */
static inline void wuy_json_init(wuy_json_t *json, char *buf, size_t size)
{
	json->start = json->pos = buf;
	json->end = buf + size;
}

/**
 * @brief Start a new object.
 */
static inline void wuy_json_new_object(wuy_json_t *json)
{
	WUY_JSON_BUILD(json, "{");
}

/**
 * @brief Start a new array.
 */
static inline void wuy_json_new_array(wuy_json_t *json)
{
	WUY_JSON_BUILD(json, "[");
}

/**
 * @brief Close an object.
 */
static inline void wuy_json_object_close(wuy_json_t *json)
{
	if (json->pos >= json->end) {
		return;
	}
	switch (json->pos[-1]) {
	case ',':
		json->pos--;
	case '{':
		WUY_JSON_BUILD(json, "},");
		break;
	default:
		abort();
	}
}

/**
 * @brief Close an array.
 */
static inline void wuy_json_array_close(wuy_json_t *json)
{
	if (json->pos >= json->end) {
		return;
	}
	switch (json->pos[-1]) {
	case ',':
		json->pos--;
	case '[':
		WUY_JSON_BUILD(json, "],");
		break;
	default:
		abort();
	}
}

/**
 * @brief Finish the dump.
 */
static inline int wuy_json_done(wuy_json_t *json)
{
	if (json->pos >= json->end) {
		return json->end - json->start;
	}

	if (json->pos[-1] != ',') {
		abort();
	}
	json->pos[-1] = '\n';
	return json->pos - json->start;
}

/**
 * @brief Add an item with unsigned int type value to an object.
 */
static inline void wuy_json_object_uint(wuy_json_t *json, const char *key, unsigned long n)
{
	WUY_JSON_BUILD(json, "\"%s\":%lu,", key, n);
}
/**
 * @brief Add an item with int type value to an object.
 */
static inline void wuy_json_object_int(wuy_json_t *json, const char *key, long n)
{
	WUY_JSON_BUILD(json, "\"%s\":%ld,", key, n);
}
/**
 * @brief Add an item with double type value to an object.
 */
static inline void wuy_json_object_double(wuy_json_t *json, const char *key, double n)
{
	WUY_JSON_BUILD(json, "\"%s\":%f,", key, n);
}
/**
 * @brief Add an item with string type value to an object.
 */
static inline void wuy_json_object_string(wuy_json_t *json, const char *key, const char *s)
{
	WUY_JSON_BUILD(json, "\"%s\":\"%s\",", key, s);
}
/**
 * @brief Add an item with raw type value to an object.
 */
static inline void wuy_json_object_raw(wuy_json_t *json, const char *key, const char *s)
{
	WUY_JSON_BUILD(json, "\"%s\":%s,", key, s);
}
/**
 * @brief Add an item with null value to an object.
 */
static inline void wuy_json_object_null(wuy_json_t *json, const char *key)
{
	WUY_JSON_BUILD(json, "\"%s\":null,", key);
}
/**
 * @brief Add an item with bool type value to an object.
 */
static inline void wuy_json_object_bool(wuy_json_t *json, const char *key, bool b)
{
	WUY_JSON_BUILD(json, "\"%s\":%s,", key, b ? "true" : "false");
}
/**
 * @brief Add an item with array type value to an object.
 */
static inline void wuy_json_object_array(wuy_json_t *json, const char *key)
{
	WUY_JSON_BUILD(json, "\"%s\":[", key);
}
/**
 * @brief Add an item with object type value to an object.
 */
static inline void wuy_json_object_object(wuy_json_t *json, const char *key)
{
	WUY_JSON_BUILD(json, "\"%s\":{", key);
}

/**
 * @brief Append a unsigned int value to an array.
 */
static inline void wuy_json_array_uint(wuy_json_t *json, unsigned long n)
{
	WUY_JSON_BUILD(json, "%lu,", n);
}
/**
 * @brief Append a int value to an array.
 */
static inline void wuy_json_array_int(wuy_json_t *json, long n)
{
	WUY_JSON_BUILD(json, "%ld,", n);
}
/**
 * @brief Append a double value to an array.
 */
static inline void wuy_json_array_double(wuy_json_t *json, double n)
{
	WUY_JSON_BUILD(json, "%f,", n);
}
/**
 * @brief Append a string value to an array.
 */
static inline void wuy_json_array_string(wuy_json_t *json, const char *s)
{
	WUY_JSON_BUILD(json, "\"%s\",", s);
}
/**
 * @brief Append a raw value to an array.
 */
static inline void wuy_json_array_raw(wuy_json_t *json, const char *s)
{
	WUY_JSON_BUILD(json, "%s,", s);
}
/**
 * @brief Append a null value to an array.
 */
static inline void wuy_json_array_null(wuy_json_t *json)
{
	WUY_JSON_BUILD(json, "null,");
}
/**
 * @brief Append a bool value to an array.
 */
static inline void wuy_json_array_bool(wuy_json_t *json, bool b)
{
	WUY_JSON_BUILD(json, b ? "true," : "false,");
}
/**
 * @brief Append an array to an array.
 */
static inline void wuy_json_array_array(wuy_json_t *json)
{
	WUY_JSON_BUILD(json, "[");
}
/**
 * @brief Append an object to an array.
 */
static inline void wuy_json_array_object(wuy_json_t *json)
{
	WUY_JSON_BUILD(json, "{");
}

#endif
