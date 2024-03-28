#include "os_test.h"

static vuint test_flag = 0;

static void _os_test_fifo_handler_0(void *arg)
{
    SYS_LOG_INF("测试创建和清空, 应出现1个警告");
    void *first_handle;
    SYS_ASSERT_FALSE(os_fifo_q_create(&fifo_handle[0]) != OS_OK, "");
    first_handle = fifo_handle[0].handle;
    os_fifo_q_clr(&fifo_handle[0]);
    os_fifo_q_delete(&fifo_handle[0]);

    SYS_LOG_INF("测试警告, 应出现3个警告");
    SYS_ASSERT_FALSE(os_fifo_q_create(&fifo_handle[0]) != OS_OK, "");
    uint32_t *p32 = os_fifo_alloc(sizeof(*p32));
    SYS_ASSERT_FALSE(p32 == 0, "");
    *p32 = 0x1234abcd;
    SYS_ASSERT_FALSE(os_fifo_put(&fifo_handle[0], p32) != OS_OK, "");
    SYS_ASSERT_FALSE(os_fifo_put(&fifo_handle[0], p32) == OS_OK, ""); // os_fifo_put(): fifo_data not valid
    SYS_ASSERT_FALSE(os_fifo_take(&fifo_handle[0], 0) != p32, "");
    SYS_ASSERT_FALSE(*p32 != 0x1234abcd, "");
    SYS_ASSERT_FALSE(os_fifo_take(&fifo_handle[0], 0) != 0, "");
    SYS_ASSERT_FALSE(os_fifo_free(p32) != OS_OK, "");
    SYS_ASSERT_FALSE(os_fifo_free(p32) == OS_OK, "");                 // os_fifo_free(): 0x.. was not in fifo queue, can not to free
    SYS_ASSERT_FALSE(os_fifo_put(&fifo_handle[0], p32) == OS_OK, ""); // os_fifo_put(): fifo_data not valid

    SYS_LOG_INF("测试清除");
    SYS_ASSERT_FALSE(os_fifo_alloc(sizeof(*p32)) != p32, "");
    SYS_ASSERT_FALSE(os_fifo_put(&fifo_handle[0], p32) != OS_OK, "");
    os_fifo_q_clr(&fifo_handle[0]);
    SYS_ASSERT_FALSE(os_fifo_take(&fifo_handle[0], 0) != 0, "");
    SYS_ASSERT_FALSE(os_fifo_alloc(sizeof(*p32)) != p32, "");
    SYS_ASSERT_FALSE(os_fifo_free(p32) != OS_OK, "");

    SYS_LOG_INF("测试自动释放已在队列内的数据");
    SYS_ASSERT_FALSE(os_fifo_alloc(sizeof(*p32)) != p32, "");
    SYS_ASSERT_FALSE(os_fifo_put(&fifo_handle[0], p32) != 0, "");
    os_fifo_q_delete(&fifo_handle[0]);
    SYS_ASSERT_FALSE(os_fifo_q_create(&fifo_handle[0]) != OS_OK, "");
    SYS_ASSERT_FALSE(first_handle != fifo_handle[0].handle, "");
    SYS_ASSERT_FALSE(os_fifo_alloc(sizeof(*p32)) != p32, "");
    SYS_ASSERT_FALSE(os_fifo_put(&fifo_handle[0], p32) != OS_OK, "");
    os_fifo_q_delete(&fifo_handle[0]);
    test_flag = 1;
}

static void _os_test_fifo_t1(void *arg)
{
    test_flag = test_flag << 4 | 1;
    void *mem = os_fifo_take(&fifo_handle[0], -1u);
    test_flag = test_flag << 4 | 2;
    os_fifo_free(mem);
}

void os_test_fifo(void)
{
    SYS_LOG_INF("-------------------------------");
    test_flag = 0;
    os_work_create(&test_work_handle[0], "", _os_test_fifo_handler_0, NULL, 1);
    os_work_submit(default_os_work_q_hdl,  &test_work_handle[0], 0);
    os_work_delete(&test_work_handle[0]);
    SYS_ASSERT_FALSE(test_flag == 0, "");

    SYS_LOG_INF("-------------------------------");
    size_t max_block_size;
    os_heap_info(NULL, NULL, &max_block_size);
    static os_thread_t t1;
    void *mem;

    SYS_LOG_INF("测试提交 FIFO 唤醒");
    test_flag = 0;
    os_fifo_q_create(&fifo_handle[0]);
    os_test_create_thread(&t1, _os_test_fifo_t1, "t1", OS_PRIORITY_HIGHEST);
    SYS_ASSERT_FALSE(test_flag != 0x1, "");
    mem = os_fifo_alloc(0x10);
    SYS_ASSERT_FALSE(test_flag != 0x1, "");
    os_fifo_put(&fifo_handle[0], mem);
    SYS_ASSERT_FALSE(test_flag != 0x12, "");
    os_fifo_q_delete(&fifo_handle[0]);
}
