#ifndef WUY_METER_H
#define WUY_METER_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/time.h>

struct wuy_meter_conf {
	int		rate; /* limit rate per second */
	int		burst; /* burst per second, must >=rate */
	int		punish_sec; /* punish time if excessing the burst limit. 0 for no punish */
};

struct wuy_meter_node {
	uint32_t	begin_cs;
	uint32_t	count;
};

static inline uint32_t wuy_meter_now_centisec(void)
{
	/* uint32_t overflows but it's OK, because we only use the diff of time */
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (uint64_t)(tv.tv_sec * 100 + tv.tv_usec / 10000);
}

static inline void wuy_meter_init2(struct wuy_meter_node *m, uint32_t now_cs)
{
	m->begin_cs = now_cs;
	m->count = 1;
}
static inline void wuy_meter_init(struct wuy_meter_node *m)
{
	wuy_meter_init2(m, wuy_meter_now_centisec());
}

static inline bool wuy_meter_in_punishing(const struct wuy_meter_conf *conf,
		struct wuy_meter_node *m, uint32_t last_cs)
{
	return m->count > conf->burst && conf->punish_sec != 0 && last_cs/100 < conf->punish_sec;
}

static inline bool wuy_meter_check(const struct wuy_meter_conf *conf,
		struct wuy_meter_node *m)
{
	uint32_t now_cs = wuy_meter_now_centisec();
	uint32_t last_cs = now_cs - m->begin_cs;

	/* if (last_cs / 100) < (conf->burst / conf->rate) */
	if (last_cs * conf->rate < conf->burst * 100) {
		if (m->count * 100 < conf->rate * last_cs) {
			/* restart the meter */
			wuy_meter_init2(m, now_cs);
			return true;
		} else {
			m->count++;
			return m->count <= conf->burst;
		}
	}

	if (wuy_meter_in_punishing(conf, m, last_cs)) {
		/* in punishing */
		m->count++;
		return false;
	}
 
	/* restart the meter */
	wuy_meter_init2(m, now_cs);
	return true;
}

static inline bool wuy_meter_is_expired(const struct wuy_meter_conf *conf,
		struct wuy_meter_node *m)
{
	uint32_t last_cs = wuy_meter_now_centisec() - m->begin_cs;
	if (last_cs * conf->rate < conf->burst * 100) {
		return false;
	}
	return !wuy_meter_in_punishing(conf, m, last_cs);
}

#endif
