#ifndef WUY_TIME_H
#define WUY_TIME_H

#define WUY_TIME_BUFFER_SIZE	(sizeof("2018-09-19T16:53:50.123456+0800") - 1)

#define WUY_TIME_ZONE_LOCAL -10000  /* for gmt_offset parameter */

int wuy_time_rfc3339(char *buffer, int gmt_offset);

#include <sys/time.h>
static inline long wuy_time_us(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000000 + tv.tv_usec;
}

static inline long wuy_time_ms(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

#endif
