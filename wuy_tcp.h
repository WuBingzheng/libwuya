/**
 * @file     wuy_sockaddr.h
 * @author   Wu Bingzheng <wubingzheng@gmail.com>
 * @date     2018-7-19
 *
 * @section LICENSE
 * GPLv2
 *
 * @section DESCRIPTION
 *
 * Some TCP connection utils in non-blocking mode.
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

#include "wuy_sockaddr.h"


/**
 * @brief Set sndbuf.
 */
static inline int wuy_tcp_set_sndbuf(int fd, int value)
{
	return setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &value, sizeof(int));
}
/**
 * @brief Get sndbuf.
 */
static inline int wuy_tcp_get_sndbuf(int fd)
{
	int value;
	socklen_t size = sizeof(int);
	getsockopt(fd, SOL_SOCKET, SO_SNDBUF, &value, &size);
	return value;
}
/**
 * @brief Set rcvbuf.
 */
static inline int wuy_tcp_set_rcvbuf(int fd, int value)
{
	return setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &value, sizeof(int));
}
/**
 * @brief Get rcvbuf.
 */
static inline int wuy_tcp_get_rcvbuf(int fd)
{
	int value;
	socklen_t size = sizeof(int);
	getsockopt(fd, SOL_SOCKET, SO_RCVBUF, &value, &size);
	return value;
}
/**
 * @brief Set cork.
 */
static inline int wuy_tcp_set_cork(int fd, int cork)
{
	return setsockopt(fd, IPPROTO_TCP, TCP_CORK, &cork, sizeof(int));
}
/**
 * @brief Set defer_accept.
 */
static inline int wuy_tcp_set_defer_accept(int fd, int value)
{
	return setsockopt(fd, IPPROTO_TCP, TCP_DEFER_ACCEPT, &value, sizeof(int));
}

/**
 * @brief Bind and listen on the address.
 */
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

	if (bind(fd, sa, wuy_sockaddr_size(sa)) < 0) {
		close(fd);
		return -1;
	}

	listen(fd, backlog);
	return fd;
}

/**
 * @brief Accept new connections.
 */
static inline int wuy_tcp_accept(int listen_fd, struct sockaddr_storage *client_addr)
{
	socklen_t addrlen = sizeof(struct sockaddr_storage);
	return accept4(listen_fd, (struct sockaddr *)client_addr, &addrlen, SOCK_NONBLOCK);
}

/**
 * @brief Connect to the address.
 *
 * You should check the errno to know if the connection is ready to write.
 */
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
