#include "app_main.h"

/* 标准库 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 配置 */
#include "config/board_config.h"

/* SDK */
#include "esp_err.h"

/* 通用模块 */
#include "button/button_event.h"

/* 应用模块 */

/* 自定义框架抽象层 */
#include "os/os.h"
#include "os/os_common.h"
#include "console.h"
#include "shell/sh_vset.h"

#define CONFIG_SYS_LOG_LEVEL SYS_LOG_LEVEL_DBG
#define SYS_LOG_DOMAIN "MAIN"
#include "sys_log.h"

nvs_handle g_nvs_hdl;         // nvs 句柄
os_work_q_t g_work_q_hdl_low; // 低于 default_os_work_q_hdl 优先级的工作队列

static void _init_nvs(void);
static int _change_mode_event_button(const button_event_t *event);
static void _vset_cb(sh_t *sh_hdl);

void work_app_main(void *arg)
{
    /* 初始化 SDK 的 nvs 模块 */
    _init_nvs();

    /* 初始化按键检测和控制 */
    button_init(PIN_BIT(g_cfg_board->key_boot.pin), g_cfg_board->key_boot.en_lev);

    button_event_add_callback(g_cfg_board->key_boot.pin, _change_mode_event_button);

    /* 启动 shell */
    SYS_LOG_INF("app start");
    vset_init(&g_uart_handle_vt100, _vset_cb);
    os_thread_sleep(100);
    shell_start();
}

static void _init_nvs(void)
{
#define _NVS_NAME "nvs-app"
    if (g_nvs_hdl == 0)
    {
        ESP_ERROR_CHECK(nvs_open(_NVS_NAME, NVS_READWRITE, &g_nvs_hdl));
    }
}

static int _change_mode_event_button(const button_event_t *event)
{
    static const char *const stat_tab[] = {
        [BUTTON_UP] = "up",
        [BUTTON_DOWN] = "down",
        [BUTTON_HELD] = "held",
    };
    if (event->event < __ARRAY_SIZE(stat_tab))
    {
        SYS_LOG_DBG("button stat: %s", stat_tab[event->event]);
    }

    return 0;
}

static void _vset_cb(sh_t *sh_hdl)
{
}
