#ifndef __SPI_H__
#define __SPI_H__

#include "drivers/chip/_hal.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum __packed
{
    _SPI_MODE_0, // clock polarity is low level and phase is first edge
    _SPI_MODE_1, // clock polarity is low level and phase is second edge
    _SPI_MODE_2, // clock polarity is high level and phase is first edge
    _SPI_MODE_3, // clock polarity is high level and phase is second edge
} spi_mode_t;

typedef struct
{
    bool ms; // false: master, true: slave
    spi_mode_t mode;
    bool quad_mode;
    bool lsb_first;
    unsigned freq;
} spi_init_t;

typedef void (*spi_isr_cb_fn)(void);

void drv_spi_pin_configure_nss(hal_id_t id, uint8_t pin);
void drv_spi_pin_configure_sck(hal_id_t id, uint8_t pin);
void drv_spi_pin_configure_mosi(hal_id_t id, uint8_t pin);
void drv_spi_pin_configure_miso(hal_id_t id, uint8_t pin);
void drv_spi_pin_configure_io2(hal_id_t id, uint8_t pin);
void drv_spi_pin_configure_io3(hal_id_t id, uint8_t pin);

void drv_spi_enable(hal_id_t id);
void drv_spi_disable(hal_id_t id);

void drv_spi_init(hal_id_t id, const spi_init_t *param);
void drv_spi_deinit(hal_id_t id);

int drv_spi_poll_read(hal_id_t id, void *dest);
int drv_spi_poll_write(hal_id_t id, uint8_t data);

int drv_spi_irq_callback_enable(hal_id_t id, spi_isr_cb_fn cb);
int drv_spi_irq_callback_disable(hal_id_t id);

void drv_spi_irq_enable(hal_id_t id, bool rx, bool tx, int priority);
void drv_spi_irq_disable(hal_id_t id, bool rx, bool tx);

bool drv_spi_is_tx_ready(hal_id_t id);
bool drv_spi_is_rx_ready(hal_id_t id);

void drv_spi_dma_enable(hal_id_t id, bool tx_dma, bool rx_dma, unsigned prio_level);
void drv_spi_dma_disable(hal_id_t id, bool tx_dma, bool rx_dma);

int drv_spi_dma_irq_callback_enable(hal_id_t id, spi_isr_cb_fn cb);
int drv_spi_dma_irq_callback_disable(hal_id_t id);

void drv_spi_dma_irq_enable(hal_id_t id, int priority);
void drv_spi_dma_irq_disable(hal_id_t id);

int drv_spi_dma_set_read(hal_id_t id, void *dest, unsigned len);
int drv_spi_dma_set_write(hal_id_t id, const void *src, unsigned len);
void drv_spi_dma_start(hal_id_t id);

bool drv_spi_dma_is_busy(hal_id_t id);

#ifdef __cplusplus
}
#endif

#endif
