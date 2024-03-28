#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "os/os.h"

#include "uart_port.h"

#include "driver/uart.h"

#include "drivers/data_port/sb_data_port.h"

#define CONFIG_SYS_LOG_DUMP_ON 0
#define CONFIG_SYS_LOG_LEVEL SYS_LOG_LEVEL_WRN
#define SYS_LOG_DOMAIN "UART"
#include "sys_log.h"

#define _MAX_UART_NUM 3

#define UART_IN_TAST_STK_SIZE 2048

typedef struct
{
    uint16_t size;
    uint8_t data[0];
} __fifo_data_t;

typedef struct
{
    int uart_num;
    int baudrate;
    int tx_pin;
    int rx_pin;
    int rx_task_priority;
    uint16_t buffer_size;
    uint8_t frame_ms;
    volatile uint16_t buffer_used;
    os_work_t *rx_resume_work;
    QueueHandle_t event_queue;
    os_thread_t rx_thread_hdl;
    union
    {
        os_pipe_t rx_pipe;
        os_fifo_t rx_fifo;
    };
} __uart_data_t;

static void _uart_rx_thread(void *parmas)
{
    uart_event_t event;
    int ret = 0;
    int len = 0;
    int rx_len = 0;
    __uart_data_t *uart_data = ((sb_data_port_t *)parmas)->data;
    const uint32_t TMP_BUF_LEN = 1024;
    uint8_t tmpbuffer[TMP_BUF_LEN];
    TickType_t tick = portMAX_DELAY;
    uint16_t recv_total = 0;
    os_time_t rx_time = 0;

    for (;;)
    {
        if (xQueueReceive(uart_data->event_queue, (void *)&event, tick))
        {
            switch (event.type)
            {
            case UART_DATA:
            {
                rx_len = event.size;
                while (rx_len > 0)
                {
                    if (uart_data->frame_ms == 0)
                    {
                        len = (rx_len > TMP_BUF_LEN) ? TMP_BUF_LEN : rx_len;
                        ret = uart_read_bytes(uart_data->uart_num, tmpbuffer, len, portMAX_DELAY);
                        if (ret <= 0)
                        {
                            break;
                        }
                        rx_len -= ret;

                        // 将数据写入ringbuffer，由外部轮询ringbuffer
                        uint8_t *p = tmpbuffer;
                        for (int i = 0; i < 100 && ret != 0; i++)
                        {
                            int write_result = os_pipe_fifo_fill(&uart_data->rx_pipe, p, ret);
                            p = &((uint8_t *)p)[write_result];
                            ret -= write_result;
                            if (ret != 0)
                            {
                                os_thread_sleep(10);
                            }
                        }
                        if (ret != 0)
                        {
                            SYS_LOG_ERR("write data to rx_pipe_obj failed, remain: %d bytes", ret);
                        }
                    }
                    else
                    {
                        len = sizeof(tmpbuffer) - recv_total;
                        if (len > rx_len)
                        {
                            len = rx_len;
                        }
                        ret = uart_read_bytes(uart_data->uart_num, &tmpbuffer[recv_total], len, portMAX_DELAY);
                        if (ret <= 0)
                        {
                            break;
                        }

                        rx_len -= ret;
                        recv_total += ret;
                        if (recv_total >= sizeof(tmpbuffer))
                        {
                            rx_time = os_get_sys_time() - uart_data->frame_ms - 1; // 使退出 switch 后的超时条件必定成立而立即保存数据
                            break;
                        }
                        rx_time = os_get_sys_time();
                    }
                }
            }
            break;

            // Event of HW FIFO overflow detected
            case UART_FIFO_OVF:
                SYS_LOG_WRN("hw fifo overflow");
                // If fifo overflow happened, you should consider adding flow control for your application.
                // The ISR has already reset the rx FIFO,
                uart_flush_input(uart_data->uart_num);
                xQueueReset(uart_data->event_queue);
                break;
            // Event of UART ring buffer full
            case UART_BUFFER_FULL:
                SYS_LOG_WRN("ring buffer full");
                // If buffer full happened, you should consider encreasing your buffer size
                uart_flush_input(uart_data->uart_num);
                xQueueReset(uart_data->event_queue);
                break;
            // Event of UART RX break detected
            case UART_BREAK:
                SYS_LOG_WRN("uart rx break");
                break;
            // Event of UART parity check error
            case UART_PARITY_ERR:
                SYS_LOG_WRN("uart parity error");
                break;
            // Event of UART frame error
            case UART_FRAME_ERR:
                SYS_LOG_WRN("uart frame error");
                break;
            // Others
            default:
                SYS_LOG_WRN("uart event type: %d", event.type);
                break;
            }
        }

        if (uart_data->frame_ms != 0)
        {
            if (os_get_sys_time() - rx_time > uart_data->frame_ms)
            {
                if (recv_total > 0)
                {
                    while (uart_data->buffer_used >= uart_data->buffer_size)
                    {
                        os_thread_sleep(1);
                    }

                    __fifo_data_t *fifo_data = os_fifo_alloc(sizeof(__fifo_data_t) + recv_total);
                    if (fifo_data)
                    {
                        os_scheduler_suspend();
                        uart_data->buffer_used += recv_total + sizeof(((__fifo_data_t *)0)->size);
                        os_scheduler_resume();

                        fifo_data->size = recv_total;
                        memcpy(fifo_data->data, tmpbuffer, recv_total);
                        os_fifo_put(&uart_data->rx_fifo, fifo_data);
                    }

                    recv_total = 0;
                    tick = 1;
                }
                else
                {
                    tick = portMAX_DELAY;
                }
            }
            else
            {
                tick = 1;
            }
        }
    }
}

