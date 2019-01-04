#ifndef WUY_EVENT_H
#define WUY_EVENT_H

#include <stdbool.h>

typedef struct wuy_event_ctx_s wuy_event_ctx_t;

typedef struct {
	unsigned set_read:1;
	unsigned set_write:1;
} wuy_event_status_t;

wuy_event_ctx_t *wuy_event_ctx_new(void (*handler)(void *, bool, bool));

void wuy_event_run(wuy_event_ctx_t *ctx, int timeout_ms);

int wuy_event_add_read(wuy_event_ctx_t *ctx, int fd, void *data,
		wuy_event_status_t *status);
int wuy_event_add_write(wuy_event_ctx_t *ctx, int fd, void *data,
		wuy_event_status_t *status);
int wuy_event_del_read(wuy_event_ctx_t *ctx, int fd, void *data,
		wuy_event_status_t *status);
int wuy_event_del_write(wuy_event_ctx_t *ctx, int fd, void *data,
		wuy_event_status_t *status);
int wuy_event_del(wuy_event_ctx_t *ctx, int fd,
		wuy_event_status_t *status);

#endif
