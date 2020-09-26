#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "wuy_time.h"

int wuy_time_rfc3339(char *buffer, enum wuy_time_precision precision)
{
/* TODO need to optimize? */

#define WUY_TIME_SIZE_SEC	(sizeof("2018-09-19T16:53:50") - 1)
#define WUY_TIME_SIZE_TZ	(sizeof("+0800") - 1)
        static char buf_sec[WUY_TIME_SIZE_SEC + 1];
        static char buf_tz[WUY_TIME_SIZE_TZ + 1];

        static struct timeval last;

        struct timeval now;
        gettimeofday(&now, NULL);

        if (now.tv_sec != last.tv_sec) {
                struct tm *tm = localtime(&now.tv_sec); // TODO replace localtime() because it do syscall
                sprintf(buf_sec, "%04d-%02d-%02dT%02d:%02d:%02d",
                                tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                                tm->tm_hour, tm->tm_min, tm->tm_sec);
        }

        if (last.tv_sec == 0) {
		/* timezone */
                time_t tmp = time(NULL);
                struct tm *tm = localtime(&tmp);

                int off_sign = '+';
                int off = (int) tm->tm_gmtoff;
                if (tm->tm_gmtoff < 0) {
                        off_sign = '-';
                        off = -off;
                }

                off /= 60; /* second to minute */
                sprintf(buf_tz, "%c%02d%02d", off_sign, off / 60, off % 60);
        }

        last = now;

	/* build output string */
	char *p = buffer;

	memcpy(p, buf_sec, WUY_TIME_SIZE_SEC);
	p += WUY_TIME_SIZE_SEC;

	switch(precision) {
	case WUY_TIME_PRECISION_S:
		break;
	case WUY_TIME_PRECISION_MS:
		p += sprintf(p, ".%03ld", now.tv_usec / 1000);
		break;
	case WUY_TIME_PRECISION_US:
		p += sprintf(p, ".%06ld", now.tv_usec);
		break;
	default:
		abort();
	}

	memcpy(p, buf_tz, WUY_TIME_SIZE_TZ);
	p += WUY_TIME_SIZE_TZ;

	*p = '\0';
        return p - buffer;
}
