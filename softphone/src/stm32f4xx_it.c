#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include "main.h"
#include "ethernetif.h"
#include "uart.h"


/******************************************************************************/
/*            Cortex-M4 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
 * @brief  This function handles NMI exception.
 */
void NMI_Handler(void) {
}

/**
 * @brief  This function handles Hard Fault exception.
 */
void HardFault_Handler(void) {
	printf("HardFault!\n");
	/* Go to infinite loop when Hard Fault exception occurs */
	while (1) {
	}
}

/**
 * @brief  This function handles Memory Manage exception.
 */
void MemManage_Handler(void) {
	/* Go to infinite loop when Memory Manage exception occurs */
	while (1) {
	}
}

/**
 * @brief  This function handles Bus Fault exception.
 */
void BusFault_Handler(void) {
	/* Go to infinite loop when Bus Fault exception occurs */
	while (1) {
	}
}

/**
 * @brief  This function handles Usage Fault exception.
 */
void UsageFault_Handler(void) {
	/* Go to infinite loop when Usage Fault exception occurs */
	while (1) {
	}
}

/**
 * @brief  This function handles Debug Monitor exception.
 */
void DebugMon_Handler(void) {
}

/**
 * @brief  This function handles SysTick Handler.
 */
void SysTick_Handler(void) {
	HAL_IncTick();
	osSystickHandler();
}

/******************************************************************************/
/*                 STM32F4xx Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f4xx.s).                                               */
/******************************************************************************/

/**
 * @brief  This function handles Ethernet interrupt request.
 */
void ETH_IRQHandler(void) {
	ETHERNET_IRQHandler();
}

/**
 * @brief  This function handles DMA interrupt request.
 */
void DMA2_Stream0_IRQHandler(void) {

}

/**
 * @brief  This function handles PPP interrupt request.
 */
/*void PPP_IRQHandler(void)
 {
 }*/

