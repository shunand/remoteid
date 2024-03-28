#ifndef __TIM_H__
#define __TIM_H__

#include "drivers/chip/_hal.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @param channel_id: 0..n
 * @param channel_mask: 1 << 0, 1 << 1, ... 1 << n
 */

typedef void (*tim_isr_cb_fn)(void);

/** @defgroup time base
 * @{
 */

void drv_tim_enable(hal_id_t id);
void drv_tim_disable(hal_id_t id);

void drv_tim_init(hal_id_t id, unsigned period_ns, unsigned reload_enable);
void drv_tim_deinit(hal_id_t id);

int drv_tim_irq_callback_enable(hal_id_t id, tim_isr_cb_fn cb);
int drv_tim_irq_callback_disable(hal_id_t id);

void drv_tim_irq_enable(hal_id_t id, int priority);
void drv_tim_irq_disable(hal_id_t id);

void drv_tim_base_start(hal_id_t id);
void drv_tim_base_stop(hal_id_t id);

unsigned drv_tim_get_period_ns(hal_id_t id);
unsigned drv_tim_get_cost_ns(hal_id_t id);
unsigned drv_tim_get_remain_ns(hal_id_t id);

/**
 * @}
 */

/** @defgroup PWM
 * @{
 */

void drv_pwm_pin_configure(hal_id_t id, uint8_t pin);

void drv_pwm_enable(hal_id_t id);
void drv_pwm_disable(hal_id_t id);

void drv_pwm_init(hal_id_t id, unsigned period_ns, unsigned reload_enable);
void drv_pwm_deinit(hal_id_t id);

int drv_pwm_irq_callback_enable(hal_id_t id, tim_isr_cb_fn cb);
int drv_pwm_irq_callback_disable(hal_id_t id);

void drv_pwm_irq_enable(hal_id_t id, unsigned channel_mask, int priority);
void drv_pwm_irq_disable(hal_id_t id, unsigned channel_mask);

void drv_pwm_start(hal_id_t id, unsigned channel_mask, unsigned active_level, unsigned pulse_ns);
void drv_pwm_stop(hal_id_t id, unsigned channel_mask, unsigned active_level);

bool drv_pwm_is_busy(hal_id_t id);

/**
 * @}
 */

/** @defgroup one pluse
 * @{
 */

void drv_pluse_pin_configure(hal_id_t id, uint8_t pin);

void drv_pluse_enable(hal_id_t id);
void drv_pluse_disable(hal_id_t id);

void drv_pluse_init(hal_id_t id);
void drv_pluse_deinit(hal_id_t id);

int drv_pluse_irq_callback_enable(hal_id_t id, tim_isr_cb_fn cb);
int drv_pluse_irq_callback_disable(hal_id_t id);

void drv_pluse_irq_enable(hal_id_t id, unsigned channel_mask, int priority);
void drv_pluse_irq_disable(hal_id_t id, unsigned channel_mask);

void drv_pluse_set(hal_id_t id, unsigned channel_mask, unsigned active_level, unsigned period_ns, unsigned pulse_ns);

bool drv_pluse_is_busy(hal_id_t id, unsigned channel);

/**
 * @}
 */

/** @defgroup input capture
 * @{
 */

typedef enum __packed
{
    _IC_EDGE_RISING,
    _IC_EDGE_FALLING,
    _IC_EDGE_BOTH,
} capture_edge_t;

void drv_capture_pin_configure(hal_id_t id, uint8_t pin);

void drv_capture_enable(hal_id_t id);
void drv_capture_disable(hal_id_t id);

void drv_capture_init(hal_id_t id, unsigned channel_mask, capture_edge_t polarity);
void drv_capture_deinit(hal_id_t id);

int drv_capture_irq_callback_enable(hal_id_t id, tim_isr_cb_fn cb);
int drv_capture_irq_callback_disable(hal_id_t id);

void drv_capture_irq_enable(hal_id_t id, unsigned channel_mask, int priority);
void drv_capture_irq_disable(hal_id_t id, unsigned channel_mask);

void drv_capture_start(hal_id_t id, unsigned channel_mask);
void drv_capture_stop(hal_id_t id, unsigned channel_mask);

unsigned drv_capture_get_cap_value(hal_id_t id, uint8_t channel_id);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
