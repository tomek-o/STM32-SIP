#include <re.h>
#include <lwip/sockets.h>
#include <stdint.h>
#include "stm32f4xx_hal.h"

static uint32_t prev_tick = 0;
static uint64_t tick64 = 0;

int gettimeofday(struct timeval *tv, void* unused)
{
    __disable_irq();

    uint32_t tick = HAL_GetTick();

    if (prev_tick > tick) {
        tick64 += 0x100000000ULL;
    }
    prev_tick = tick;
    tick64 = (tick64 & 0xFFFFFFFF00000000ULL) + tick;

    tv->tv_usec = (tick64 % 1000) * 1000;
    tv->tv_sec = tick64 / 1000;

    __enable_irq();

    return 0;
}
