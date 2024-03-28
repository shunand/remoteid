#ifndef __UART_H__
#define __UART_H__

#include "drivers/chip/_hal.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct
{
    uint32_t br;
    uint8_t length;    /* 7..9 */
    uint8_t stop_bit;  /* 1 or 2 */
    uint8_t parity;    /* 0: none 1: even  2: odd */
    uint8_t flow_ctrl; /* 0: none 1: RTS 2: CTS 3: RTS+CTS */

} uart_param_t;

typedef void (*uart_isr_cb_fn)(void);

void drv_uart_pin_configure_txd(hal_id_t id, uint8_t pin);
void drv_uart_pin_configure_rxd(hal_id_t id, uint8_t pin);

void drv_uart_enable(hal_id_t id);
void drv_uart_disable(hal_id_t id);

void drv_uart_init(hal_id_t id, const uart_param_t *param);
void drv_uart_deinit(hal_id_t id);

void drv_uart_set_br(hal_id_t id, unsigned clk, unsigned baudrate);
unsigned drv_uart_get_br(hal_id_t id, unsigned clk);

int drv_uart_poll_read(hal_id_t id, void *data);
int drv_uart_poll_write(hal_id_t id, uint8_t data);

int drv_uart_fifo_read(hal_id_t id, void *data, int size);
int drv_uart_fifo_fill(hal_id_t id, const void *data, int size);

int drv_uart_irq_callback_enable(hal_id_t id, uart_isr_cb_fn cb);
int drv_uart_irq_callback_disable(hal_id_t id);

void drv_uart_irq_enable(hal_id_t id, bool rx, bool tx, int priority);
void drv_uart_irq_disable(hal_id_t id, bool rx, bool tx);

bool drv_uart_wait_tx_busy(hal_id_t id);

bool drv_uart_is_tx_ready(hal_id_t id);
bool drv_uart_is_rx_ready(hal_id_t id);

#ifdef __cplusplus
}
#endif

#endif
