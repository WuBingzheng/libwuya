#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "wuy_time.h"

static int wuy_time_local_gmtoff(void)
{
	static int offset = -1;
	if (offset == -1) {
		time_t local = time(NULL);
		struct tm *tm = localtime(&local);
		offset = timegm(tm) - local;
	}
	return offset;
}

int wuy_time_rfc3339(char *buffer, int gmt_offset)
{
#define WUY_TIME_SIZE_SEC	(sizeof("2018-09-19T16:53:50") - 1)
	static char buf_sec[WUY_TIME_SIZE_SEC + 20]; /* +20 to avoid GCC warning */
	static time_t last_sec;

	struct timeval now;
	gettimeofday(&now, NULL);

	/* offset for timezone */
	if (gmt_offset == WUY_TIME_ZONE_LOCAL) {
		gmt_offset = wuy_time_local_gmtoff();
	}
	now.tv_sec += gmt_offset;

	/* date */
	if (now.tv_sec != last_sec) {
		last_sec = now.tv_sec;
		struct tm *tm = gmtime(&now.tv_sec);
		sprintf(buf_sec, "%04d-%02d-%02dT%02d:%02d:%02d",
				tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
				tm->tm_hour, tm->tm_min, tm->tm_sec);
	}

	char *p = buffer;
	memcpy(p, buf_sec, WUY_TIME_SIZE_SEC);
	p += WUY_TIME_SIZE_SEC;

	/* micro second */
	*p++ = '.';
	suseconds_t usec = now.tv_usec;
	for (int i = 0; i < 6; i++) {
		p[5 - i] = (usec % 10) + '0';
		usec /= 10;
	}
	p += 6;

	/* timezone */
	if (gmt_offset >= 0) {
		*p++ = '+';
	} else {
		*p++ = '-';
		gmt_offset = -gmt_offset;
	}
	gmt_offset /= 60;
	int hour = gmt_offset / 60;
	int minute = gmt_offset % 60;
	*p++ = hour / 10 + '0';
	*p++ = hour % 10 + '0';
	*p++ = minute / 10 + '0';
	*p++ = minute % 10 + '0';

	*p = '\0';
	return p - buffer;
}
