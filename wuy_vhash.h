#ifndef WUY_VHASH_H
#define WUY_VHASH_H

#include <stdint.h>

#ifdef WUY_VHASH_OTHERS
#define wuy_vhash128 xxx

#else /* use murmurhash as default */

#include "wuy_murmurhash.h"
#define wuy_vhash128 wuy_murmurhash

#endif

static inline uint64_t wuy_vhash64(const void *data, int len)
{
	union {
		uint8_t out[16];
		uint64_t ret[2];
	} u;
	wuy_vhash128(data, len, u.out);
	return u.ret[0] ^ u.ret[1];
}

#endif
