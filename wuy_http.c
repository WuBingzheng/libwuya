#define _XOPEN_SOURCE
#define _DEFAULT_SOURCE
#define _GNU_SOURCE // memrchr()
#include <stdio.h>
#include <time.h>
#include <ctype.h>
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

int wuy_http_decode_query_value(char *dest, const char *src, int len)
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
		if (c == '+') {
			c = ' ';
		} else if (c == '%') {
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
		*p++ = c;
	}
	*p = '\0';
	return p - dest;
}

int wuy_http_encode_query(const char *key_str, int key_len,
		const char *value_str, int value_len,
		char *out_buffer, int out_buf_size)
{
	char *p = out_buffer;
	char *out_buf_end = out_buffer + out_buf_size;

	p += snprintf(p, out_buf_size, "&%.*s=", key_len, key_str);

	for (int i = 0; i < value_len; i++) {
		if (p >= out_buf_end) {
			break;
		}

		char c = value_str[i];
		if (isalnum(c) || c=='-' || c=='.' || c=='_' || c=='~') {
			*p++ = c;
		} else if (c == ' ') {
			*p++ = '+';
		} else {
			*p++ = '%';
			int high = (unsigned char)c >> 4;
			int low = c & 0xF;
			*p++ = high < 10 ? high + '0' : high + 'A' - 10;
			*p++ = low < 10 ? low + '0' : low + 'A' - 10;
		}
	}

	return p - out_buffer;
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
		p = stop;
		*p_query = p;
	} else {
		*p_query = NULL;
	}

	/* URI: optional fragment */
	stop = memchr(p, '#', end - p);
	if (stop != NULL) {
		if (path_len == 0) {
			path_len = stop - p;
		}
		p = stop;
		*p_fragment = p;
	} else {
		*p_fragment = NULL;
	}

	if (path_len == 0) {
		path_len = end - p;
	}
	return path_len;
}

int wuy_http_uri_query_get(const char *query_str, int query_len,
		const char *key_str, int key_len, char *value_buf)
{
	if (query_str == NULL) {
		return -1;
	}

	const char *p = query_str;
	const char *end = query_str + query_len;
	if (p[0] == '?') {
		p++;
	}

	while (end - p > key_len) {
		if (memcmp(p, key_str, key_len) == 0 && p[key_len] == '=') {
			p += key_len + 1;
			const char *next = memchr(p, '&', end - p);
			int value_len = (next ? next : end) - p;
			return wuy_http_decode_query_value(value_buf, p, value_len);
		}
		p = memchr(p, '&', end - p);
		if (p == NULL) {
			break;
		}
		p++;
	}
	return -1;
}

int wuy_http_uri_query_first(const char *query_str, int query_len,
		int *p_key_len, char *value_buf, int *p_value_len)
{
	const char *p = query_str;
	const char *end = query_str + query_len;
	if (p[0] != '?' && p[0] != '&') {
		return -1;
	}
	p++;

	const char *next = memchr(p, '&', end - p);
	if (next != NULL) {
		end = next;
	}

	const char *value_pos = memchr(p, '=', end - p);
	if (value_pos == NULL) {
		*p_key_len = end - p;
		*p_value_len = 0;
		return end - query_str;
	}

	*p_key_len = value_pos - p;
	*p_value_len = wuy_http_decode_query_value(value_buf, value_pos + 1, end - value_pos);
	return end - query_str;
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

static int wuy_http_chunked_size(wuy_http_chunked_t *chunked, const void *buf, int len)
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
	return p - (const char *)buf;
}

