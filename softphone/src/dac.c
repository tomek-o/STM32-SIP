/**
  	Each time a DAC interface detects a rising edge on the selected Timer Trigger Output
	(TIMx_TRGO), the last data stored in the DAC_DHRx register is transferred to the
	DAC_DORx register.

	Total of 4APB clock cycles are needed to update the DAC DOR register data. As APB1
	maximum clock is 45 MHz (for STM32F42x), 11.25 Msps is the maximum update rate for the
	DAC output register when timer trigger and the DMA are used for the data update.

	Basic timer (TIM6&TIM7) features include:
	- 16-bit auto-reload upcounter
	- 16-bit programmable prescaler used to divide (also “on the fly”) the counter clock frequency by any factor between 1 and 65536
	- Synchronization circuit to trigger the DAC
	- Interrupt/DMA generation on the update event: counter overflow
	Counter Register (TIMx_CNT)
	Prescaler Register (TIMx_PSC)
	Auto-Reload Register (TIMx_ARR)
 */

/** STM32F429
The two 12-bit buffered DAC channels can be used to convert two digital signals into two
analog voltage signal outputs.
This dual digital Interface supports the following features:
- two DAC converters: one for each output channel
- 8-bit or 10-bit monotonic output
- left or right data alignment in 12-bit mode
- synchronized update capability
- noise-wave generation
- triangular-wave generation
- dual DAC channel independent or simultaneous conversions
- DMA capability for each channel
- external triggers for conversion
- input voltage reference V REF+
Eight DAC trigger inputs are used in the device. The DAC channels are triggered through
the timer update outputs that are also connected to different DMA streams.
*/

#include "dac.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_dac.h"
#include "stm32f4xx_hal_tim.h"
#include "asserts.h"
#include "utils.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>


#define LOCAL_DEBUG
#ifdef LOCAL_DEBUG
#	define TRACE(args) (printf("DAC: "), printf args)
#else
#	define TRACE(args)
#endif
#	define MSG(args) (printf("DAC: "), printf args)

enum { DAC_SAMPLING_FREQUENCY = 16000 };
enum { DAC_OFFSET = 32768 };

/* Definition for DAC clock resources */
#define DACx_CHANNEL1_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOA_CLK_ENABLE()
#define DMAx_CLK_ENABLE()               __HAL_RCC_DMA1_CLK_ENABLE()

#define DACx_FORCE_RESET()              __HAL_RCC_DAC_FORCE_RESET()
#define DACx_RELEASE_RESET()            __HAL_RCC_DAC_RELEASE_RESET()

/* Pins */
#define DACx_CHANNEL1_PIN                GPIO_PIN_4
#define DACx_CHANNEL1_GPIO_PORT          GPIOA
#define DACx_CHANNEL2_PIN                GPIO_PIN_5
#define DACx_CHANNEL2_GPIO_PORT          GPIOA


/* DMA */
#define DACx_DMA_CHANNEL1                DMA_CHANNEL_7
#define DACx_DMA_STREAM1                 DMA1_Stream5

/* Definition for DACx's NVIC */
#define DACx_DMA_IRQn1                   DMA1_Stream5_IRQn
#define DACx_DMA_IRQHandler1             DMA1_Stream5_IRQHandler

DAC_HandleTypeDef DacHandle;
static DAC_ChannelConfTypeDef sConfig;
/// \note using DAC_ALIGN_12B_L
/// \note Some issues observed with combination of G.722 and smaller DAC tables
static uint32_t dacSampleTable[DAC_SAMPLING_FREQUENCY/25] = { 0 };
static int32_t samplesSum[DAC_SAMPLING_FREQUENCY/50] = { 0 };
static int16_t callbackBuf[DAC_SAMPLING_FREQUENCY/50] = { 0 };
static void TIM6_Config(void);

volatile uint32_t *samples_ptr = NULL;

struct dac_client {
    dac_callback *dac_cb;
    unsigned int samples_count_ratio;   ///< assuming that requested frequency would be equal to 1/N of DAC native frequency or equal to DAC native frequency itself
    void *arg;
};

static struct dac_client clients[4] = {0};

int dac_client_register(dac_callback *dac_cb, unsigned int sampling_rate, void *arg) {
    assert(arg != NULL);
    if (DAC_SAMPLING_FREQUENCY % sampling_rate) {   // using trivial resampling by duplicating samples
        MSG(("Requested sampling rate = %u is not accepted for trivial resampling\n", sampling_rate));
        return -2;
    }
    for (unsigned int i=0; i<ARRAY_SIZE(clients); i++) {
        struct dac_client *c = &clients[i];
        if (c->arg == NULL) {
            c->arg = arg;
            c->samples_count_ratio = DAC_SAMPLING_FREQUENCY / sampling_rate;
            c->dac_cb = dac_cb;
            return 0;
        }
    }
    MSG(("Empty DAC client structure not found!\n"));
    return -1;
}

