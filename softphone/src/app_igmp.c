#include "app_igmp.h"
#include "lwip/opt.h"
#include "lwip/igmp.h"
#include <stdio.h>

void app_igmp_join(uint32_t local_ip)
{
#if 0
    struct ip_addr SCREAM_MCAST_ADDR;//, localIP;
    IP4_ADDR(&SCREAM_MCAST_ADDR, 234, 2, 1, 1);
    //IP4_ADDR(&localIP, 192, 168, 2, 211);
    err_t err = igmp_joingroup((ip_addr_t *)&local_ip, (ip_addr_t *)(&SCREAM_MCAST_ADDR));
    if (err != ERR_OK)
    {
        printf("UDP RX: error %d trying to join IGMP group\n", err);
    }
    else
    {
        printf("joined multicast group on interface %u.%u.%u.%u\n",
               ip4_addr1(&local_ip), ip4_addr2(&local_ip), ip4_addr3(&local_ip), ip4_addr4(&local_ip));
    }
#endif
}
