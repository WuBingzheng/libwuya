#define _XOPEN_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE // memrchr()
#include <stdio.h>
#include <time.h>
#include <string.h>

#include "wuy_http.h"

#define WUY_HTTP_ERROR -1
#define WUY_HTTP_AGAIN 0

enum wuy_http_method wuy_http_method(const char *buf, int len)
{
#define X(m) if (len == sizeof(#m)-1 && memcmp(buf, #m, sizeof(#m)-1) == 0) return WUY_HTTP_##m;
	WUY_HTTP_METHOD_TABLE
#undef X
	return WUY_HTTP_ERROR;
}

const char *wuy_http_string_method(enum wuy_http_method method)
{
	switch (method) {
#define X(m) case WUY_HTTP_##m: return #m;
	WUY_HTTP_METHOD_TABLE
#undef X
	default: return NULL;
	}
}

const char *wuy_http_string_status_code(enum wuy_http_status_code code)
{
	switch (code) {
#define X(n, s) case n: return s;
	WUY_HTTP_STATUS_CODE_TABLE
#undef X
	default:
		return "XXX";
	}
}

int wuy_http_version(const char *buf, int len)
{
	if (len != sizeof("HTTP/1.0") - 1) {
		return WUY_HTTP_ERROR;
	}
	if (memcmp(buf, "HTTP/1.", 7) != 0) {
		return WUY_HTTP_ERROR;
	}
	switch (buf[7]) {
	case '0':
		return 0;
	case '1':
		return 1;
	default:
		return WUY_HTTP_ERROR;
	}
}

static int wuy_http_char_hex(char c)
{
	if (c >= '0' && c <= '9') {
		return c - '0';
	}
	if (c >= 'a' && c <= 'z') {
		return c - 'a' + 10;
	}
	if (c >= 'A' && c <= 'Z') {
		return c - 'A' + 10;
	}
	return -1;
}

/* TODO this does not follow RFC... */
int wuy_http_decode_path(char *dest, const char *src, int len)
{
	char *p = dest;
	for (int i = 0; i < len; i++) {
		char c = src[i];
		if (c == '\0') {
			break;
		}
		if (c < 0) { /* not ascii */
			return WUY_HTTP_ERROR;
		}
		if (c <= 0x20 || c == 0x7F) {
			return WUY_HTTP_ERROR;
		}
		if (c == '%') {
			if (i + 2 >= len) {
				return WUY_HTTP_ERROR;
			}
			int n1 = wuy_http_char_hex(src[++i]);
			int n2 = wuy_http_char_hex(src[++i]);
			if (n1 < 0 || n2 < 0) {
				return WUY_HTTP_ERROR;
			}
			c = n1 * 16 + n2;
		}
		if (c == '/' && p - dest >= 1) {
			char lastc = p[-1];
			if (lastc == '/') {
				/* merge "//" */
				continue;
			}
			if (lastc == '.') {
				if (p - dest == 1) { /* should not happen */
					return WUY_HTTP_ERROR;
				}
				char last2 = p[-2];
				if (last2 == '/') {
					/* merge "/./" */
					p--;
					continue;
				}
				if (last2 == '.' && p[-3] == '/') {
					/* go up level for "/../" */
					p = memrchr(dest, '/', p - dest - 3);
					if (p == NULL) {
						return WUY_HTTP_ERROR;
					}
					continue;
				}
			}
		}

		*p++ = c;
	}

	if (p - dest >= 3 && p[-3]=='/' && p[-2]=='.' && p[-1]=='.') {
		/* go up level for "/..$" */
		p = memrchr(dest, '/', p - dest - 3);
		if (p == NULL) {
			return WUY_HTTP_ERROR;
		}
	}

	*p = '\0';
	return p - dest;
}

int wuy_http_uri(const char *uri, int len, const char **p_host,
		const char **p_path, const char **p_query, const char **p_fragment)
{
	if (len == 0) {
		return WUY_HTTP_ERROR;
	}

	/* special "*" */
	if (uri[0] == '*') {
		if (len != 1) {
			return WUY_HTTP_ERROR;
		}
		*p_path = uri;
		*p_host = NULL;
		*p_query = NULL;
		*p_fragment = NULL;
		return 1;
	}
	
	/* scheme and host */
	const char *p = uri;
	const char *end = uri + len;
	if (p[0] != '/') {
		if (memcmp(p, "http://", 7) == 0) {
			p += 7;
		} else if(memcmp(p, "https://", 8) == 0) {
			p += 8;
		} else {
			return WUY_HTTP_ERROR;
		}
		if (p >= end) {
			return WUY_HTTP_ERROR;
		}

		*p_host = p;

		const char *stop = memchr(p, '/', end - p);
		if (stop == NULL) {
			return WUY_HTTP_ERROR;
		}
		p = stop;

	} else {
		*p_host = NULL;
	}

	/* path */
	*p_path = p;
	int path_len = 0;

	/* optional query */
	const char *stop = memchr(p, '?', end - p);
	if (stop != NULL) {
		path_len = stop - p;
		*p_query = p;
		p = stop;
	} else {
		*p_query = NULL;
	}

	/* URI: optional fragment */
	stop = memchr(p, '#', end - p);
	if (stop != NULL) {
		if (path_len == 0) {
			path_len = stop - p;
		}
		*p_fragment = p;
		p = stop;
	} else {
		*p_fragment = NULL;
	}

	if (path_len == 0) {
		path_len = end - p;
	}
	return path_len;
}