static void uart_port_initialize(sb_data_port_t *port)
{
    __uart_data_t *uart_data = port->data;
    uart_config_t uart_config;

    memset(&uart_config, 0, sizeof(uart_config));
    uart_config.baud_rate = uart_data->baudrate;
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.parity = UART_PARITY_DISABLE;
    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;

    esp_err_t ret = uart_set_pin(uart_data->uart_num, uart_data->tx_pin, uart_data->rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK)
    {
        SYS_LOG_ERR("failed to set uart pin:%d, %d", uart_data->uart_num, ret);
        return;
    }

    ret = uart_param_config(uart_data->uart_num, &uart_config);
    if (ret != ESP_OK)
    {
        SYS_LOG_ERR("failed to set uart param:%d, %d", uart_data->uart_num, ret);
        return;
    }

    ret = uart_driver_install(uart_data->uart_num, 0x400, 0x400, 3, &(uart_data->event_queue), 0);
    if (ret != ESP_OK)
    {
        SYS_LOG_ERR("failed to uart_driver_install:%d, %d", uart_data->uart_num, ret);
        return;
    }

    SYS_LOG_INF("uart_driver_install ok. (%d, %d, %d, %d, %d, %d)", uart_data->uart_num, 0x400, 0x400, 3, (uint32_t)&uart_data->event_queue, 0);
}

static int _uart_port_start(sb_data_port_t *port)
{
    __uart_data_t *uart_data = port->data;

    SYS_LOG_INF("uart start, uartnum:%d", uart_data->uart_num);

    if (uart_data->frame_ms == 0)
    {
        if (!os_pipe_is_valid(&uart_data->rx_pipe))
        {
            if (os_pipe_create(&uart_data->rx_pipe, uart_data->buffer_size) != OS_OK)
            {
                port->vtable->stop(port);
                return -1;
            }
            os_pipe_regist(&uart_data->rx_pipe, uart_data->rx_resume_work, 0);
        }
    }
    else
    {
        if (!os_fifo_q_is_valid(&uart_data->rx_fifo))
        {
            if (os_fifo_q_create(&uart_data->rx_fifo) != OS_OK)
            {
                port->vtable->stop(port);
                return -1;
            }
            os_fifo_q_regist(&uart_data->rx_fifo, uart_data->rx_resume_work, 0);
        }
        uart_data->buffer_used = 0;
    }

    if (!uart_is_driver_installed(uart_data->uart_num))
    {
        uart_port_initialize(port);
    }

    if (!os_thread_is_valid(&uart_data->rx_thread_hdl))
    {
        if (os_thread_create(&uart_data->rx_thread_hdl,
                             "uart_rx",
                             _uart_rx_thread,
                             port,
                             UART_IN_TAST_STK_SIZE,
                             uart_data->rx_task_priority) != OS_OK)
        {
            SYS_LOG_WRN("task create fail");
            port->vtable->stop(port);
            return -1;
        }
    }

    return 0;
}

static int _uart_port_stop(sb_data_port_t *port)
{
    __uart_data_t *uart_data = port->data;

    if (os_thread_is_valid(&uart_data->rx_thread_hdl))
    {
        os_thread_delete(&uart_data->rx_thread_hdl);
    }

    if (uart_is_driver_installed(uart_data->uart_num))
    {
        uart_driver_delete(uart_data->uart_num);
    }

    if (uart_data->frame_ms == 0)
    {
        if (os_pipe_is_valid(&uart_data->rx_pipe))
        {
            os_pipe_delete(&uart_data->rx_pipe);
        }
    }
    else
    {
        if (os_fifo_q_is_valid(&uart_data->rx_fifo))
        {
            os_fifo_q_delete(&uart_data->rx_fifo);
        }
    }

    return 0;
}

