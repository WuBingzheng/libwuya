/*
 * transfer socket address
 *
 * Author: Wu Bingzheng
 *
 */

#ifndef WUY_SOCKADDR_H
#define WUY_SOCKADDR_H

#include <stdbool.h>
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
bool wuy_sockaddr_pton(const char *str, struct sockaddr *out, unsigned short default_port);

/**
 * @brief convert address (ip + port) from struct sockaddr into string.
 *
 * @return -1 on fail, or the length of string, excluding the null byte.
 */
int wuy_sockaddr_ntop(const struct sockaddr *sa, char *buf, int size);

#endif
