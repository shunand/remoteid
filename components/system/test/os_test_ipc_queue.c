#include "os_test.h"

static vuint test_flag = 0;

static void _os_test_queue_t3(void *arg)
{
    uint64_t dst_64 = 0;
    test_flag = test_flag << 4 | 1;
    SYS_ASSERT_FALSE(os_queue_recv(&queue_handle[0], &dst_64, OS_WAIT_FOREVER) != OS_OK, "");
    SYS_ASSERT_FALSE(dst_64 != 0x1234567890abcdef, "");
    test_flag = test_flag << 4 | 2;
}

void os_test_queue(void)
{
    SYS_LOG_INF("-------------------------------");
    static os_thread_t t2;
    uint64_t src_64 = 0x1234567890abcdef;
    uint64_t dst_64 = 0;
    int systime_start;
    int systime_end;
    os_queue_create(&queue_handle[0], 1, sizeof(src_64));

    SYS_LOG_INF("测试发送和接收");
    test_flag = 0;
    os_test_create_thread(&t2, _os_test_queue_t3, "t3", OS_PRIORITY_HIGHEST);
    SYS_ASSERT_FALSE(test_flag != 0x1, "test_flag = 0x%x", test_flag);
    SYS_ASSERT_FALSE(os_queue_send(&queue_handle[0], &src_64, 100) != OS_OK, "");
    SYS_ASSERT_FALSE(test_flag != 0x12, "test_flag = 0x%x", test_flag);

    SYS_LOG_INF("测试发送和发送超时");
    systime_start = os_get_sys_ticks();
    SYS_ASSERT_FALSE(os_queue_send(&queue_handle[0], &src_64, 100) != OS_OK, "");
    SYS_ASSERT_FALSE(os_queue_send(&queue_handle[0], &src_64, 100) == OS_OK, "");
    systime_end = os_get_sys_ticks();
    SYS_ASSERT_FALSE(systime_end - systime_start < 100 || systime_end - systime_start > 120, "systime_end - systime_start = %d", systime_end - systime_start);

    SYS_LOG_INF("测试接收和接收超时");
    systime_start = os_get_sys_ticks();
    SYS_ASSERT_FALSE(os_queue_recv(&queue_handle[0], &dst_64, 100) != OS_OK, "");
    SYS_ASSERT_FALSE(dst_64 != src_64, "");
    SYS_ASSERT_FALSE(os_queue_recv(&queue_handle[0], &dst_64, 100) == OS_OK, "");
    systime_end = os_get_sys_ticks();
    SYS_ASSERT_FALSE(systime_end - systime_start < 100 || systime_end - systime_start > 120, "systime_end - systime_start = %d", systime_end - systime_start);

    os_queue_delete(&queue_handle[0]);
}
