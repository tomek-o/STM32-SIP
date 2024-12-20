/**
 * @file re_sa.h  Interface to Socket Address
 *
 * Copyright (C) 2010 Creytiv.com
 */
#if defined(WIN32) || defined(__WIN32__)

#include <winsock2.h>

#ifdef __BORLANDC__
#pragma warn -8004
#endif

#include <ws2tcpip.h>

#ifdef __BORLANDC__
#pragma warn .8008
#endif

#else

#include <sys/types.h>
#include <sys/socket.h>
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#endif


#ifdef __cplusplus
extern "C" {
#endif

struct pl;

/** Socket Address flags */
enum sa_flag {
	SA_ADDR      = 1<<0,
	SA_PORT      = 1<<1,
	SA_ALL       = SA_ADDR | SA_PORT
};

/** Defines a Socket Address */
struct sa {
	union {
		struct sockaddr sa;
		struct sockaddr_in in;
#ifdef HAVE_INET6
		struct sockaddr_in6 in6;
#endif

#ifdef LWIP_SOCKET
        socklen_t len;
#endif
	} u;
#ifndef LWIP_SOCKET
	socklen_t len;
#endif
};

void     sa_init(struct sa *sa, int af);
int      sa_set(struct sa *sa, const struct pl *addr, uint16_t port);
int      sa_set_str(struct sa *sa, const char *addr, uint16_t port);
void     sa_set_in(struct sa *sa, uint32_t addr, uint16_t port);
void     sa_set_in6(struct sa *sa, const uint8_t *addr, uint16_t port);
int      sa_set_sa(struct sa *sa, const struct sockaddr *s);
void     sa_set_port(struct sa *sa, uint16_t port);
int      sa_decode(struct sa *sa, const char *str, size_t len);

int      sa_af(const struct sa *sa);
uint32_t sa_in(const struct sa *sa);
void     sa_in6(const struct sa *sa, uint8_t *addr);
int      sa_ntop(const struct sa *sa, char *buf, int size);
uint16_t sa_port(const struct sa *sa);
bool     sa_isset(const struct sa *sa, int flag);
uint32_t sa_hash(const struct sa *sa, int flag);

void     sa_cpy(struct sa *dst, const struct sa *src);
bool     sa_cmp(const struct sa *l, const struct sa *r, int flag);

bool     sa_is_linklocal(const struct sa *sa);
bool     sa_is_loopback(const struct sa *sa);
bool     sa_is_any(const struct sa *sa);

struct re_printf;
int      sa_print_addr(struct re_printf *pf, const struct sa *sa);

/* Net ntop/pton */
int      net_inet_ntop(const struct sa *sa, char *buf, int size);
int      net_inet_pton(const char *addr, struct sa *sa);


#ifdef __cplusplus
}
#endif
