#include "os_test.h"

static os_thread_t thread_1;
static os_thread_t thread_2;
static os_thread_t thread_3;
static os_sem_t test_sem_fifo;
static vuint test_flag = 0;
static void _test_os_t1(void *arg)
{
    os_sem_take(&test_sem_fifo, OS_WAIT_FOREVER);
    test_flag = test_flag << 4 | 1;
    os_sem_take(&test_sem_fifo, OS_WAIT_FOREVER);
    test_flag = test_flag << 4 | 1;
}
static void _test_os_t2(void *arg)
{
    os_sem_take(&test_sem_fifo, OS_WAIT_FOREVER);
    test_flag = test_flag << 4 | 2;
}
static void _test_os_t3(void *arg)
{
    os_sem_take(&test_sem_fifo, OS_WAIT_FOREVER);
    test_flag = test_flag << 4 | 3;
}

void os_test_sem(uint TestTime)
{
    SYS_LOG_INF("");

    SYS_LOG_DBG("测试 os_sem_create()");
    SYS_ASSERT_FALSE(os_sem_create(&test_sem_fifo, 1, 3) != OS_OK, "");

    SYS_LOG_DBG("测试 OS_SEM_TYPE_FIFO");
    test_flag = 0;
    os_test_create_thread(&thread_1, _test_os_t1, "task01-2", 2);
    os_test_create_thread(&thread_2, _test_os_t2, "task02-3", 3);
    os_test_create_thread(&thread_3, _test_os_t3, "task03-4", 4);
    SYS_ASSERT(test_flag == 0x1, "");

    os_sem_release(&test_sem_fifo);
    os_sem_release(&test_sem_fifo);
    os_sem_release(&test_sem_fifo);
    SYS_ASSERT(test_flag == 0x1321, "test_flag = %#x", test_flag);

    os_sem_delete(&test_sem_fifo);
}
