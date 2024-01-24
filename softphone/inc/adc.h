#ifndef ADC_H
#define ADC_H

#include <stdint.h>

void adc_init(void);

typedef void (adc_callback)(const int16_t *samples, unsigned int samples_count, void *arg);
int adc_client_register(adc_callback *adc_cb, unsigned int sampling_rate, void *arg);
void adc_client_unregister(void *arg);

void adc_poll(void);

#endif