int wuy_http_request_line(const char *buf, int len, enum wuy_http_method *out_method,
		const char **out_url_pos, int *out_url_len, int *out_version)
{
	const char *end = buf + len;

	/* method */
	const char *p = memchr(buf, ' ', len);
	if (p == NULL) {
		return WUY_HTTP_AGAIN;
	}
	*out_method = wuy_http_method(buf, p - buf);
	if (*out_method	== WUY_HTTP_ERROR) {
		return WUY_HTTP_ERROR;
	}

	/* url */
	const char *url = p + 1;
	if (url == end) {
		return WUY_HTTP_AGAIN;
	}
	p = memchr(url, ' ', end - url);
	if (p == NULL) {
		return WUY_HTTP_AGAIN;
	}
	*out_url_pos = url;
	*out_url_len = p - url;

	/* version */
	const char *version = p + 1;
	if (end - version < sizeof("HTTP/1.1\r\n") - 1) {
		return WUY_HTTP_AGAIN;
	}
	*out_version = wuy_http_version(version, 8);
	if (*out_version == WUY_HTTP_ERROR) {
		return WUY_HTTP_ERROR;
	}
	if (version[8] != '\r' || version[9] != '\n') {
		return WUY_HTTP_ERROR;
	}

	return version + sizeof("HTTP/1.1\r\n") - 1 - buf;
}

int wuy_http_status_line(const char *buf, int len, enum wuy_http_status_code *out_status_code,
		int *out_version)
{
	if (len < sizeof("HTTP/1.1 200\r\n") - 1) {
		return WUY_HTTP_AGAIN;
	}

	/* version */
	*out_version = wuy_http_version(buf, 8);
	if (*out_version == WUY_HTTP_ERROR) {
		return WUY_HTTP_ERROR;
	}

	const char *p = buf + 8;
	if (*p++ != ' ') {
		return WUY_HTTP_ERROR;
	}

	/* status code */
	*out_status_code = (p[0]-'0')*100 + (p[1]-'0')*10 + (p[2]-'0');
	if (*out_status_code < 200 || *out_status_code > 999) {
		return WUY_HTTP_ERROR;
	}

	/* end */
	p += 3;
	const char *end = memchr(p, '\r', buf + len - p);
	if (end == NULL) {
		return WUY_HTTP_AGAIN;
	}
	if (end[1] != '\n') {
		return WUY_HTTP_ERROR;
	}

	return end - buf + 2;

}

int wuy_http_header(const char *buf, int len, int *out_name_len,
		const char **out_value_start, int *out_value_len)
{
	/* end of headers */
	if (len < 2) {
		return WUY_HTTP_AGAIN;
	}
	if (buf[0] == '\r') {
		if (buf[1] != '\n') {
			return WUY_HTTP_ERROR;
		}
		return 2;
	}

	/* name */
	const char *name_end = memchr(buf, ':', len);
	if (name_end == NULL) {
		return WUY_HTTP_AGAIN;
	}
	*out_name_len = name_end - buf;

	/* value */
	const char *buf_end = buf + len;
	const char *value_start = name_end + 1;
	while (1) {
		if (value_start >= buf_end) {
			return WUY_HTTP_AGAIN;
		}
		if (*value_start != ' ') {
			break;
		}
		value_start++;
	}
	const char *value_end = memchr(value_start, '\r', buf_end - value_start);
	if (value_end == NULL || value_end == buf_end) {
		return WUY_HTTP_AGAIN;
	}
	if (value_end[1] != '\n') {
		return WUY_HTTP_ERROR;
	}

	*out_value_start = value_start;
	*out_value_len = value_end - value_start;

	return value_end - buf + 2;
}

enum wuy_http_chunked_state {
	WUY_HTTP_CHUNKED_DISABLE = 0,
	WUY_HTTP_CHUNKED_HEADER,
	WUY_HTTP_CHUNKED_HEADER_TAIL_LF,
	WUY_HTTP_CHUNKED_BODY,
	WUY_HTTP_CHUNKED_BODY_TAIL_CR,
	WUY_HTTP_CHUNKED_BODY_TAIL_LF,
	WUY_HTTP_CHUNKED_FINISH,
};

