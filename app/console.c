#include "driver/uart.h"
#include "console.h"
#include "shell/sh.h"
#include "os/os.h"
#include "drivers/chip/uart.h"
#include "config/board_config.h"
#include "sys_log.h"

static shell_input_cb s_input_cb;

static void initialize_console(void)
{
    /* Drain stdout before reconfiguring it */
    fflush(stdout);

    /* Disable buffering on stdin */
    setvbuf(stdin, NULL, _IONBF, 0);

    /* Configure UART. Note that REF_TICK is used so that the baud rate remains
     * correct while APB frequency is changing in light sleep mode.
     */
    const uart_config_t uart_config = {
        .baud_rate = CONFIG_ESP_CONSOLE_UART_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
#if CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S2
        .source_clk = UART_SCLK_REF_TICK,
#else
        .source_clk = UART_SCLK_XTAL,
#endif
    };
    /* Install UART driver for interrupt-driven reads and writes */
    ESP_ERROR_CHECK(uart_driver_install(CONFIG_ESP_CONSOLE_UART_NUM,
                                        256, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(CONFIG_ESP_CONSOLE_UART_NUM, &uart_config));
}

static void _work_sh(void *arg)
{
    char c;
    int flag = 0;
    shell_input_cb input_cb = s_input_cb;

    if (input_cb)
    {
        static char last_key;
        while (drv_uart_poll_read(g_cfg_board->uart_console.id, &c) > 0)
        {
            if (c != '\n' || last_key != '\r')
            {
                input_cb(&g_uart_handle_vt100, c);
                flag = 1;
            }
            last_key = c;
        }
    }
    else
    {
        while (drv_uart_poll_read(g_cfg_board->uart_console.id, &c) > 0)
        {
            sh_putc(&g_uart_handle_vt100, c);
            flag = 1;
        }
    }

    if (flag)
    {
        fflush(stdout);
    }

    os_work_later(10);
}

static int _port_sh_vprint_fn(const char *format, va_list va)
{
    return vprintf(format, va);
}

/**
 * @brief 启动 shell.
 * 这将：
 * 1. 执行对应的初始化
 * 2. 自动默认工作队列中创建一个解析数据的后台
 */
void shell_start(void)
{
    static uint8_t start_flag = 0;

    if (start_flag == 0)
    {
        /* 初始化控制台串口 */
        initialize_console();

        /* 执行 shell 内部的初始化 */
        sh_register_external(_port_sh_vprint_fn);

        /* 注册系统命令 */
        extern void soc_shell_register(void);
        soc_shell_register();

        /* 创建 shell 的接收任务 _work_sh() */
        static os_work_t work_handler_shell;
        os_work_create(&work_handler_shell, "work-shell", _work_sh, NULL, 1);
        os_work_submit(default_os_work_q_hdl, &work_handler_shell, 0);

        /* 设置 shell 的提示符 */
        g_uart_handle_vt100.disable_echo = !CONFIG_SYS_LOG_CONS_ON; // 关闭回显
        sh_putstr_quiet(&g_uart_handle_vt100, "sh version\r");
        sh_set_prompt(&g_uart_handle_vt100, "esp32:/> ");
        sh_putc(&g_uart_handle_vt100, '\r');
        fflush(stdout);

        start_flag = 1;
    }
}

/**
 * @brief 临时改变终端的输入接口。
 * 使用 shell_input_restore() 可恢复预设的接口
 *
 * @param cb 输入接口
 */
void shell_input_set(shell_input_cb cb)
{
    s_input_cb = cb;
}

/**
 * @brief 恢复 shell_input_set() 被执行前的设置
 *
 */
void shell_input_restore(void)
{
    s_input_cb = NULL;
}

/**
 * @brief 在执行命令的过程中使用。
 * 若被执行的命令需要循环执行时，使用这个函数将读取控制台收到的数据，
 * 当收到 ESC 按键时返回真值，此时可退出该命令的执行函数
 *
 * @retval true 按下了 ESC 键
 * @retval false 未按下 ESC 键
 */
bool shell_is_aobrt(void)
{
    char c;
    if (drv_uart_poll_read(g_cfg_board->uart_console.id, &c) > 0)
    {
        if (c == '\x1b')
        {
            fflush(stdout);
            return true;
        }
    }
    return false;
}
