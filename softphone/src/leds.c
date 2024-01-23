#include "leds.h"
#include "stm32f4xx_ll_gpio.h"
#include "stm32f4xx_ll_bus.h"
#include <assert.h>

static bool initialized = false;


/** all 3 LEDs on PORTB */
static const unsigned int LD1_Pin = LL_GPIO_PIN_0;
static const unsigned int LD2_Pin = LL_GPIO_PIN_7;
static const unsigned int LD3_Pin = LL_GPIO_PIN_14;


static void init(void)
{
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);
    LL_GPIO_ResetOutputPin(GPIOB, LD1_Pin|LD3_Pin|LD2_Pin);

    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = LD1_Pin|LD3_Pin|LD2_Pin;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    initialized = true;
}

void led_set_state(enum led_id id, bool state)
{
    if (!initialized)
        init();

    unsigned int pin;

    switch(id) {
    case LED1_GREEN:
        pin = LD1_Pin;
        break;
    case LED2_BLUE:
        pin = LD2_Pin;
        break;
    case LED3_RED:
        pin = LD3_Pin;
        break;
    default:
        assert(!"Unhandled LED!");
        return;
    }

    if (state) {
        LL_GPIO_SetOutputPin(GPIOB, pin);
    } else {
        LL_GPIO_ResetOutputPin(GPIOB, pin);
    }
}

void led_toggle(enum led_id id)
{
    if (!initialized)
        init();

    unsigned int pin;

    switch(id) {
    case LED1_GREEN:
        pin = LD1_Pin;
        break;
    case LED2_BLUE:
        pin = LD2_Pin;
        break;
    case LED3_RED:
        pin = LD3_Pin;
        break;
    default:
        assert(!"Unhandled LED!");
        return;
    }

    LL_GPIO_TogglePin(GPIOB, pin);
}

