#ifndef WUY_TIME_H
#define WUY_TIME_H

#define WUY_TIME_SIZE_PREC_S	(sizeof("2018-09-19T16:53:50+0800") - 1)
#define WUY_TIME_SIZE_PREC_MS	(sizeof("2018-09-19T16:53:50.123+0800") - 1)
#define WUY_TIME_SIZE_PREC_US	(sizeof("2018-09-19T16:53:50.123456+0800") - 1)

enum wuy_time_precision {
	WUY_TIME_PRECISION_S,
	WUY_TIME_PRECISION_MS,
	WUY_TIME_PRECISION_US,
};

int wuy_time_rfc3339(char *buffer, enum wuy_time_precision precision);

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

static inline long wuy_time_usec(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_usec;
}

#endif
