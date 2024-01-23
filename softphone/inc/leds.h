#ifndef LEDS_H
#define LEDS_H

#include <stdbool.h>

/** Leds from NUCLEO-F429ZI board */

enum led_id {
    LED1_GREEN = 0,
    LED2_BLUE,
    LED3_RED
};

void led_set_state(enum led_id id, bool state);
void led_toggle(enum led_id id);

#endif
