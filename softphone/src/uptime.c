#include "uptime.h"
#include "shell.h"
#include <inttypes.h>
#include <stdio.h>

static const unsigned int PERIOD = 1000;

static uint32_t uptime = 0;

void uptimeIncrement(void)
{
    uptime++;
}

uint32_t uptimeGet(void)
{
    return uptime;
}

void uptimeHandle(void)
{
    static uint32_t tickstart = 0;
    if (HAL_GetTick() - tickstart >= PERIOD) {
        tickstart += PERIOD;
        uptimeIncrement();
    }
}

static enum shell_error sh_uptime(int argc, char ** argv) {
    printf("Uptime %"PRIu32" s (%u:%02u:%02u)\n", uptime, (unsigned int)uptime/3600, (unsigned int)(uptime%3600)/60, (unsigned int)uptime%60);
    return SHELL_ERR_NONE;
}

static __attribute__((constructor)) void registerCmd(void)
{
	shell_add("uptime", (void*)sh_uptime, "Show uptime");
}
