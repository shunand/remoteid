/**
 * @file sb_data_port.h
 * @author LokLiang
 * @brief 统一的数据接口定义
 * @version 0.1
 * @date 2023-11-14
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct sb_data_port_s sb_data_port_t;

typedef struct
{
    int (*start)(sb_data_port_t *port);                                  // 由具体驱动实现的，以达到节省资源为主要目的，启动数据接口
    int (*stop)(sb_data_port_t *port);                                   // 由具体驱动实现的，以达到节省资源为主要目的，关闭数据接口
    int (*write)(sb_data_port_t *port, const void *data, uint32_t size); // 由具体驱动实现的，写数据到对应的接口。
    int (*read)(sb_data_port_t *port, void *buffer, uint32_t length);    // 由具体驱动实现的，从数据接口中读取已缓存的数据。
    bool (*is_started)(sb_data_port_t *port);                            // 由具体驱动实现的，获取当前数据接口是否可用（是否已启动）
    uint32_t (*get_rx_length)(sb_data_port_t *port);                     // 由具体驱动实现的，获取当前数据接口的本次可读长度
} sb_data_port_vtable_t;

struct sb_data_port_s
{
    const sb_data_port_vtable_t *vtable; // 接口
    void *data;                          // 数据
};

int sb_data_port_start(sb_data_port_t *port);
int sb_data_port_stop(sb_data_port_t *port);

int sb_data_port_write(sb_data_port_t *port, const void *data, uint32_t size, uint32_t wait_ms);
int sb_data_port_read(sb_data_port_t *port, void *buffer, uint32_t length, uint32_t wait_ms);

bool sb_data_port_is_started(sb_data_port_t *port);
uint32_t sb_data_port_get_rx_length(sb_data_port_t *port);
