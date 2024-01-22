#ifndef SHELL_SYSTEM_H
#define SHELL_SYSTEM_H

#include "stm32f4xx_hal.h"

int shell_init(UART_HandleTypeDef* huart);
uint8_t shell_char_received(void);
void shell_poll(void);

#endif
