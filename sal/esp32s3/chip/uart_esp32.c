#include "drivers/chip/uart.h"

#include "driver/uart.h"

#define SYS_LOG_DOMAIN "uart"
#include "sys_log.h"

void drv_uart_pin_configure_txd(hal_id_t id, uint8_t pin)
{
    SYS_LOG_ERR("func not set");
}

void drv_uart_pin_configure_rxd(hal_id_t id, uint8_t pin)
{
    SYS_LOG_ERR("func not set");
}

void drv_uart_enable(hal_id_t id)
{
    SYS_LOG_ERR("func not set");
}

void drv_uart_disable(hal_id_t id)
{
    SYS_LOG_ERR("func not set");
}

void drv_uart_init(hal_id_t id, const uart_param_t *param)
{
    SYS_LOG_ERR("func not set");
}

void drv_uart_deinit(hal_id_t id)
{
    SYS_LOG_ERR("func not set");
}

void drv_uart_set_br(hal_id_t id, unsigned clk, unsigned baudrate)
{
    SYS_LOG_ERR("func not set");
}

unsigned drv_uart_get_br(hal_id_t id, unsigned clk)
{
    SYS_LOG_ERR("func not set");
    return -1;
}

int drv_uart_poll_read(hal_id_t id, void *data)
{
    return uart_read_bytes(id, data, 1, 0);
}

int drv_uart_poll_write(hal_id_t id, uint8_t data)
{
    SYS_LOG_ERR("func not set");
    return 0;
}

int drv_uart_fifo_read(hal_id_t id, void *data, int size)
{
    int ret = 0;
    for (int i = 0; i < size; i++)
    {
        if (drv_uart_poll_read(id, &((char *)data)[i]) <= 0)
        {
            break;
        }
        ++ret;
    }
    return 0;
}

int drv_uart_fifo_fill(hal_id_t id, const void *data, int size)
{
    for (unsigned i = 0; i < size; i++)
    {
        if (drv_uart_poll_write(id, ((char *)data)[i]) < 0)
        {
            return -1;
        }
    }
    return size;
}

int drv_uart_irq_callback_enable(hal_id_t id, uart_isr_cb_fn cb)
{
    SYS_LOG_ERR("func not set");
    return -1;
}

int drv_uart_irq_callback_disable(hal_id_t id)
{
    SYS_LOG_ERR("func not set");
    return -1;
}

void drv_uart_irq_tx_enable(hal_id_t id, int priority)
{
    SYS_LOG_ERR("func not set");
}

void drv_uart_irq_tx_disable(hal_id_t id)
{
    SYS_LOG_ERR("func not set");
}

void drv_uart_irq_rx_enable(hal_id_t id, int priority)
{
    SYS_LOG_ERR("func not set");
}

void drv_uart_irq_rx_disable(hal_id_t id)
{
    SYS_LOG_ERR("func not set");
}

bool drv_uart_wait_tx_busy(hal_id_t id)
{
    return false;
}

bool drv_uart_is_tx_ready(hal_id_t id)
{
    return true;
}

bool drv_uart_is_rx_ready(hal_id_t id)
{
    return true;
}
