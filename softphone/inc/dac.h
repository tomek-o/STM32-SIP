#ifndef DAC_H_INCLUDED
#define DAC_H_INCLUDED

#include <stdint.h>

void dac_init(void);

typedef void (dac_callback)(int16_t *samples, unsigned int samples_count, void *arg);
int dac_client_register(dac_callback *dac_cb, unsigned int sampling_rate, void *arg);
void dac_client_unregister(void *arg);

void dac_poll(void);

#endif
