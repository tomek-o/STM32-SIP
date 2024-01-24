/** \file

Audio source: ADC1 triggered by TIM8, 12-bit resolution, using DMA.
Using PA0 as analog input pin.

*/


#include "adc.h"
#include "utils.h"
#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_ll_adc.h"
#include "stm32f4xx_ll_tim.h"
#include "stm32f4xx_ll_dma.h"
#include "stm32f4xx_ll_bus.h"
#include "stm32f4xx_ll_gpio.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <inttypes.h>


#define LOCAL_DEBUG
#ifdef LOCAL_DEBUG
#	define TRACE(args) (printf("DAC: "), printf args)
#else
#	define TRACE(args)
#endif
#	define MSG(args) (printf("DAC: "), printf args)

enum { ADC_SAMPLING_FREQUENCY = 64000 };    ///< oversampling: reducing some of the noise observed on NUCLEO-F429ZI
static int TODO__USE_HIGH_PASS_FILTER_TO_CUT_OFF_ADC_OFFSET;
enum { ADC_OFFSET = 32768 };

struct adc_client {
    adc_callback *adc_cb;
    unsigned int samples_count_ratio;   ///< assuming that requested frequency would be equal to 1/N of ADC native frequency or equal to ADC native frequency itself
    void *arg;
};

static struct adc_client clients[4] = {0};

int adc_client_register(adc_callback *adc_cb, unsigned int sampling_rate, void *arg) {
    assert(arg != NULL);
    if (ADC_SAMPLING_FREQUENCY % sampling_rate) {   // using trivial resampling by duplicating samples
        MSG(("Requested sampling rate = %u is not accepted for trivial resampling\n", sampling_rate));
        return -2;
    }
    for (unsigned int i=0; i<ARRAY_SIZE(clients); i++) {
        struct adc_client *c = &clients[i];
        if (c->arg == NULL) {
            c->arg = arg;
            c->samples_count_ratio = ADC_SAMPLING_FREQUENCY / sampling_rate;
            c->adc_cb = adc_cb;
            return 0;
        }
    }
    MSG(("Empty ADC client structure not found!\n"));
    return -1;
}

void adc_client_unregister(void *arg) {
    assert(arg != NULL);
    for (unsigned int i=0; i<ARRAY_SIZE(clients); i++) {
        struct adc_client *c = &clients[i];
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



static uint16_t adcVal[ADC_SAMPLING_FREQUENCY/25];
static volatile uint16_t* adcValLastPtr = NULL;

static int16_t callbackBuf[ADC_SAMPLING_FREQUENCY/50] = { 0 };


static void dma_init(void)
{
    /* DMA controller clock enable */
    __HAL_RCC_DMA2_CLK_ENABLE();

    /* DMA interrupt init */
    /* DMA2_Stream0_IRQn interrupt configuration */
    NVIC_SetPriority(DMA2_Stream0_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),0, 0));
    NVIC_EnableIRQ(DMA2_Stream0_IRQn);
}

static void timer_init(void)
{
    // TIMER8 -> APB2
    uint32_t timer8_clock = HAL_RCC_GetPCLK2Freq() * 2;
    if (timer8_clock % ADC_SAMPLING_FREQUENCY) {
        MSG(("\nTimer clock = %lu is not evenly divisable by ADC sampling frequency!\n", timer8_clock));
    }

    LL_TIM_InitTypeDef TIM_InitStruct = {0};

    /* Peripheral clock enable */
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_TIM8);

    TIM_InitStruct.Prescaler = 0;
    TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
    TIM_InitStruct.Autoreload = (timer8_clock/ADC_SAMPLING_FREQUENCY) - 1;
    TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
    TIM_InitStruct.RepetitionCounter = 0;
    LL_TIM_Init(TIM8, &TIM_InitStruct);
    LL_TIM_EnableARRPreload(TIM8);
    LL_TIM_SetClockSource(TIM8, LL_TIM_CLOCKSOURCE_INTERNAL);
    LL_TIM_SetTriggerOutput(TIM8, LL_TIM_TRGO_UPDATE);
    LL_TIM_DisableMasterSlaveMode(TIM8);

    LL_TIM_EnableCounter(TIM8);
}