void dac_client_unregister(void *arg) {
    assert(arg != NULL);
    for (unsigned int i=0; i<ARRAY_SIZE(clients); i++) {
        struct dac_client *c = &clients[i];
        if (c->arg == arg) {
        #if 0
            /** \note Ugly: waiting for possible callback end to avoid hazard (st->ready was cleared earlier) */
            /** not needed since all is single threaded for this STM32F429 configuration */
            HAL_Delay(20);
        #endif
            memset(c, 0, sizeof(*c));
            break;
        }
    }
}

static void collect_samples(uint32_t *ptr) {
    memset(samplesSum, 0, sizeof(samplesSum));
    unsigned int sumPos = 0;
    for (unsigned int i=0; i<ARRAY_SIZE(clients); i++) {
        struct dac_client *c = &clients[i];
        if (c->dac_cb) {
            memset(callbackBuf, 0, sizeof(callbackBuf));
            unsigned int samplesCount = ARRAY_SIZE(callbackBuf) / c->samples_count_ratio;
            c->dac_cb(callbackBuf, samplesCount, c->arg);
            for (unsigned int j=0; j<samplesCount; j++) {
                for (unsigned int k=0; k<c->samples_count_ratio; k++) {
                    samplesSum[sumPos++] += callbackBuf[j];
                }
            }
        }
    }
    for (unsigned int i=0; i<ARRAY_SIZE(samplesSum); i++) {
        int32_t val = samplesSum[i];
        if (val > 32767)
            val = 32767;
        if (val < -32768)
            val = 32768;
        val += DAC_OFFSET;
		ptr[i] = val;
		ptr[i] |= ((uint32_t)val) << 16; //
    }
}

void dac_poll(void) {
    static volatile uint32_t *prev_samples_ptr = NULL;
    if (samples_ptr != prev_samples_ptr) {
        prev_samples_ptr = samples_ptr;
        // this must not be called in ISR context! assert when freeing memory
        collect_samples((uint32_t*)samples_ptr);
    }
}

