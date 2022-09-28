#include "ethernetif_mac.h"
#include "stm32f4xx.h"
#include <stdio.h>
#include <stdbool.h>

static uint8_t mac[6];
static bool initialized = false;

/* Using CRC32 as simple hash function */
static unsigned int crc32b(const uint8_t *data, unsigned int len)
{
    int i, j;
    unsigned int byte, crc, mask;

    i = 0;
    crc = 0xFFFFFFFF;
    while (len > 0)
    {
        len--;
        byte = data[i];            // Get next byte.
        crc = crc ^ byte;
        for (j = 7; j >= 0; j--)      // Do eight times.
        {
            mask = -(crc & 1);
            crc = (crc >> 1) ^ (0xEDB88320 & mask);
        }
        i = i + 1;
    }
    return ~crc;
}

uint8_t* get_ethernetif_mac(void)
{
    if (!initialized)
    {
        initialized = true;
        // 96 bits / 12 bytes of unique id
        const uint8_t *uid = (uint8_t *)UID_BASE;
        printf("UID: ");
        for (int i=0; i<12; i++) {
            printf("%02X, ", uid[i]);
        }
        printf("\n");
        unsigned int crc = crc32b(uid, 12);
        printf("crc = %08X\n", crc);
        uint8_t *crc_ptr = (uint8_t*)&crc;

        mac[0] = 0x02;
        mac[1] = 0x00;
        mac[2] = 0x00;
        mac[3] = crc_ptr[0];
        mac[4] = crc_ptr[1];
        mac[5] = crc_ptr[2];
        printf("MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
    return mac;
}
