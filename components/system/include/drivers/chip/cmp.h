#ifndef __HAL_CMP_H__
#define __HAL_CMP_H__

#include "drivers/chip/_hal.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef void (*cmp_isr_cb_fn)(void);

typedef enum __packed
{
    _CMP_OUTPUT_NONE, // 没有任何输出动作
    _CMP_OUTPUT_IO,   // 输出到 IO
    _CMP_OUTPUT_EXTI, // 触发 中断线
    _CMP_OUTPUT_TIM,  // 触发 定时器的输入
} cmp_output_t;

typedef enum __packed
{
    _CMP_INT_EDGE_NONE,
    _CMP_INT_EDGE_FALL,
    _CMP_INT_EDGE_RISE,
    _CMP_INT_EDGE_BOTH,
} cmp_int_edge_t;

void drv_cmp_pin_configure_in(hal_id_t id, uint8_t pin);
void drv_cmp_pin_configure_out(hal_id_t id, uint8_t pin);

void drv_cmp_enable(hal_id_t id);
void drv_cmp_disable(hal_id_t id);

void drv_cmp_init(hal_id_t id, cmp_output_t output);
void drv_cmp_deinit(hal_id_t id);

int drv_cmp_get_out_value(hal_id_t id);

int drv_cmp_irq_callback_enable(hal_id_t id, cmp_isr_cb_fn cb);
int drv_cmp_irq_callback_disable(hal_id_t id);

void drv_cmp_irq_enable(hal_id_t id, cmp_int_edge_t edge, int priority);
void drv_cmp_irq_disable(hal_id_t id);

void drv_cmp_start(hal_id_t id);
void drv_cmp_stop(hal_id_t id);

void drv_cmp_set_cmp_value(hal_id_t id, uint16_t value);

uint8_t drv_cmp_read(hal_id_t id);

#ifdef __cplusplus
}
#endif

#endif
