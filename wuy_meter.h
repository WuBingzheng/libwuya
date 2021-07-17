/* Token bucket rate limit */

#ifndef WUY_METER_H
#define WUY_METER_H

#include <stdbool.h>
#include <sys/time.h>

/* configration */
struct wuy_meter_conf {
	float	rate; /* limit rate per second */
	float	burst; /* burst per second, must >=rate */
};

/* meter for each entry, set zero to init */
struct wuy_meter_node {
	double	reset;
	float	tokens;
};

static inline double wuy_meter_now(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
}

static inline bool wuy_meter_check(const struct wuy_meter_conf *conf,
		struct wuy_meter_node *m, float cost)
{
	/* add tokens */
	double now = wuy_meter_now();
	float tokens = (now - m->reset) * conf->rate;
	m->reset = now;
	m->tokens += tokens;
	if (m->tokens > conf->burst) {
		m->tokens = conf->burst;
	}

	/* consume tokens */
	m->tokens -= cost;

	return m->tokens >= 0;
}

#endif
