/**
 * @file main.c
 * @author LokLiang
 * @brief 根据实际平台启动并初始化，用户 APP 入口为 void work_app_main(void *arg)
 * @version 0.1
 * @date 2023-11-24
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "os/os.h"
#include "app_main.h"

#include "ota.h"
#include "nvs_flash.h"
#include "esp_err.h"

#undef SYS_LOG_DOMAIN
#define SYS_LOG_DOMAIN "OS"
#include "os_util.h"

#ifndef CONFIG_OS_THREAD_MAIN_STACK_SIZE
#define CONFIG_OS_THREAD_MAIN_STACK_SIZE 0x1C00
#endif

#ifndef CONFIG_OS_THREAD_MAIN_PRIORITY
#define CONFIG_OS_THREAD_MAIN_PRIORITY OS_PRIORITY_NORMAL
#endif

static unsigned s_int_nest;

static unsigned _port_interrupt_save(void)
{
    if (s_int_nest == 0)
    {
        os_interrupt_disable();
    }
    return s_int_nest++;
}

static void _port_interrupt_restore(unsigned nest)
{
    s_int_nest = nest;
    if (nest == 0)
    {
        os_interrupt_enable();
    }
}

static k_work_q_t *_port_get_work_q_hdl(void)
{
    struct os_thread_handle *thread_handle = os_thread_get_self();
    os_work_q_list_t *work_q_list = thread_handle->work_q_list;
    return &work_q_list->work_q_handle;
}

static void _port_thread_sleep(k_tick_t sleep_ticks)
{
    vTaskDelay(sleep_ticks);
}

static void _os_init(void)
{
    static k_init_t const init_struct = {
        .malloc = os_malloc,
        .free = os_free,
        .get_sys_ticks = os_get_sys_ticks,
        .interrupt_save = _port_interrupt_save,
        .interrupt_restore = _port_interrupt_restore,
        .scheduler_disable = os_scheduler_suspend,
        .scheduler_enable = os_scheduler_resume,
        .get_work_q_hdl = _port_get_work_q_hdl,
        .thread_sleep = _port_thread_sleep,
    };
    k_init(&init_struct);

    os_work_q_create(default_os_work_q_hdl,
                     "app-work_q",
                     CONFIG_OS_THREAD_MAIN_STACK_SIZE,
                     CONFIG_OS_THREAD_MAIN_PRIORITY);

    extern void work_app_main(void *arg);
    static os_work_t _work_hdl_init;
    os_work_create(&_work_hdl_init, "work-main", work_app_main, NULL, 3);
    os_work_submit(default_os_work_q_hdl, &_work_hdl_init, 0);
}

static void _init_prev_nvs(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        // OTA app partition table has a smaller NVS partition size than the
        // non-OTA partition table. This size mismatch may cause NVS
        // initialization to fail. If this happens, we erase NVS partition and
        // initialize NVS again.
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

int app_main(void)
{
    /* ota */
    ota_partition_check();

    /* 初始化平台相关 */
    _init_prev_nvs();

    /* 初始化 os 抽象层 */
    _os_init();

    return 0;
}
