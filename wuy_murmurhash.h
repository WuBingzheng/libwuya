#ifndef WUY_MURMURHASH
#define WUY_MURMURHASH

void wuy_murmurhash(const void *key, int len, void *out);

static inline uint64_t wuy_murmurhash_id(const void *key, int len)
{
	union {
		uint8_t out[16];
		uint64_t ret[2];
	} u;
	wuy_murmurhash(key, len, u.out);
	return u.ret[0] ^ u.ret[1];
}

#endif
