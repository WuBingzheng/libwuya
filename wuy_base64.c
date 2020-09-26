int wuy_base64_encode(char *dest, const unsigned char *src, int len)
{
	static const char *table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	char *out = dest;
	const unsigned char *in, *end = src + len;
	for (in = src; end - in >= 3; in += 3) {
		*out++ = table[in[0] >> 2];
		*out++ = table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
		*out++ = table[((in[1] & 0x0f) << 2) | (in[2] >> 6)];
		*out++ = table[in[2] & 0x3f];
	}

	if (end - in == 1) {
		*out++ = table[in[0] >> 2];
		*out++ = table[(in[0] & 0x03) << 4];
		*out++ = '=';
		*out++ = '=';
	} else if (end - in == 2) {
		*out++ = table[in[0] >> 2];
		*out++ = table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
		*out++ = table[(in[1] & 0x0f) << 2];
		*out++ = '=';
	}

	return out - dest;
}

int wuy_base64_decode(unsigned char *dest, const char *src, int len)
{
	static unsigned table[256] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 62, 63, 62, 62, 63, 52, 53, 54, 55,
		56, 57, 58, 59, 60, 61, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6,
		7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 0,
		0, 0, 0, 63, 0, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
		41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51 };

	if (len == 0) {
		return 0;
	}
	if ((len % 4) != 0) {
		return -1;
	}

	unsigned char *out = dest;
	const unsigned char *usrc = (const unsigned char *)src;
	for (const unsigned char *in = usrc; in < usrc + len; in += 4) {
		unsigned n = table[in[0]] << 18 | table[in[1]] << 12 | table[in[2]] << 6 | table[in[3]];
		*out++ = n >> 16;
		*out++ = n >> 8 & 0xFF;
		*out++ = n & 0xFF;
	}
	if (src[len - 1] == '=') {
		out--;
		if (src[len - 2] == '=') {
			out--;
		}
	}
	return out - dest;
}
