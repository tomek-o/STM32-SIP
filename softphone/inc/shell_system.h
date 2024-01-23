#ifndef SHELL_SYSTEM_H
#define SHELL_SYSTEM_H

#include "stm32f4xx_hal.h"

int shell_init(UART_HandleTypeDef* huart);
void shell_on_rx_char(const char shell_character);

#endif
