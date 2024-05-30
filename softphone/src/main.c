#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "cmsis_os.h"
#include "ethernetif.h"
#include "app_ethernet.h"
#include "app_igmp.h"
#include "httpserver-netconn.h"
#include "asserts.h"
#include "uart.h"
#include "dac.h"
#include "adc.h"
#include "rng.h"
#include "sip_ua.h"
#include "ip_addr_config.h"
#include "version.h"
#include <inttypes.h>

#include "stm32f4xx_hal.h"


struct netif gnetif; /* network interface structure */


static void SystemClock_Config(void);
static void StartThread(void const * argument);
static void Netif_Config(void);


int main(void)
{
    /* STM32F4xx HAL library initialization:
         - Configure the Flash ART accelerator on ITCM interface
         - Configure the Systick to generate an interrupt each 1 msec
         - Set NVIC Group Priority to 4
         - Global MSP (MCU Support Package) initialization
       */
    HAL_Init();

    SystemClock_Config();

    uart_init();

    printf("\n*******************************************\n");
    printf("%s version %s\n", DEV_NAME, APP_VERSION);
    printf("Built: %s\n", BUILD_TIMESTAMP);
    printf("Core clock: %" PRIu32 "\n", SystemCoreClock);

#ifdef USE_DHCP
    printf("Using DHCP\n");
#else
    printf("Using static IP: %d.%d.%d.%d/%d.%d.%d.%d\n", IP_ADDR0, IP_ADDR1, IP_ADDR2, IP_ADDR3, NETMASK_ADDR0, NETMASK_ADDR1, NETMASK_ADDR2, NETMASK_ADDR3);
#endif

    if (uart_init_shell() != 0) {
        printf("\n\nFailed to init shell!\n\n");
    }

    MX_RNG_Init();

    {
        uint32_t val;
        int status = HAL_RNG_GenerateRandomNumber(&hrng, &val);
        if (status != HAL_OK) {
            printf("Failed to generate random seed for stdlib!\n");
        } else {
            srand(val);
        }
    }


    dac_init();
    adc_init();

    /* Init thread */
    osThreadDef(Start, StartThread, osPriorityNormal, 0, configMINIMAL_STACK_SIZE * 5);

    osThreadCreate (osThread(Start), NULL);

    /* Start scheduler */
    osKernelStart();

    /* We should never get here as control is now taken by the scheduler */
    for( ;; ) {
    }
}


static void StartThread(void const * argument)
{
    /* Create tcp_ip stack thread */
    tcpip_init(NULL, NULL);

    /* Initialize the LwIP stack */
    Netif_Config();

    /* Initialize webserver demo */
    http_server_netconn_init();

    /* Notify user about the network interface config */
    User_notification(&gnetif);

#ifdef USE_DHCP

    /* Start DHCPClient */
    osThreadDef(DHCP, DHCP_thread, osPriorityBelowNormal, 0, configMINIMAL_STACK_SIZE * 5);
    osThreadCreate (osThread(DHCP), &gnetif);

#else   // !USE_DHCP

    {
        struct ip_addr ipaddr;
        printf("Using static IP %u.%u.%u.%u\n", IP_ADDR0, IP_ADDR1, IP_ADDR2, IP_ADDR3);
        IP4_ADDR(&ipaddr, IP_ADDR0 ,IP_ADDR1 , IP_ADDR2 , IP_ADDR3 );
        app_igmp_join(ipaddr.addr);
        sip_ua_init();
    }

#endif  // USE_DHCP


    for( ;; )
    {
        /* Delete the Init Thread */
        osThreadTerminate(NULL);
    }
}

/**
  * @brief  Initializes the lwIP stack
  */
static void Netif_Config(void)
{
    struct ip_addr ipaddr;
    struct ip_addr netmask;
    struct ip_addr gw;

    /* IP address setting */
    IP4_ADDR(&ipaddr, IP_ADDR0, IP_ADDR1, IP_ADDR2, IP_ADDR3);
    IP4_ADDR(&netmask, NETMASK_ADDR0, NETMASK_ADDR1 , NETMASK_ADDR2, NETMASK_ADDR3);
    IP4_ADDR(&gw, GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);

    /* - netif_add(struct netif *netif, struct ip_addr *ipaddr,
    struct ip_addr *netmask, struct ip_addr *gw,
    void *state, err_t (* init)(struct netif *netif),
    err_t (* input)(struct pbuf *p, struct netif *netif))

    Adds your network interface to the netif_list. Allocate a struct
    netif and pass a pointer to this structure as the first argument.
    Give pointers to cleared ip_addr structures when using DHCP,
    or fill them with sane numbers otherwise. The state pointer may be NULL.

    The init function pointer must point to a initialization function for
    your ethernet netif interface. The following code illustrates it's use.*/

    netif_add(&gnetif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &tcpip_input);

    /*  Registers the default network interface. */
    netif_set_default(&gnetif);

    if (netif_is_link_up(&gnetif))
    {
        /* When the netif is fully configured this function must be called.*/
        netif_set_up(&gnetif);
    }
    else
    {
        /* When the netif link is down this function must be called */
        netif_set_down(&gnetif);
    }
}


/** \note Clock should be multiple of DAC/ADC sampling frequency */
/** \note For USB: 48 MHz is needed (MAINPLL / Q) */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* Enable Power Control clock */
    __HAL_RCC_PWR_CLK_ENABLE();

    /* The voltage scaling allows optimizing the power consumption when the device is
       clocked below the maximum system frequency, to update the voltage scaling value
       regarding system frequency refer to product datasheet.  */
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    /* Enable HSE Oscillator and activate PLL with HSE as source */
    /* NUCLEO-F429ZI: 8 MHz external clock (= HSE_VALUE) */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 8;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 7;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    /** Initializes the CPU, AHB and APB busses clocks
    */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)  // flash latency = 4 for 144 MHz, 5 for 168 MHz
    {
        Error_Handler();
    }
}


#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  */
void assert_failed(uint8_t* file, uint32_t line)
{
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

    /* Infinite loop */
    while (1)
    {
    }
}
#endif

void vApplicationStackOverflowHook( xTaskHandle pxTask, signed char *pcTaskName )
{
    printf("STACK OVERFLOW!\n");
    while (1)
    {
    }
}
