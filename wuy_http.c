#include <string.h>

#include "wuy_http.h"

#define WUY_HTTP_ERROR -1
#define WUY_HTTP_AGAIN 0

int wuy_http_method(const char *buf, int len)
{
	switch (len) {
	case 3:
		if (memcmp(buf, "GET", 3) == 0) {
			return WUY_HTTP_METHOD_GET;
		}
		if (memcmp(buf, "PUT", 3) == 0) {
			return WUY_HTTP_METHOD_PUT;
		}
		return WUY_HTTP_ERROR;
	case 4:
		if (memcmp(buf, "POST", 4) == 0) {
			return WUY_HTTP_METHOD_POST;
		}
		if (memcmp(buf, "HEAD", 4) == 0) {
			return WUY_HTTP_METHOD_HEAD;
		}
		return WUY_HTTP_ERROR;
	case 6:
		if (memcmp(buf, "DELETE", 6) == 0) {
			return WUY_HTTP_METHOD_DELETE;
		}
		return WUY_HTTP_ERROR;
	case 7:
		if (memcmp(buf, "OPTIONS", 7) == 0) {
			return WUY_HTTP_METHOD_OPTIONS;
		}
		if (memcmp(buf, "CONNECT", 7) == 0) {
			return WUY_HTTP_METHOD_CONNECT;
		}
		return WUY_HTTP_ERROR;
	default:
		return WUY_HTTP_ERROR;
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

int wuy_http_request_line(const char *buf, int len, int *out_method,
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

int wuy_http_status_line(const char *buf, int len, int *out_status_code,
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
			goto out;
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