static int wuy_http_chunked_size(wuy_http_chunked_t *chunked, const char *buf, int len)
{
	const char *end = buf + len;
	const char *p = buf;

	uint32_t n = chunked->size;
	while (p < end) {
		if (n > 0xFFFFFFF) {
			return -1;
		}

		char c = *p++;
		if (c >= '0' && c <= '9') {
			n = (n << 4) + c - '0';
		} else if (c >= 'a' && c <= 'f') {
			n = (n << 4) + c - 'a' + 10;
		} else if (c >= 'A' && c <= 'F') {
			n = (n << 4) + c - 'A' + 10;
		} else if (c == '\r') {
			chunked->state = WUY_HTTP_CHUNKED_HEADER_TAIL_LF;
			break;
		} else {
			return -2;
		}
	}

	chunked->size = n;
	return p - buf;
}

#define MIN3(a, b, c) (a)<(b) ? ((a)<(c)?(a):(c)) : ((b)<(c)?(b):(c))
int wuy_http_chunked_process(wuy_http_chunked_t *chunked, const uint8_t *in_buf,
		int in_len, uint8_t *out_buf, int *p_out_len)
{
	const uint8_t *in_pos = in_buf;
	uint8_t *out_pos = out_buf;
	int out_len = *p_out_len;

	while (in_len > 0 && out_len > 0) {
		int proc_len;
		switch (chunked->state) {
		case WUY_HTTP_CHUNKED_DISABLE:
			return -3;
		case WUY_HTTP_CHUNKED_HEADER:
			proc_len = wuy_http_chunked_size(chunked, (const char *)in_pos, in_len);
			if (proc_len < 0) {
				return -4;
			}
			break;
		case WUY_HTTP_CHUNKED_HEADER_TAIL_LF:
			if (in_pos[0] != '\n') {
				return -5;
			}
			proc_len = 1;
			chunked->state = (chunked->size != 0) ? WUY_HTTP_CHUNKED_BODY : WUY_HTTP_CHUNKED_FINISH;
			break;
		case WUY_HTTP_CHUNKED_BODY:
			proc_len = MIN3(chunked->size, in_len, out_len);
			memcpy(out_pos, in_pos, proc_len);
			out_pos += proc_len;
			out_len -= proc_len;
			chunked->size -= proc_len;
			if (chunked->size == 0) {
				chunked->state = WUY_HTTP_CHUNKED_BODY_TAIL_CR;
			}
			break;
		case WUY_HTTP_CHUNKED_BODY_TAIL_CR:
			if (in_pos[0] != '\r') {
				return -6;
			}
			proc_len = 1;
			chunked->state = WUY_HTTP_CHUNKED_BODY_TAIL_LF;
			break;
		case WUY_HTTP_CHUNKED_BODY_TAIL_LF:
			if (in_pos[0] != '\n') {
				return -7;
			}
			proc_len = 1;
			chunked->state = WUY_HTTP_CHUNKED_HEADER;
			break;
		case WUY_HTTP_CHUNKED_FINISH:
			if (in_pos[0] != '\r' && in_pos[0] != '\n') {
				goto out;
			}
			proc_len = 1;
			break;
		default:
			return -8;
		}

		in_pos += proc_len;
		in_len -= proc_len;
	}

out:
	*p_out_len = out_pos - out_buf;
	return in_pos - in_buf;
}

void wuy_http_chunked_init(wuy_http_chunked_t *chunked)
{
	chunked->state = WUY_HTTP_CHUNKED_DISABLE;
	chunked->size = 0;
}
void wuy_http_chunked_enable(wuy_http_chunked_t *chunked)
{
	chunked->state = WUY_HTTP_CHUNKED_HEADER;
}
bool wuy_http_chunked_is_enabled(const wuy_http_chunked_t *chunked)
{
	return chunked->state != WUY_HTTP_CHUNKED_DISABLE;
}
bool wuy_http_chunked_is_finished(const wuy_http_chunked_t *chunked)
{
	return chunked->state == WUY_HTTP_CHUNKED_FINISH;
}

#define WUY_HTTP_DATE_FORMAT "%a, %d %b %Y %H:%M:%S GMT"
time_t wuy_http_date_parse(const char *str)
{
	struct tm tm;
	char *end = strptime(str, WUY_HTTP_DATE_FORMAT, &tm);
	if (*end != '\0') {
		return -1;
	}
	return timegm(&tm);
}
const char *wuy_http_date_make(time_t ts)
{
	static time_t last = 0;
	static char cache[WUY_HTTP_DATE_LENGTH + 1];
	if (ts != last) {
		strftime(cache, sizeof(cache), WUY_HTTP_DATE_FORMAT, gmtime(&ts));
	}
	return cache;
}
