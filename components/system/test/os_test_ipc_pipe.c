#include "os_test.h"

static vuint test_flag = 0;

static void _os_test_pipe_handler_0(void *arg)
{
    uint32_t data = 0;
    uint8_t *p8 = (uint8_t *)&data;
    SYS_LOG_INF("测试创建和清空, 应出现1个警告");
    void *first_handle;
    SYS_ASSERT_FALSE(os_pipe_create(&pipe_handle[0], 4) != OS_OK, "");
    first_handle = pipe_handle[0].handle;
    os_pipe_clr(&pipe_handle[0]);
    os_pipe_delete(&pipe_handle[0]);

    SYS_LOG_INF("测试状态");
    SYS_ASSERT_FALSE(os_pipe_create(&pipe_handle[0], 3) != OS_OK, "");
    SYS_ASSERT_FALSE(first_handle != pipe_handle[0].handle, "");
    SYS_ASSERT_FALSE(os_pipe_is_ne(&pipe_handle[0]) != false, "");
    SYS_ASSERT_FALSE(os_pipe_get_valid_size(&pipe_handle[0]) != 0, "");
    SYS_ASSERT_FALSE(os_pipe_get_empty_size(&pipe_handle[0]) != 3, "");

    SYS_LOG_INF("测试写");
    SYS_ASSERT_FALSE(os_pipe_poll_write(&pipe_handle[0], 1) != 1, "");
    SYS_ASSERT_FALSE(os_pipe_is_ne(&pipe_handle[0]) != true, "");
    SYS_ASSERT_FALSE(os_pipe_get_valid_size(&pipe_handle[0]) != 1, "");
    SYS_ASSERT_FALSE(os_pipe_get_empty_size(&pipe_handle[0]) != 2, "");
    p8[0] = 2, p8[1] = 3;
    SYS_ASSERT_FALSE(os_pipe_fifo_fill(&pipe_handle[0], &data, sizeof(data)) != 2, "");
    SYS_ASSERT_FALSE(os_pipe_get_valid_size(&pipe_handle[0]) != 3, "");
    SYS_ASSERT_FALSE(os_pipe_get_empty_size(&pipe_handle[0]) != 0, "");

    SYS_LOG_INF("测试读");
    data = -1u;
    SYS_ASSERT_FALSE(os_pipe_poll_read(&pipe_handle[0], &p8[0]) != 1, "");
    SYS_ASSERT_FALSE(os_pipe_is_ne(&pipe_handle[0]) != true, "");
    SYS_ASSERT_FALSE(os_pipe_get_valid_size(&pipe_handle[0]) != 2, "");
    SYS_ASSERT_FALSE(os_pipe_get_empty_size(&pipe_handle[0]) != 1, "");
    SYS_ASSERT_FALSE(os_pipe_fifo_read(&pipe_handle[0], &p8[1], 3) != 2, "");
    SYS_ASSERT_FALSE(os_pipe_is_ne(&pipe_handle[0]) != false, "");
    SYS_ASSERT_FALSE(os_pipe_get_valid_size(&pipe_handle[0]) != 0, "");
    SYS_ASSERT_FALSE(os_pipe_get_empty_size(&pipe_handle[0]) != 3, "");
    SYS_ASSERT_FALSE(data != 0xff030201, "");

    os_pipe_delete(&pipe_handle[0]);
    test_flag = 1;
}

void os_test_pipe(void)
{
    SYS_LOG_INF("-------------------------------");
    test_flag = 0;
    os_work_create(&test_work_handle[0], "", _os_test_pipe_handler_0, NULL, 1);
    os_work_submit(default_os_work_q_hdl,  &test_work_handle[0], 0);
    os_work_delete(&test_work_handle[0]);
    SYS_ASSERT_FALSE(test_flag == 0, "");
}
