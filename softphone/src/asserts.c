#include "asserts.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_nucleo_144.h"

/**
  * @brief  This function is executed in case of error occurrence.
  */
void Error_Handler(void)
{
  /* Turn LED2 on */
  BSP_LED_On(LED_RED);
  while (1)
  {
  }
}
