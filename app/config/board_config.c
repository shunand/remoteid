#include "board_config.h"

#include "driver/uart.h"

#define CONFIG_SYS_LOG_LEVEL SYS_LOG_LEVEL_DBG
#define SYS_LOG_DOMAIN "CFG"
#define CONS_ABORT()
#include "sys_log.h"

static cfg_board_t const s_cfg_board_default = {
    /* 控制台串口 */
    .uart_console = {
        .pin_txd = {43, _GPIO_DIR_OUT, _GPIO_PUD_PULL_UP},
        .pin_rxd = {44, _GPIO_DIR_IN, _GPIO_PUD_PULL_UP},
        .id = UART_NUM_0,
        .irq_prior = 0,
        .br = 115200,
    },

    /* 启动按键 */
    .key_boot = {
        .pin = 9,    // 用于切换灯效
        .en_lev = 0, // 用于切换灯效
    },
};

const cfg_board_t *g_cfg_board = &s_cfg_board_default;
