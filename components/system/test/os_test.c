#include "os_test.h"
#include "sys_init.h"

Type_OS_TestMem_Def rm_Test; // 用于测试的内存
os_work_t test_work_handle[10];
os_fifo_t fifo_handle[10];
os_queue_t queue_handle[10];
os_pipe_t pipe_handle[10];

os_thread_t *os_test_create_thread(os_thread_t *thread_handle, void (*TaskRoute)(void *), char *name, s16_t Priority)
{
    os_thread_create(thread_handle,
                     name,
                     TaskRoute,
                     0,
                     0x1000,
                     Priority);

    return thread_handle;
}
//

/* 具体测试 */
extern void os_test_fifo(void);
extern void os_test_queue(void);
extern void os_test_pipe(void);
extern void os_test_yield(uint TestTime);
extern void os_test_sem(uint TestTime);
extern void os_test_sleep(uint TestTime);

typedef void (*test_single_t)(void);
typedef void (*test_con_t)(uint TestTime);

static test_con_t const test_tab_rt[] = {
    os_test_yield,
    os_test_sem,
    os_test_sleep,
};

static test_single_t const test_tab_ipc[] = {
    os_test_fifo,
    os_test_queue,
    os_test_pipe,
};

INIT_EXPORT_APP(99)
{
    SYS_LOG_INF("success");
    os_thread_sleep(200);
}

void os_test_main(void)
{
    rand();             // rand() 用到 malloc()
    CONS_PRINT("\r\n"); // printf() 用到 _malloc_r()

    SYS_LOG_INF("测试准备中 ... \r\n");

    CONS_PRINT("开始测试...\r\n\r\n");

    for (uint i = 0; i < sizeof(test_tab_rt) / sizeof(test_tab_rt[0]); i++)
    {
        test_tab_rt[i](1);
        CONS_PRINT("\r\n");
    }

    for (uint i = 0; i < sizeof(test_tab_ipc) / sizeof(test_tab_ipc[0]); i++)
    {
        test_tab_ipc[i]();
    }

    for (uint n = 0; n < sizeof(test_work_handle) / sizeof(test_work_handle[0]); n++)
    {
        if (os_work_is_valid(&test_work_handle[n]))
        {
            os_work_delete(&test_work_handle[n]);
        }
    }

    CONS_PRINT("所有测试结束\r\n");
    CONS_PRINT("============================\r\n");
}
