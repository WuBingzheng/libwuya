/*
 * transfer socket address
 *
 * Author: Wu Bingzheng
 *
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>

#include "wuy_sockaddr.h"

static bool wuy_port_pton(const char *str, unsigned short *pport)
{
	unsigned n = 0;
	const char *p = str;
	while (*p) {
		if (*p < '0' || *p > '9') {
			return false;
		}
		n = n * 10 + *p - '0';
		if (n > 65535) {
			return false;
		}
		p++;
	}
	if (n == 0) {
		return false;
	}

	*pport = htons(n);
	return true;
}

static bool wuy_sockaddr_pton_ipv4(const char *str, struct sockaddr_in *out,
		unsigned short default_port)
{
	bzero(out, sizeof(struct sockaddr_in));
	out->sin_family = AF_INET;

	const char *port_begin = strchr(str, ':');

	if (port_begin == NULL) {
		if (default_port != 0) { /* ip is mandatory */
			if (!inet_pton(AF_INET, str, &(out->sin_addr))) {
				return false;
			}
			out->sin_port = htons(default_port);
		} else { /* port is mandatory */
			if (!wuy_port_pton(str, &(out->sin_port))) {
				return false;
			}
		}
	} else {
		char tmpbuf[port_begin - str + 1];
		memcpy(tmpbuf, str, port_begin - str);
		tmpbuf[port_begin - str] = '\0';
		if (!inet_pton(AF_INET, tmpbuf, &(out->sin_addr))) {
			return false;
		}
		if (!wuy_port_pton(port_begin + 1, &(out->sin_port))) {
			return false;
		}
	}
	return true;
}

static bool wuy_sockaddr_pton_ipv6(const char *str, struct sockaddr_in6 *out,
		unsigned short default_port)
{
	const char *port_begin = NULL;

	bzero(out, sizeof(struct sockaddr_in6));
	out->sin6_family = AF_INET6;

	const char *ip_end = strchr(str, ']');
	if (ip_end == NULL) {
		return false;
	}

	if (ip_end[1] == ':') {
		port_begin = ip_end + 2;
	} else if (ip_end[1] != '\0') {
		return false;
	}

	/* ip */
	if (ip_end != str + 1) {
		char tmpbuf[ip_end - str + 1];
		memcpy(tmpbuf, str, ip_end - str);
		tmpbuf[ip_end - str] = '\0';
		if (!inet_pton(AF_INET6, tmpbuf, &(out->sin6_addr))) {
			return false;
		}
	} else if (default_port != 0) { /* ip is mandatory */
		return false;
	}

	/* port */
	if (port_begin != NULL) {
		if (!wuy_port_pton(port_begin, &(out->sin6_port))) {
			return false;
		}
	} else if (default_port != 0) {
		out->sin6_port = htons(default_port);
	} else { /* port is mandatory */
		return false;
	}
	return true;
}

bool wuy_sockaddr_pton(const char *str, struct sockaddr *out, unsigned short default_port)
{
	return (str[0] == '[')
		? wuy_sockaddr_pton_ipv6(str, (struct sockaddr_in6 *)out, default_port)
		: wuy_sockaddr_pton_ipv4(str, (struct sockaddr_in *)out, default_port);
}

int wuy_sockaddr_ntop(const struct sockaddr *sa, char *buf, int size)
{
	if (sa->sa_family == AF_INET) {
		struct sockaddr_in *sain = (struct sockaddr_in *)sa;
		if (inet_ntop(AF_INET, &sain->sin_addr, buf, size) == NULL) {
			return -1;
		}
		int ip_len = strlen(buf);
		return snprintf(buf + ip_len, size - ip_len, ":%d",
				ntohs(sain->sin_port)) + ip_len;

	} else if (sa->sa_family == AF_INET6) {
		struct sockaddr_in6 *sain6 = (struct sockaddr_in6 *)sa;
		buf[0] = '[';
		if (inet_ntop(AF_INET6, &sain6->sin6_addr, buf + 1, size - 1) == NULL) {
			return -1;
		}
		int ip_len = strlen(buf);
		return snprintf(buf + ip_len, size - ip_len, "]:%d",
				ntohs(sain6->sin6_port)) + ip_len;

	} else {
		return -2;
	}
}
