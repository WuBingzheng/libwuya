#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>

#include "wuy_event.h"

struct wuy_event_ctx_s {
	int fd;
	void (*handler)(void *, bool, bool);
};

wuy_event_ctx_t *wuy_event_ctx_new(void (*handler)(void *, bool, bool))
{
	wuy_event_ctx_t *ctx = malloc(sizeof(wuy_event_ctx_t));
	assert(ctx != NULL);

	ctx->fd = epoll_create(1);
	assert(ctx->fd >= 0);

	ctx->handler = handler;
	return ctx;
}

void wuy_event_run(wuy_event_ctx_t *ctx, int timeout_ms)
{
	struct epoll_event events[100];
	int n = epoll_wait(ctx->fd, events, 100, timeout_ms);

	int i;
	for (i = 0; i < n; i++) {
		struct epoll_event *ev = &events[i];
		ctx->handler(ev->data.ptr, ev->events & ~EPOLLOUT,
				ev->events & EPOLLOUT);
	}
}

static int wuy_event_op(int epfd, int fd, int op, uint32_t event, void *data)
{
	struct epoll_event ev;
	ev.events = event | EPOLLET;
	ev.data.ptr = data;
	return epoll_ctl(epfd, op, fd, &ev);
}
static int wuy_event_do_add_read(int epfd, int fd, void *data)
{
	return wuy_event_op(epfd, fd, EPOLL_CTL_ADD, EPOLLIN, data);
}
static int wuy_event_do_add_write(int epfd, int fd, void *data)
{
	return wuy_event_op(epfd, fd, EPOLL_CTL_ADD, EPOLLOUT, data);
}
static int wuy_event_do_mod_read(int epfd, int fd, void *data)
{
	return wuy_event_op(epfd, fd, EPOLL_CTL_MOD, EPOLLIN, data);
}
static int wuy_event_do_mod_write(int epfd, int fd, void *data)
{
	return wuy_event_op(epfd, fd, EPOLL_CTL_MOD, EPOLLOUT, data);
}
static int wuy_event_do_mod_rdwr(int epfd, int fd, void *data)
{
	return wuy_event_op(epfd, fd, EPOLL_CTL_MOD, EPOLLIN | EPOLLOUT, data);
}
static int wuy_event_do_del(int epfd, int fd)
{
	return wuy_event_op(epfd, fd, EPOLL_CTL_DEL, 0, NULL);
}

int wuy_event_add_read(wuy_event_ctx_t *ctx, int fd, void *data,
		wuy_event_status_t *status)
{
	if (status->set_read) {
		return 0;
	}

	status->set_read = 1;
	if (status->set_write) {
		return wuy_event_do_mod_rdwr(ctx->fd, fd, data);
	} else {
		return wuy_event_do_add_read(ctx->fd, fd, data);
	}
}

int wuy_event_add_write(wuy_event_ctx_t *ctx, int fd, void *data,
		wuy_event_status_t *status)
{
	if (status->set_write) {
		return 0;
	}

	status->set_write = 1;
	if (status->set_read) {
		return wuy_event_do_mod_rdwr(ctx->fd, fd, data);
	} else {
		return wuy_event_do_add_write(ctx->fd, fd, data);
	}
}

int wuy_event_del_read(wuy_event_ctx_t *ctx, int fd, void *data,
		wuy_event_status_t *status)
{
	if (!status->set_read) {
		return 0;
	}

	status->set_read = 0;
	if (status->set_write) {
		return wuy_event_do_mod_write(ctx->fd, fd, data);
	} else {
		return wuy_event_do_del(ctx->fd, fd);
	}
}

int wuy_event_del_write(wuy_event_ctx_t *ctx, int fd, void *data,
		wuy_event_status_t *status)
{
	if (!status->set_write) {
		return 0;
	}

	status->set_write = 0;
	if (status->set_read) {
		return wuy_event_do_mod_read(ctx->fd, fd, data);
	} else {
		return wuy_event_do_del(ctx->fd, fd);
	}
}

int wuy_event_del(wuy_event_ctx_t *ctx, int fd,
		wuy_event_status_t *status)
{
	if (!status->set_write && !status->set_read) {
		return 0;
	}
	status->set_read = 0;
	status->set_write = 0;
	return wuy_event_do_del(ctx->fd, fd);
}

