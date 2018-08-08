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
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>  
#include <netinet/tcp.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>


static inline int tcp_set_sndbuf(int fd, int value)
{
	return setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &value, sizeof(int));
}
static inline int tcp_get_sndbuf(int fd)
{
	int value, size = 4;
	getsockopt(fd, SOL_SOCKET, SO_SNDBUF, &value, &size);
	return value;
}
static inline int tcp_get_rcvbuf(int fd)
{
	int value, size = 4;
	getsockopt(fd, SOL_SOCKET, SO_RCVBUF, &value, &size);
	return value;
}

static inline int tcp_set_rcvbuf(int fd, int value)
{
	return setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &value, sizeof(int));
}

static inline int tcp_set_cork(int fd, int cork)
{
	return setsockopt(fd, IPPROTO_TCP, TCP_CORK, &cork, sizeof(int));
}

static inline int wuy_tcp_listen(struct sockaddr *sa, int x)
{
	int fd = socket(sa->sa_family, SOCK_STREAM, 0);  
	if (fd < 0) {
		return -1;
	}

	if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK) != 0) {
		return -1;
	}

	int val = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int)) < 0) {
		return -1;
	}
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &val, sizeof(int)) < 0) {
		return -1;
	}

	val = 60;
	if (setsockopt(fd, IPPROTO_TCP, TCP_DEFER_ACCEPT, &val, sizeof(int)) < 0) {
		return -1;
	}

	if (bind(fd, sa, sizeof(struct sockaddr)) < 0) {
		close(fd);
		return -1;
	}

	listen(fd, 1000);
	return fd;
}


static inline int wuy_tcp_accept(int listen_fd, struct sockaddr *client_addr)
{
	int client_fd;
	socklen_t addrlen = sizeof(struct sockaddr);
again:
	// client_fd = accept4(listen_fd, client_addr, &addrlen, SOCK_NONBLOCK);
	client_fd = accept(listen_fd, client_addr, &addrlen);
	if (client_fd < 0) {
		if (errno == EINTR)
			goto again;
		return client_fd;
	}
	int ret = fcntl(client_fd, F_SETFL, fcntl(client_fd, F_GETFL) | O_NONBLOCK); // XXX
	printf("set nonblock %d\n", ret);
	return client_fd;
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
		printf("close 4\n");
		return -1;
	}

	return fd;
}
