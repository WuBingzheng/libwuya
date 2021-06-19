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
 * Transfer socket address between struct sockaddr and string.
 */

#ifndef WUY_SOCKADDR_H
#define WUY_SOCKADDR_H

#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>

/**
 * @brief convert address (ip + port) from string into struct sockaddr.
 *
 * IPv4 and IPv6 are supported. Formats:
 *
 *     IPv6:port        - [2006:0724:2241::8535]:80
 *     IPv6             - [2006:0724:2241::8535]
 *     port for IPv6    - []:80
 *     IPv4:port        - 1.2.3.4:80
 *     IPv4             - 1.2.3.4
 *     port for IPv4    - 80
 *
 * @param default_port if 0, port is mandatory which is always used to listen;
 *                     else IP is mandatory which is always used to connect.
 *
 * @return true on success, and false on fail.
 */
bool wuy_sockaddr_loads(const char *str, struct sockaddr_storage *out,
		unsigned short default_port);

/**
 * @brief convert address (ip + port) from struct sockaddr_storage into string.
 *
 * @return NULL on error, or @buf if success.
 */
const char *wuy_sockaddr_dumps(const struct sockaddr *sa, char *buf, int size);

/**
 * @brief convert address ip only from struct sockaddr_storage into string.
 *
 * @return NULL on error, or @buf if success.
 */
const char *wuy_sockaddr_dumps_iponly(const struct sockaddr *sa, char *buf, int size);

/**
 * @brief return size of address by sa->sa_family.
 */
size_t wuy_sockaddr_size(const struct sockaddr *sa);

/**
 * @brief compare 2 address, include port
 */
static inline int wuy_sockaddr_cmp(const struct sockaddr *sa, const struct sockaddr *sb)
{
	return memcmp(sa, sb, wuy_sockaddr_size(sa));
}

/**
 * @brief compare 2 address, exclude port
 */
int wuy_sockaddr_addrcmp(const struct sockaddr *sa, const struct sockaddr *sb);

#endif
