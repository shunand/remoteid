#ifndef ___HAL_H__
#define ___HAL_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef uint8_t hal_id_t;

#ifdef __cplusplus
}
#endif

#include "drivers/chip/gpio.h"
#include "drivers/chip/clk.h"
#include "drivers/chip/irq.h"
#include "drivers/chip/uart.h"
#include "drivers/chip/i2c.h"
#include "drivers/chip/spi.h"
#include "drivers/chip/phy.h"
#include "drivers/chip/adc.h"
#include "drivers/chip/cmp.h"
#include "drivers/chip/dac.h"
#include "drivers/chip/dma.h"
#include "drivers/chip/tim.h"
#include "drivers/chip/tick.h"
#include "drivers/chip/fmc.h"
#include "drivers/chip/wdt.h"
#include "drivers/chip/lpm.h"
#include "drivers/chip/misc.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct
{
    uint8_t pin;
    gpio_dir_t dir;
    gpio_pud_t pull;
} hal_pin_hdl_t;

typedef struct
{
    hal_pin_hdl_t pin_txd;
    hal_pin_hdl_t pin_rxd;
    hal_id_t id;
    uint8_t irq_prior;
    uint32_t br;
} hal_uart_hdl_t;

typedef struct
{
    hal_pin_hdl_t pin_sck;
    hal_pin_hdl_t pin_sda;
    hal_id_t id;
    uint8_t irq_prior;
    uint32_t freq;
} hal_i2c_hdl_t;

typedef struct
{
    hal_pin_hdl_t pin_sck;
    hal_pin_hdl_t pin_mosi;
    hal_pin_hdl_t pin_miso;
    hal_id_t id;
    spi_mode_t mode;
    uint8_t irq_prior;
    uint32_t freq;
} hal_spi_hdl_t;

typedef struct
{
    hal_id_t id;
    uint16_t vref_mv;
    uint32_t value_max;
    uint32_t channel_mask;
} hal_adc_hdl_t;

void drv_hal_init(void);
void drv_hal_deinit(void);

int drv_hal_gpio_pin_init(const hal_pin_hdl_t *gpio_hdl);
int drv_hal_uart_init(const hal_uart_hdl_t *uart_hdl);
int drv_hal_i2c_init(const hal_i2c_hdl_t *i2c_hdl);
int drv_hal_spi_init(const hal_spi_hdl_t *spi_hdl);
int drv_hal_drv_adc_init(const hal_adc_hdl_t *adc_hdl);

void drv_hal_debug_enable(void);
void drv_hal_debug_disable(void);

void drv_hal_sys_reset(void);
void drv_hal_sys_exit(int status);

#ifdef __cplusplus
}
#endif

#endif
