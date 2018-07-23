#ifndef WUY_TIMER_H
#define WUY_TIMER_H

#include <stdint.h>
#include <stdbool.h>
#include <sys/time.h>

#include "wuy_heap.h"
#include "wuy_container.h"

typedef wuy_heap_t wuy_timer_ctx_t;
typedef struct wuy_timer_s wuy_timer_t;

typedef void wuy_timer_handler_f(wuy_timer_t *);

struct wuy_timer_s {
	int64_t			expire;
	wuy_timer_handler_f	*handler;
	wuy_heap_node_t		heap_node;
};

static inline wuy_timer_ctx_t *wuy_timer_ctx_new(void)
{
	return wuy_heap_new_type(offsetof(wuy_timer_t, heap_node),
			WUY_HEAP_KEY_INT64, offsetof(wuy_timer_t, expire), false);
}

static inline int64_t __now_ms(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000 + tv.tv_usec / 1000000;
}

static inline bool wuy_timer_add(wuy_timer_ctx_t *ctx, wuy_timer_t *timer,
		int64_t timeout_ms, wuy_timer_handler_f *handler)
{
	timer->expire = __now_ms() + timeout_ms;
	timer->handler = handler;
	return wuy_heap_push_or_fix(ctx, timer);
}

static inline void wuy_timer_delete(wuy_timer_ctx_t *ctx, wuy_timer_t *timer)
{
	wuy_heap_delete(ctx, timer);
}

static inline int64_t wuy_timer_expire(wuy_timer_ctx_t *ctx)
{
	int64_t now_ms = __now_ms();
	while (1) {
		wuy_timer_t *timer = wuy_heap_min(ctx);
		if (timer == NULL) {
			return -1;
		}
		if (timer->expire > now_ms) {
			return timer->expire - now_ms;
		}
		wuy_heap_delete(ctx, timer);
		timer->handler(timer);
	}
}

#endif
