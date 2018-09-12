/*
 * Utils for TCP socket.
 *
 * Author: Wu Bingzheng
 *
 */

#define __USE_GNU
#define _GNU_SOURCE
#include <sys/socket.h>

#include <sys/types.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>  
#include <netinet/tcp.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>


static inline int wuy_tcp_set_sndbuf(int fd, int value)
{
	return setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &value, sizeof(int));
}
static inline int wuy_tcp_get_sndbuf(int fd)
{
	int value;
	socklen_t size = sizeof(int);
	getsockopt(fd, SOL_SOCKET, SO_SNDBUF, &value, &size);
	return value;
}
static inline int wuy_tcp_set_rcvbuf(int fd, int value)
{
	return setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &value, sizeof(int));
}
static inline int wuy_tcp_get_rcvbuf(int fd)
{
	int value;
	socklen_t size = sizeof(int);
	getsockopt(fd, SOL_SOCKET, SO_RCVBUF, &value, &size);
	return value;
}
static inline int wuy_tcp_set_cork(int fd, int cork)
{
	return setsockopt(fd, IPPROTO_TCP, TCP_CORK, &cork, sizeof(int));
}
static inline int wuy_tcp_set_defer_accept(int fd, int value)
{
	return setsockopt(fd, IPPROTO_TCP, TCP_DEFER_ACCEPT, &value, sizeof(int));
}

static inline int wuy_tcp_listen(struct sockaddr *sa, int backlog, bool reuse_port)
{
	int fd = socket(sa->sa_family, SOCK_STREAM, 0);  
	if (fd < 0) {
		return -1;
	}

	if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK) != 0) {
		close(fd);
		return -1;
	}

	int value = 1;
	int opt = reuse_port ? SO_REUSEPORT : SO_REUSEADDR;
	if (setsockopt(fd, SOL_SOCKET, opt, &value, sizeof(int)) < 0) {
		close(fd);
		return -1;
	}

	if (bind(fd, sa, sizeof(struct sockaddr)) < 0) {
		close(fd);
		return -1;
	}

	listen(fd, backlog);
	return fd;
}


static inline int wuy_tcp_accept(int listen_fd, struct sockaddr *client_addr)
{
	socklen_t addrlen = sizeof(struct sockaddr);
	return accept4(listen_fd, client_addr, &addrlen, SOCK_NONBLOCK);
}

static inline int wuy_tcp_connect(struct sockaddr *sa)
{
	int fd = socket(sa->sa_family, SOCK_STREAM, 0);
	if (fd < 0) {
		return -1;
	}

	if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK) != 0) { // TODO 1-syscall
		close(fd);
		return -1;
	}

	errno = 0;
	int ret = connect(fd, sa, sizeof(struct sockaddr));
	if (ret < 0 && errno != EINPROGRESS) {
		close(fd);
		return -1;
	}

	return fd;
}
