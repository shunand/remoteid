#ifndef __ADC_H__
#define __ADC_H__

#include "drivers/chip/_hal.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef void (*adc_isr_cb_fn)(uint8_t channel_id);

void drv_adc_pin_configure(hal_id_t id, uint8_t pin);

void drv_adc_enable(hal_id_t id);
void drv_adc_disable(hal_id_t id);

void drv_adc_init(hal_id_t id, unsigned channel_mask, uint32_t sampling_length);
void drv_adc_deinit(hal_id_t id, unsigned channel_mask);

int drv_adc_irq_callback_enable(hal_id_t id, uint8_t channel_id, adc_isr_cb_fn cb);
int drv_adc_irq_callback_disable(hal_id_t id, uint8_t channel_id);

void drv_adc_irq_enable(hal_id_t id, int priority);
void drv_adc_irq_disable(hal_id_t id);

void drv_adc_start(hal_id_t id, unsigned channel_mask);
void drv_adc_stop(hal_id_t id, unsigned channel_mask);

bool drv_adc_is_busy(hal_id_t id, uint8_t channel_id);
bool drv_adc_is_ready(hal_id_t id, uint8_t channel_id);

int drv_adc_read(hal_id_t id, uint8_t channel_id);

#ifdef __cplusplus
}
#endif

#endif
