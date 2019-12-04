#ifndef WUY_HTTP_H
#define WUY_HTTP_H

#include <stdint.h>
#include <stdbool.h>

#define WUY_HTTP_METHOD_GET	1
#define WUY_HTTP_METHOD_PUT	2
#define WUY_HTTP_METHOD_POST	3
#define WUY_HTTP_METHOD_HEAD	4
#define WUY_HTTP_METHOD_DELETE	5
#define WUY_HTTP_METHOD_OPTIONS	6
#define WUY_HTTP_METHOD_CONNECT	7

/*
 * @brief Parse and return HTTP method.
 *
 * @param len, must be the method string length, not longer.
 *
 * @return HTTP method if success, or -1 if fail.
 */
int wuy_http_method(const char *buf, int len);

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
int wuy_http_request_line(const char *buf, int len, int *out_method,
		const char **out_url_pos, int *out_url_len, int *out_version);

/*
 * @brief Parse HTTP response status line.
 *
 * @param len, could be longger than the actually line.
 *
 * @return >0 for the actually line length, or -1 if fail, or 0 if expect more data.
 */
int wuy_http_status_line(const char *buf, int len, int *out_status_code,
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
