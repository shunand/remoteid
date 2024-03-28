#ifndef __DAC_H__
#define __DAC_H__

#include "drivers/chip/_hal.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef void (*dac_isr_cb_fn)(void);

void drv_dac_pin_configure(hal_id_t id, uint8_t pin);

void drv_dac_enable(hal_id_t id);
void drv_dac_disable(hal_id_t id);

int drv_dac_init(hal_id_t id, unsigned channel_mask, unsigned bit_width);
int drv_dac_deinit(hal_id_t id, unsigned channel_mask);

int drv_dac_irq_callback_enable(hal_id_t id, dac_isr_cb_fn cb);
int drv_dac_irq_callback_disable(hal_id_t id);

void drv_dac_irq_enable(hal_id_t id, int priority);
void drv_dac_irq_disable(hal_id_t id);

void drv_dac_write(hal_id_t id, unsigned channel_id, int value);

#ifdef __cplusplus
}
#endif

#endif
