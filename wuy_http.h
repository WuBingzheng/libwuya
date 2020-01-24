#ifndef WUY_HTTP_H
#define WUY_HTTP_H

#include <stdint.h>
#include <stdbool.h>

#define WUY_HTTP_METHOD_TABLE \
	X(GET) \
	X(PUT) \
	X(POST) \
	X(HEAD) \
	X(DELETE) \
	X(OPTIONS) \
	X(CONNECT)

enum wuy_http_method {
	WUY_HTTP_NOUSE_ZERO = 0,
#define X(m) WUY_HTTP_##m,
	WUY_HTTP_METHOD_TABLE
#undef X
};

#define WUY_HTTP_STATUS_CODE_TABLE \
	X(200, "OK") \
	X(201, "Created") \
	X(202, "Accepted") \
	X(204, "No Content") \
	X(301, "Moved Permanently") \
	X(302, "Found") \
	X(303, "See Other") \
	X(307, "Temporary Redirect") \
	X(400, "Bad Request") \
	X(401, "Unauthorized") \
	X(403, "Forbidden") \
	X(404, "Not Found") \
	X(405, "Method Not Allowed") \
	X(406, "Not Acceptable") \
	X(408, "Request Timeout") \
	X(500, "Internal Server Error") \
	X(502, "Bad Gateway") \
	X(503, "Service Unavailable") \
	X(504, "Gateway Timeout")

enum wuy_http_status_code {
#define X(n, s) WUY_HTTP_##n = n,
	WUY_HTTP_STATUS_CODE_TABLE
#undef X
};

/*
 * @brief Parse and return HTTP method.
 *
 * @param len, must be the method string length, not longer.
 *
 * @return HTTP method if success, or -1 if fail.
 */
enum wuy_http_method wuy_http_method(const char *buf, int len);

/*
 * @brief Convert method to string.
 */
const char *wuy_http_string_method(enum wuy_http_method method);

/*
 * @brief Convert status code to string.
 */
const char *wuy_http_string_status_code(enum wuy_http_status_code code);

/*
 * @brief Parse and return HTTP version.
 *
 * @param len, must be the version string length, not longer.
 *
 * @return 0 for HTTP/1.0, 1 for HTTP/1.1, or -1 if fail.
 */
int wuy_http_version(const char *buf, int len);

/*
 * @brief Parse HTTP request line.
 *
 * @param len, could be longger than the actually line.
 *
 * @return >0 for the actually line length, or -1 if fail, or 0 if expect more data.
 */
int wuy_http_request_line(const char *buf, int len, enum wuy_http_method *out_method,
		const char **out_url_pos, int *out_url_len, int *out_version);

/*
 * @brief Parse HTTP response status line.
 *
 * @param len, could be longger than the actually line.
 *
 * @return >0 for the actually line length, or -1 if fail, or 0 if expect more data.
 */
int wuy_http_status_line(const char *buf, int len, enum wuy_http_status_code *out_status_code,
		int *out_version);

/*
 * @brief Parse a HTTP header.
 *
 * @param len, could be longger than the actually header.
 *
 * @return 2 for end of headers, or >0 for the actually header length,
 *         or -1 if fail, or 0 if expect more data.
 */
int wuy_http_header(const char *buf, int len, int *out_name_len,
		const char **out_value_start, int *out_value_len);


typedef struct {
	int	state;
	int	size;
} wuy_http_chunked_t;

int wuy_http_chunked_process(wuy_http_chunked_t *chunked, const uint8_t *in_buf,
		int in_len, uint8_t *out_buf, int *p_out_len);

void wuy_http_chunked_init(wuy_http_chunked_t *chunked);
void wuy_http_chunked_enable(wuy_http_chunked_t *chunked);
bool wuy_http_chunked_is_enabled(const wuy_http_chunked_t *chunked);
bool wuy_http_chunked_is_finished(const wuy_http_chunked_t *chunked);

#endif
