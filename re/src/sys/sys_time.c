#include <re_types.h>
#include <ctype.h>
#include <stdlib.h>
#include <re_sys.h>
#include <time.h>
#include <sys\timeb.h>
#include <stdio.h>
#include "stm32f4xx_hal.h"

char* sys_time(char* buf, int size) {
    unsigned int ticks = HAL_GetTick();
    snprintf(buf, size, "%u.%03u", ticks/1000, ticks%1000);
	buf[size-1] = '\0';
	return buf;
}

