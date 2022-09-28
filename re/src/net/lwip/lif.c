/**
 * @file posix/pif.c  POSIX network interface code
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <re_types.h>
#include <re_fmt.h>
#include <re_mbuf.h>
#include <re_sa.h>
#include <re_net.h>
#include <lwip/netif.h>


#define DEBUG_MODULE "posixif"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


/**
 * Get IP address for a given network interface
 *
 * @param ifname  Network interface name
 * @param af      Address Family
 * @param ip      Returned IP address
 *
 * @return 0 if success, otherwise errorcode
 *
 * @deprecated Works for IPv4 only
 */
int net_if_getaddr4(const char *ifname, int af, struct sa *ip)
{
    extern struct netif gnetif;
    sa_set_in(ip, gnetif.ip_addr.addr, 0);
	return 0;
}


/**
 * Enumerate all network interfaces
 *
 * @param ifh Interface handler
 * @param arg Handler argument
 *
 * @return 0 if success, otherwise errorcode
 *
 * @deprecated Works for IPv4 only
 */
int net_if_list(net_ifaddr_h *ifh, void *arg)
{
	struct sa sa;

    extern struct netif gnetif;
    sa_set_in(&sa, ntohl(gnetif.ip_addr.addr), 0);

	if (ifh) {
        ifh("", &sa, arg);
	}

	return 0;
}
