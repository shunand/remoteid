#ifndef __GPIO_H__
#define __GPIO_H__

#include <stdbool.h>
#include <stdint.h>

#ifndef __packed
#define __packed __attribute__((__packed__))
#endif

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum __packed
{
    _GPIO_DIR_IN,
    _GPIO_DIR_ANALOG,
    _GPIO_DIR_OUT,
    _GPIO_DIR_OPEN_DRAIN,
} gpio_dir_t;

typedef enum __packed
{
    _GPIO_PUD_NONE,
    _GPIO_PUD_PULL_UP,
    _GPIO_PUD_PULL_DOWN,
} gpio_pud_t;

typedef enum __packed
{
    _GPIO_EXTI_EDGE_NONE,
    _GPIO_EXTI_EDGE_FALL,
    _GPIO_EXTI_EDGE_RISE,
    _GPIO_EXTI_EDGE_BOTH,
} gpio_exti_edge_t;

typedef void (*gpio_exti_cb_fn)(void);

void drv_gpio_enable_all(void);
void drv_gpio_disable_all(void);

int drv_gpio_pin_configure(uint8_t pin, gpio_dir_t dir, gpio_pud_t pull);
int drv_gpio_pin_read(uint8_t pin);
void drv_gpio_pin_write(uint8_t pin, int value);

void drv_gpio_exti_callback_enable(gpio_exti_cb_fn cb);
void drv_gpio_exti_callback_disable(void);

void drv_gpio_exti_irq_enable(uint8_t pin, gpio_exti_edge_t polarity, int priority);
void drv_gpio_exti_irq_disable(uint8_t pin);

int drv_gpio_exti_get_pin(void);

#ifdef __cplusplus
}
#endif

#endif
