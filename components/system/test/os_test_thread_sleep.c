#include "os_test.h"

void os_test_sleep(uint TestTime)
{
    vuint start_tick;
    vuint test_tick;
    int offset;

    SYS_LOG_INF("测试延时");

    SYS_LOG_DBG("测试 os_thread_sleep()");
    os_thread_sleep(1);
    start_tick = os_get_sys_ticks();
    os_thread_sleep(10);
    test_tick = os_get_sys_ticks();
    offset = test_tick - start_tick;
    SYS_ASSERT_FALSE(offset != 10 && offset != 11, "offset = %d", offset);

    SYS_LOG_DBG("测试 os_thread_sleep_until()");
    os_thread_sleep(1);
    start_tick = os_get_sys_ticks();

    os_thread_sleep(30);
    test_tick = os_get_sys_ticks();
    offset = test_tick - start_tick;
    SYS_ASSERT_FALSE(offset < 30 || offset > 31, "offset = %d", offset); //使用软件测试时允许有1毫秒的误差
}
