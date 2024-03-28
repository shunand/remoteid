/**
 * @file board_config.h
 * @author LokLiang
 * @brief 统一板载硬件描述数据结构
 * @version 0.1
 * @date 2023-11-24
 *
 * @copyright Copyright (c) 2023
 *
 * 目的与作用场景：
 * 使同类型的固件适应不同的硬件版本。
 * 例如同一类型产品，即使某些功能引脚改动，依然可以根据配置文件正确引导程序，而不需要新增特定的固件版本，旧的产品依然得到长期的支持。
 *
 * 数据设置：
 * 只能在工厂中配置，除此之外不允许任何手段尝试修改。
 * 数据地址固定为分区配置中
 *
 */

#pragma once

#include "drivers/chip/_hal.h"

typedef struct // 按键配置
{
    uint8_t pin;    // 引脚号
    uint8_t en_lev; // 触发电平
} cfg_board_key_t;

typedef struct // 数据结构一旦定下不可随意变更
{
    /* 硬件描述类 */
    hal_uart_hdl_t uart_console; // 控制台
    cfg_board_key_t key_boot;    // 启动按键

    /* 产品功能描述类 */

} cfg_board_t;

extern const cfg_board_t *g_cfg_board; // 配置数据