void dac_init(void) {
	/*##-1- Configure the DAC peripheral #######################################*/
	DacHandle.Instance = DAC;

	/*##-2- Configure the TIM peripheral #######################################*/
	TIM6_Config();

	HAL_DAC_DeInit(&DacHandle);

	/*##-1- Initialize the DAC peripheral ######################################*/
	if (HAL_DAC_Init(&DacHandle) != HAL_OK) {
		Error_Handler();
	}

	/*##-1- DAC channel1 Configuration #########################################*/
	sConfig.DAC_Trigger = DAC_TRIGGER_T6_TRGO;
	sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;

	if (HAL_DAC_ConfigChannel(&DacHandle, &sConfig, DAC_CHANNEL_1) != HAL_OK) {
		Error_Handler();
	}

	if (HAL_DAC_ConfigChannel(&DacHandle, &sConfig, DAC_CHANNEL_2) != HAL_OK) {
		Error_Handler();
	}

	TRACE(("Starting DMA\n"));
#if 1
	if (HAL_DAC_Start_DualDMA(&DacHandle, DAC_CHANNEL_12D, (uint32_t*) dacSampleTable,
#else
	if (HAL_DAC_Start_DualDMA(&DacHandle, DAC_CHANNEL_1, (uint32_t*) dacSampleTable,
#endif
			ARRAY_SIZE(dacSampleTable), DAC_ALIGN_12B_L) != HAL_OK) {
		TRACE(("Error starting DMA\n"));
		Error_Handler();
	}
	TRACE(("DMA started\n"));
}


/** @brief  TIM6 Configuration
 * @note   TIM6 configuration is based on APB1 frequency
 * @note   TIM6 Update event occurs each TIM6CLK/256
 */
void TIM6_Config(void) {
	static TIM_HandleTypeDef htim;
	TIM_MasterConfigTypeDef sMasterConfig;

	htim.Instance = TIM6;

    if ((SystemCoreClock/2) % DAC_SAMPLING_FREQUENCY) {
        MSG(("\nTimer clock = %lu is not evenly divisable by DAC sampling frequency!\n", SystemCoreClock/2));
    }
	htim.Init.Period = ((SystemCoreClock/2)/DAC_SAMPLING_FREQUENCY) - 1;
	htim.Init.Prescaler = 0;
	htim.Init.ClockDivision = 0;
	// The counter counts from 0 to the auto-reload value (contents of the TIMx_ARR register), then restarts from 0 and generates a counter overflow event.
	htim.Init.CounterMode = TIM_COUNTERMODE_UP;
	HAL_TIM_Base_Init(&htim);

	/* TIM6 TRGO selection */
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;

	HAL_TIMEx_MasterConfigSynchronization(&htim, &sMasterConfig);

	HAL_TIM_Base_Start(&htim);
}

void HAL_DAC_MspInit(DAC_HandleTypeDef* hdac) {
	GPIO_InitTypeDef GPIO_InitStruct;
	static DMA_HandleTypeDef hdma_dac1;

	/*##-1- Enable peripherals and GPIO Clocks #################################*/
	__HAL_RCC_DAC_CLK_ENABLE();
	DACx_CHANNEL1_GPIO_CLK_ENABLE();
	/* DMA1 clock enable */
	DMAx_CLK_ENABLE();

	/* DAC Channel1 GPIO pin configuration */
	GPIO_InitStruct.Pin = DACx_CHANNEL1_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(DACx_CHANNEL1_GPIO_PORT, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = DACx_CHANNEL2_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(DACx_CHANNEL2_GPIO_PORT, &GPIO_InitStruct);


	/*##-3- Configure the DMA streams ##########################################*/
	hdma_dac1.Instance = DACx_DMA_STREAM1;
	hdma_dac1.Init.Channel = DACx_DMA_CHANNEL1;
	hdma_dac1.Init.Direction = DMA_MEMORY_TO_PERIPH;
	hdma_dac1.Init.PeriphInc = DMA_PINC_DISABLE;
	hdma_dac1.Init.MemInc = DMA_MINC_ENABLE;
#if 0
	hdma_dac1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
	hdma_dac1.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
#else
	hdma_dac1.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
	hdma_dac1.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
#endif
	hdma_dac1.Init.Mode = DMA_CIRCULAR;
	hdma_dac1.Init.Priority = DMA_PRIORITY_HIGH;
	hdma_dac1.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
	hdma_dac1.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_HALFFULL;
	hdma_dac1.Init.MemBurst = DMA_MBURST_SINGLE;
	hdma_dac1.Init.PeriphBurst = DMA_PBURST_SINGLE;

	HAL_DMA_Init(&hdma_dac1);

	/* Associate the initialized DMA handle to the the DAC handle */
	__HAL_LINKDMA(hdac, DMA_Handle1, hdma_dac1);

	/*##-4- Configure the NVIC for DMA #########################################*/
	/* Enable the DMA1 Stream5 IRQ Channel */
	HAL_NVIC_SetPriority(DACx_DMA_IRQn1, 2, 0);
	HAL_NVIC_EnableIRQ(DACx_DMA_IRQn1);
}

/** @brief Tx Transfer completed callbacks. */
void HAL_DAC_ConvCpltCallbackCh1(DAC_HandleTypeDef* hdac)
{
	//TRACE(("F\n"));
	samples_ptr = dacSampleTable + ARRAY_SIZE(dacSampleTable)/2;
}

/** @brief Tx Transfer Half completed callbacks */
void HAL_DAC_ConvHalfCpltCallbackCh1(DAC_HandleTypeDef* hdac)
{
	//TRACE(("H\n"));
	samples_ptr = dacSampleTable;
}

void HAL_DAC_ErrorCallbackCh1(DAC_HandleTypeDef *hdac) {
	MSG(("ErrorCallbackCh1!\n"));
}

void HAL_DAC_DMAUnderrunCallbackCh1(DAC_HandleTypeDef *hdac) {
	MSG(("UnderrunCallbackCh1!\n"));
}

void HAL_DAC_MspDeInit(DAC_HandleTypeDef* hdac) {
	static DMA_HandleTypeDef hdma_dac1;

	/*##-1- Reset peripherals ##################################################*/
	DACx_FORCE_RESET();
	DACx_RELEASE_RESET();

	/*##-2- Disable peripherals and GPIO Clocks ################################*/
	/* De-initialize the DAC Channel1 GPIO pin */
	HAL_GPIO_DeInit(DACx_CHANNEL1_GPIO_PORT, DACx_CHANNEL1_PIN);
	HAL_GPIO_DeInit(DACx_CHANNEL2_GPIO_PORT, DACx_CHANNEL2_PIN);

	/*##-3- Disable the DMA Streams ############################################*/
	/* De-Initialize the DMA Stream associate to Channel1 */
	hdma_dac1.Instance = DACx_DMA_STREAM1;
	HAL_DMA_DeInit(&hdma_dac1);

	/*##-4- Disable the NVIC for DMA ###########################################*/
	HAL_NVIC_DisableIRQ(DACx_DMA_IRQn1);
}

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef* htim) {
	/* TIM6 Periph clock enable */
	__HAL_RCC_TIM6_CLK_ENABLE();
}

/**
 * @brief TIM MSP De-Initialization
 *        This function frees the hardware resources used in this example:
 *          - Disable the Peripheral's clock
 *          - Revert GPIO to their default state
 * @param htim: TIM handle pointer
 */
void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef *htim) {
	__HAL_RCC_TIM6_FORCE_RESET();
	__HAL_RCC_TIM6_RELEASE_RESET();
}

/******************************************************************************/
/*                 STM32F4xx Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (DAC), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f4xx.s).                                               */
/******************************************************************************/

/** @brief  This function handles DMA interrupt request. */
void DACx_DMA_IRQHandler1(void) {
	HAL_DMA_IRQHandler(DacHandle.DMA_Handle1);
}

/** @brief  This function handles DMA interrupt request. */
void DACx_DMA_IRQHandler2(void) {
	HAL_DMA_IRQHandler(DacHandle.DMA_Handle2);
}