static int _uart_port_write(sb_data_port_t *port, const void *data, uint32_t size)
{
    __uart_data_t *uart_data = port->data;
    int ret = uart_write_bytes(uart_data->uart_num, data, size);
    if (ret > 0)
    {
        SYS_LOG_DUMP(data, ret, 0, 0);
    }
    return ret;
}

static int _uart_port_read(sb_data_port_t *port, void *data, uint32_t size)
{
    __uart_data_t *uart_data = port->data;
    int ret;
    if (uart_data->frame_ms == 0)
    {
        ret = os_pipe_fifo_read(&uart_data->rx_pipe, data, size);
    }
    else
    {
        __fifo_data_t *fifo_data = os_fifo_take(&uart_data->rx_fifo, 0);
        if (fifo_data)
        {
            ret = size < fifo_data->size ? size : fifo_data->size;
            if (data)
            {
                memcpy(data, fifo_data->data, ret);
            }
            os_fifo_free(fifo_data);

            os_scheduler_suspend();
            uart_data->buffer_used -= ret + sizeof(((__fifo_data_t *)0)->size);
            os_scheduler_resume();
        }
        else
        {
            ret = 0;
        }
    }
    if (ret > 0)
    {
        SYS_LOG_DUMP(data, ret, 0, 0);
    }
    return ret;
}

static bool _uart_port_is_tart(sb_data_port_t *port)
{
    if (port == NULL)
    {
        return false;
    }
    __uart_data_t *uart_data = port->data;
    if (uart_data == NULL)
    {
        return false;
    }

    if (uart_data->frame_ms == 0)
    {
        if (!os_pipe_is_valid(&uart_data->rx_pipe))
        {
            return false;
        }
    }
    else
    {
        if (!os_fifo_q_is_valid(&uart_data->rx_fifo))
        {
            return false;
        }
    }

    return true;
}

static uint32_t _uart_port_get_rx_length(sb_data_port_t *port)
{
    __uart_data_t *uart_data = port->data;
    if (uart_data->frame_ms == 0)
    {
        return os_pipe_get_valid_size(&uart_data->rx_pipe);
    }
    else
    {
        __fifo_data_t *fifo_data = os_fifo_peek_head(&uart_data->rx_fifo);
        if (fifo_data)
        {
            return fifo_data->size;
        }
        else
        {
            return 0;
        }
    }
}

static sb_data_port_vtable_t const s_uart_vtable = {
    .start = _uart_port_start,
    .stop = _uart_port_stop,
    .write = _uart_port_write,
    .read = _uart_port_read,
    .is_started = _uart_port_is_tart,
    .get_rx_length = _uart_port_get_rx_length,
};

int sb_uart_port_init(void)
{
    return 0;
}

/**
 * @brief
 *
 * @param uart_num          串口号 0..n
 * @param baudrate          波特率
 * @param tx_pin            TXD 引脚
 * @param rx_pin            RXD 引脚
 * @param rx_task_priority  内部接收任务的优先级
 * @param buffer_size       接收缓存大小
 * @param frame_ms          0: 数据流， > 0: 最少连续有 n 毫秒没有收到数据时成为一帧
 * @param rx_resume_work    当收到新数据时唤醒的工作项，可设置为 NULL
 * @return sb_data_port_t*
 */
sb_data_port_t *sb_uart_port_bind(int uart_num,
                                  int baudrate,
                                  int tx_pin,
                                  int rx_pin,
                                  int rx_task_priority,
                                  uint16_t buffer_size,
                                  uint8_t frame_ms,
                                  os_work_t *rx_resume_work)
{
    static sb_data_port_t s_uart_port[_MAX_UART_NUM];
    static __uart_data_t s_uart_data[_MAX_UART_NUM];

    sb_data_port_t *port = &s_uart_port[uart_num];
    __uart_data_t *uart_data = &s_uart_data[uart_num];

    SYS_ASSERT(uart_num < _MAX_UART_NUM, "");
    SYS_ASSERT(port->data == NULL, "The interface has already been bound");

    uart_data->uart_num = uart_num;
    uart_data->baudrate = baudrate;
    uart_data->tx_pin = tx_pin;
    uart_data->rx_pin = rx_pin;
    uart_data->rx_task_priority = rx_task_priority;
    uart_data->buffer_size = buffer_size;
    uart_data->frame_ms = frame_ms;
    uart_data->rx_resume_work = rx_resume_work;

    port->vtable = &s_uart_vtable;
    port->data = uart_data;

    return port;
}

void sb_uart_port_unbind(sb_data_port_t *port)
{
    if (_uart_port_is_tart(port))
    {
        _uart_port_stop(port);
    }
    port->data = NULL;
}
