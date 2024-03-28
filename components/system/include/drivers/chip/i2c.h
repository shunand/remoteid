#ifndef __I2C_H__
#define __I2C_H__

#include "drivers/chip/_hal.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef void (*i2c_isr_cb_fn)(void);

void drv_i2c_pin_configure_sck(hal_id_t id, uint8_t pin);
void drv_i2c_pin_configure_sda(hal_id_t id, uint8_t pin);

void drv_i2c_enable(hal_id_t id);
void drv_i2c_disable(hal_id_t id);

void drv_i2c_init(hal_id_t id, uint32_t freq);
void drv_i2c_deinit(hal_id_t id);

int drv_i2c_write(hal_id_t id, char dev_addr, const void *src, unsigned count);
int drv_i2c_read(hal_id_t id, void *dest, char dev_addr, unsigned count);

int drv_i2c_irq_callback_enable(hal_id_t id, i2c_isr_cb_fn cb);
int drv_i2c_irq_callback_disable(hal_id_t id);

void drv_i2c_irq_enable(hal_id_t id, int priority);
void drv_i2c_irq_disable(hal_id_t id);

bool drv_i2c_is_busy(hal_id_t id);

#ifdef __cplusplus
}
#endif

#endif