void adc_init(void)
{
    dma_init();

    LL_ADC_InitTypeDef ADC_InitStruct = {0};
    LL_ADC_REG_InitTypeDef ADC_REG_InitStruct = {0};
    LL_ADC_CommonInitTypeDef ADC_CommonInitStruct = {0};

    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Peripheral clock enable */
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_ADC1);

    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
    /**ADC1 GPIO Configuration
    PA0/WKUP   ------> ADC1_IN0
    */
    GPIO_InitStruct.Pin = LL_GPIO_PIN_0;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* ADC1 DMA Init */

    /* ADC1 Init */
    LL_DMA_SetChannelSelection(DMA2, LL_DMA_STREAM_0, LL_DMA_CHANNEL_0);
    LL_DMA_SetDataTransferDirection(DMA2, LL_DMA_STREAM_0, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
    LL_DMA_SetStreamPriorityLevel(DMA2, LL_DMA_STREAM_0, LL_DMA_PRIORITY_LOW);
    LL_DMA_SetMode(DMA2, LL_DMA_STREAM_0, LL_DMA_MODE_CIRCULAR);
    LL_DMA_SetPeriphIncMode(DMA2, LL_DMA_STREAM_0, LL_DMA_PERIPH_NOINCREMENT);
    LL_DMA_SetMemoryIncMode(DMA2, LL_DMA_STREAM_0, LL_DMA_MEMORY_INCREMENT);
    LL_DMA_SetPeriphSize(DMA2, LL_DMA_STREAM_0, LL_DMA_PDATAALIGN_HALFWORD);
    LL_DMA_SetMemorySize(DMA2, LL_DMA_STREAM_0, LL_DMA_MDATAALIGN_HALFWORD);
    LL_DMA_DisableFifoMode(DMA2, LL_DMA_STREAM_0);

    /** Common config
    */
    ADC_InitStruct.Resolution = LL_ADC_RESOLUTION_12B;
    ADC_InitStruct.DataAlignment = LL_ADC_DATA_ALIGN_LEFT;  //LL_ADC_DATA_ALIGN_RIGHT;
    ADC_InitStruct.SequencersScanMode = LL_ADC_SEQ_SCAN_DISABLE;
    LL_ADC_Init(ADC1, &ADC_InitStruct);
    ADC_REG_InitStruct.TriggerSource = LL_ADC_REG_TRIG_EXT_TIM8_TRGO;
    ADC_REG_InitStruct.SequencerLength = LL_ADC_REG_SEQ_SCAN_DISABLE;
    ADC_REG_InitStruct.SequencerDiscont = LL_ADC_REG_SEQ_DISCONT_DISABLE;
    ADC_REG_InitStruct.ContinuousMode = LL_ADC_REG_CONV_SINGLE;
    ADC_REG_InitStruct.DMATransfer = LL_ADC_REG_DMA_TRANSFER_UNLIMITED;
    LL_ADC_REG_Init(ADC1, &ADC_REG_InitStruct);
    LL_ADC_REG_SetFlagEndOfConversion(ADC1, LL_ADC_REG_FLAG_EOC_UNITARY_CONV);
    LL_ADC_DisableIT_EOCS(ADC1);
    ADC_CommonInitStruct.CommonClock = LL_ADC_CLOCK_SYNC_PCLK_DIV4;
    ADC_CommonInitStruct.Multimode = LL_ADC_MULTI_INDEPENDENT;
    LL_ADC_CommonInit(__LL_ADC_COMMON_INSTANCE(ADC1), &ADC_CommonInitStruct);
    LL_ADC_REG_StartConversionExtTrig(ADC1, LL_ADC_REG_TRIG_EXT_RISING);
    /** Configure Regular Channel
    */
    LL_ADC_REG_SetSequencerRanks(ADC1, LL_ADC_REG_RANK_1, LL_ADC_CHANNEL_0);
    LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_0, LL_ADC_SAMPLINGTIME_144CYCLES);

    timer_init();

    // configure DMA source & target
    LL_DMA_ConfigAddresses(DMA2, LL_DMA_STREAM_0, LL_ADC_DMA_GetRegAddr(ADC1, LL_ADC_DMA_REG_REGULAR_DATA), (uint32_t)adcVal, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
    // configure DMA length
    LL_DMA_SetDataLength(DMA2, LL_DMA_STREAM_0, ARRAY_SIZE(adcVal));
    // enable DMA tranfer half + complete interrupt
    LL_DMA_EnableIT_HT(DMA2, LL_DMA_STREAM_0);
    LL_DMA_EnableIT_TC(DMA2, LL_DMA_STREAM_0);
    // enable DMA stream
    LL_DMA_EnableStream(DMA2, LL_DMA_STREAM_0);
    // enable ADC
    LL_ADC_Enable(ADC1);
    // start conversion
    LL_ADC_REG_StartConversionSWStart(ADC1);
}

static void process_samples(uint16_t *samples)
{
    //printf("%u at tick %" PRIu32 "\n", samples[0], HAL_GetTick());
    for (unsigned int i=0; i<ARRAY_SIZE(clients); i++) {
        struct adc_client *c = &clients[i];
        if (c->adc_cb) {
            const unsigned int ratio = c->samples_count_ratio;
            unsigned int samplesCount = 0;
            int sum = 0;
            unsigned int sumPos = 0;
            for (unsigned int sampleId = 0; (sampleId < ARRAY_SIZE(adcVal)/2); sampleId++) {
                sum += samples[sampleId];
                sumPos++;
                if (sumPos == ratio) {
                    callbackBuf[samplesCount++] = (sum / ratio) - ADC_OFFSET;
                    sum = 0;
                    sumPos = 0;
                }
            }
            c->adc_cb(callbackBuf, samplesCount, c->arg);
        }
    }
}

void adc_poll(void)
{
    static uint16_t* lastAdcProcessed = NULL;
    uint16_t* newAdc = (uint16_t*)adcValLastPtr;
    if (newAdc != lastAdcProcessed) {
        lastAdcProcessed = newAdc;
        // this must not be called in ISR context! assert when freeing memory
        process_samples(newAdc);
    }
}


void DMA2_Stream0_IRQHandler(void)
{
    if(LL_DMA_IsActiveFlag_TC0(DMA2)) {
        LL_DMA_ClearFlag_TC0(DMA2);
        adcValLastPtr = adcVal + (ARRAY_SIZE(adcVal) / 2);
    } else if (LL_DMA_IsActiveFlag_HT0(DMA2)) {
        LL_DMA_ClearFlag_HT0(DMA2);
        adcValLastPtr = adcVal;
    }
}
