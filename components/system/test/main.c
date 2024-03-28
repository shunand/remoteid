#include "os_test.h"
#include "sys_init.h"
#include "os/os.h"

#undef SYS_LOG_DOMAIN
#define SYS_LOG_DOMAIN "OS"
#include "os_util.h"

#ifndef CONFIG_OS_THREAD_MAIN_STACK_SIZE
#define CONFIG_OS_THREAD_MAIN_STACK_SIZE 0x2000
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

static void _work_app_main(void *arg)
{
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

    static os_work_t _work_hdl_init;
    os_work_create(&_work_hdl_init, "work-main", _work_app_main, NULL, 3);
    os_work_submit(default_os_work_q_hdl, &_work_hdl_init, 0);
}

#include "sh.h"

static void _work_sh(void *arg)
{
    char c;
    // while (drv_uart_poll_read(board_uart_cons.id, &c) == 0)
    // {
    //     sh_putc(&g_uart_handle_vt100, c);
    // }
    os_work_later(10);
}

static void _start_shell(void)
{
    static os_work_t work_handler_shell;
    os_work_create(&work_handler_shell, "work-shell", _work_sh, NULL, 0);
    os_work_submit(default_os_work_q_hdl, &work_handler_shell, 0);

    sh_putstr_quiet(&g_uart_handle_vt100, "sh version\r");
    sh_set_prompt(&g_uart_handle_vt100, "sh:/> ");
    sh_putc(&g_uart_handle_vt100, '\r');
}

void _app_main_handler(void *pArg)
{
    extern int os_test_main(void);
    os_test_main();
}

int app_main(void)
{
    _os_init();

    _start_shell();

    static os_thread_t s_main_thread;
    static char *const FirstTaskName = "test_main"; // 首个线程的线程名

    os_thread_create(&s_main_thread,
                     FirstTaskName,
                     _app_main_handler,
                     (void *)rand(),
                     0x1000,
                     1);

    return 0;
}
