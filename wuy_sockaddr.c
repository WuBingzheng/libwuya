/*
 * transfer socket address
 *
 * Author: Wu Bingzheng
 *
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>

#include "wuy_sockaddr.h"

static bool wuy_port_pton(const char *str, unsigned short *pport)
{
	unsigned n = 0;
	for (const char *p = str; *p != '\0'; p++) {
		if (*p < '0' || *p > '9') {
			return false;
		}
		n = n * 10 + *p - '0';
		if (n > 65535) {
			return false;
		}
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
		memcpy(tmpbuf, str + 1, ip_end - str - 1);
		tmpbuf[ip_end - str - 1] = '\0';
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

bool wuy_sockaddr_loads(const char *str, struct sockaddr_storage *out,
		unsigned short default_port)
{
	/* Unix Domain */
	if (memcmp(str, "unix:", 5) == 0) {
		struct sockaddr_un *sun = (struct sockaddr_un *)out;
		out->ss_family = AF_UNIX;
		strncpy(sun->sun_path, str + 5, sizeof(sun->sun_path));
		return sun->sun_path[sizeof(sun->sun_path) - 1] == '\0';
	}

	/* IPv4 and IPv6 */
	if (str[0] == '[') {
		return wuy_sockaddr_pton_ipv6(str, (struct sockaddr_in6 *)out, default_port);
	} else {
		return wuy_sockaddr_pton_ipv4(str, (struct sockaddr_in *)out, default_port);
	}
}

const char *wuy_sockaddr_dumps_iponly(const struct sockaddr *sa, char *buf, int size)
{
	switch (sa->sa_family) {
	case AF_INET: {
		struct sockaddr_in *sain = (struct sockaddr_in *)sa;
		return inet_ntop(AF_INET, &sain->sin_addr, buf, size);
	}
	case AF_INET6: {
		struct sockaddr_in6 *sain6 = (struct sockaddr_in6 *)sa;
		buf[0] = '[';
		if (inet_ntop(AF_INET6, &sain6->sin6_addr, buf + 1, size - 2) == NULL) {
			return NULL;
		}
		char *end = buf + strlen(buf);
		end[0] = ']';
		end[1] = '0';
		return buf;
	}
	case AF_UNIX: {
		const struct sockaddr_un *sun = (const struct sockaddr_un *)sa;
		memcpy(buf, "unix:", 5);
		strncpy(buf + 5, sun->sun_path, size - 5);
		return buf;
	}
	default:
		return NULL;
	}
}

const char *wuy_sockaddr_dumps(const struct sockaddr *sa, char *buf, int size)
{
	if (wuy_sockaddr_dumps_iponly(sa, buf, size) == NULL) {
		return NULL;
	}

	unsigned port = 0;
	switch (sa->sa_family) {
	case AF_INET: {
		struct sockaddr_in *sain = (struct sockaddr_in *)sa;
		port = ntohs(sain->sin_port);
		break;
	}
	case AF_INET6: {
		struct sockaddr_in6 *sain6 = (struct sockaddr_in6 *)sa;
		port = ntohs(sain6->sin6_port);
		break;
	}
	default:
		return buf;
	}

	int len = strlen(buf);
	snprintf(buf + len, size - len, ":%d", port);
	return buf;
}

size_t wuy_sockaddr_size(const struct sockaddr *sa)
{
	switch (sa->sa_family) {
	case AF_INET:
		return sizeof(struct sockaddr_in);
	case AF_INET6:
		return sizeof(struct sockaddr_in6);
	case AF_UNIX:
		return sizeof(sa->sa_family) + strlen(((const struct sockaddr_un *)sa)->sun_path) + 1;
	default:
		abort();
	}
}

int wuy_sockaddr_addrcmp(const struct sockaddr *sa, const struct sockaddr *sb)
{
	if (sa->sa_family < sb->sa_family) {
		return -1;
	}
	if (sa->sa_family > sb->sa_family) {
		return 1;
	}
	switch (sa->sa_family) {
	case AF_INET: {
		const struct sockaddr_in *sina = (const void *)sa;
		const struct sockaddr_in *sinb = (const void *)sb;
		return memcmp(&sina->sin_addr, &sinb->sin_addr, sizeof(sina->sin_addr));
	}
	case AF_INET6: {
		const struct sockaddr_in6 *sin6a = (const void *)sa;
		const struct sockaddr_in6 *sin6b = (const void *)sb;
		return memcmp(&sin6a->sin6_addr, &sin6b->sin6_addr, sizeof(sin6a->sin6_addr));
	}
	case AF_UNIX: {
		const struct sockaddr_un *suna = (const void *)sa;
		const struct sockaddr_un *sunb = (const void *)sb;
		return memcmp(&suna->sun_path, &sunb->sun_path, sizeof(suna->sun_path));
	}
	default:
		abort();
	}
}
