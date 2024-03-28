/**
 * @file uart_port.h
 * @author LokLiang
 * @brief
 * @version 0.1
 * @date 2023-09-04
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include "../sb_data_port.h"
#include "os/os.h"

int sb_uart_port_init(void);

sb_data_port_t *sb_uart_port_bind(int uartNum,
                                  int baudrate,
                                  int tx_pin,
                                  int rx_pin,
                                  int rx_task_priority,
                                  uint16_t buffer_size,
                                  uint8_t frame_ms,
                                  os_work_t *rx_resume_work);

void sb_uart_port_unbind(sb_data_port_t *port);