int wuy_http_chunked_decode(wuy_http_chunked_t *chunked, const uint8_t **p_buffer,
		const uint8_t *buf_end)
{
	const uint8_t *p = *p_buffer;

	while (p < buf_end) {
		int proc_len;
		switch (chunked->state) {
		case WUY_HTTP_CHUNKED_DISABLE:
			return -3;
		case WUY_HTTP_CHUNKED_HEADER:
			proc_len = wuy_http_chunked_size(chunked, p, buf_end - p);
			if (proc_len < 0) {
				return -4;
			}
			p += proc_len;
			break;
		case WUY_HTTP_CHUNKED_HEADER_TAIL_LF:
			if (p[0] != '\n') {
				return -5;
			}
			p += 1;
			chunked->state = (chunked->size != 0) ? WUY_HTTP_CHUNKED_BODY : WUY_HTTP_CHUNKED_FINISH;
			break;
		case WUY_HTTP_CHUNKED_BODY:
			if (chunked->size <= buf_end - p) {
				buf_end = p + chunked->size;
				chunked->state = WUY_HTTP_CHUNKED_BODY_TAIL_CR;
			}
			chunked->size -= buf_end - p;
			*p_buffer = p;
			return buf_end - p;
		case WUY_HTTP_CHUNKED_BODY_TAIL_CR:
			if (p[0] != '\r') {
				return -6;
			}
			p += 1;
			chunked->state = WUY_HTTP_CHUNKED_BODY_TAIL_LF;
			break;
		case WUY_HTTP_CHUNKED_BODY_TAIL_LF:
			if (p[0] != '\n') {
				return -7;
			}
			p += 1;
			chunked->state = WUY_HTTP_CHUNKED_HEADER;
			break;
		case WUY_HTTP_CHUNKED_FINISH:
			if (p[0] != '\r' && p[0] != '\n') {
				return -8;
			}
			p += 1;
			break;
		default:
			return -9;
		}
	}

	*p_buffer = p;
	return 0;
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

static off_t wuy_http_range_num(const char *pos, const char *end, const char **stop)
{
	off_t n = 0;
	while (pos < end) {
		char c = *pos;
		if (c < '0' || c > '9') {
			break;
		}
		n = n * 10 + c - '0';
		pos++;
	}
	*stop = pos;
	return n;
}

/* For multiple ranges, according to RFC7233, overlap ranges are allowed,
 * and unsatisfiable ranges are ignored if there is at least one satisfiable
 * range. However we return fail for both case. */
int wuy_http_range_parse(const char *value_str, int value_len, off_t total_size,
		struct wuy_http_range *ranges, int range_num)
{
	if (value_len <= 6 || memcmp(value_str, "bytes=", 6) != 0) {
		return 0;
	}

	const char *stop, *p = value_str + 6;
	const char *value_end = value_str + value_len;
	struct wuy_http_range *range = ranges;

	while (p < value_end) {
		if (range - ranges > range_num) {
			return -1;
		}
		while (p < value_end && *p == ' ') {
			p++;
		}

		/* suffix-byte-range-spec */
		if (*p == '-') {
			p++;
			off_t n = wuy_http_range_num(p, value_end, &stop);
			if (stop == p) {
				return -1;
			}
			if (stop < value_end && *stop != ',') {
				return -1;
			}
			range->first = (n < total_size) ? total_size - n : 0;
			range->last = total_size - 1;

			range++;
			p = stop + 1;
			continue;
		}

		/* byte-range-spec: first-byte-pos */
		range->first = wuy_http_range_num(p, value_end, &stop);
		if (stop == p) {
			return -1;
		}
		if (stop == value_end || *stop != '-') {
			return -1;
		}
		if (range->first >= total_size) {
			return -1;
		}
		if (range > ranges && range->first <= (range-1)->last) {
			return -1;
		}
		p = stop + 1;

		/* byte-range-spec: last-byte-pos */
		if (p == value_end || *p == ',') {
			range->last = total_size - 1;
			p++;
			continue;
		}

		range->last = wuy_http_range_num(p, value_end, &stop);
		if (stop == p) {
			return -1;
		}
		if (stop < value_end && *stop != ',') {
			return -1;
		}
		if (range->last < range->first) {
			return -1;
		}
		if (range->last >= total_size) {
			range->last = total_size - 1;
		}
		p = stop + 1;
		range++;
	}

	return range - ranges;
}

/* TODO escape */
const char *wuy_http_cookie_get(const char *cookie, int *p_cookie_len,
		const char *name, int name_len)
{
	int cookie_len = *p_cookie_len;
	while (1) {
		if (memcmp(cookie, name, name_len) == 0 && cookie[name_len] == '=') {
			break; /* find */
		}

		const char *end = memchr(cookie, ';', cookie_len);
		if (end == NULL) {
			return NULL; /* fail */
		}
		end++;
		while (*end == ' ') end++;
		cookie_len -= end - cookie;
		cookie = end;
	}

	/* find */
	cookie += name_len + 1;
	cookie_len -= name_len + 1;
	const char *end = memchr(cookie, ';', cookie_len);
	if (end != NULL) {
		cookie_len = end - cookie;
	}
	*p_cookie_len = cookie_len;
	return cookie;
}
