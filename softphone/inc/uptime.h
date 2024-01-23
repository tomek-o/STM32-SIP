#ifndef UPTIME_H
#define UPTIME_H

#include <stdint.h>

void uptimeIncrement(void);
uint32_t uptimeGet(void);

/** \brief Calls uptimeIncrement depending on HAL_Tick()
*/
void uptimeHandle(void);

#endif
