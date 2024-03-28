#include "os_test.h"

static os_thread_t thread_1;
static os_thread_t thread_2;
static os_thread_t thread_3;
static os_thread_t thread_4;
static vuint count1 = 0;
static vuint count2 = 0;
static vuint count3 = 0;
static vuint count4 = 0;

#define _TEST_TIMES 2000

static void _test_os_t1(void *arg)
{
    count1 = 0;
    while (1)
    {
        if (count1++ % _TEST_TIMES == 0)
        {
            SYS_LOG_DBG("%d", count1 / _TEST_TIMES);
            if (count1 >= (_TEST_TIMES * 10))
            {
                return;
            }
        }
        os_thread_yield();
    }
}
static void _test_os_t2(void *arg)
{
    count2 = 0;
    while (1)
    {
        if (count2++ % _TEST_TIMES == 0)
        {
            SYS_LOG_DBG("%d", count2 / _TEST_TIMES);
            if (count2 >= (_TEST_TIMES * 10))
            {
                return;
            }
        }
        os_thread_yield();
    }
}

static void _test_os_t3(void *arg)
{
    count3 = 0;
    while (1)
    {
        if (count3++ % _TEST_TIMES == 0)
        {
            SYS_LOG_DBG("%d", count3 / _TEST_TIMES);
            if (count3 >= (_TEST_TIMES * 10))
            {
                return;
            }
        }
        os_thread_suspend(NULL);
        SYS_ASSERT_FALSE(count3 != count4, "count3 = %d, count4 = %d, suspend fail", count3, count4);
    }
}
static void _test_os_t4(void *arg)
{
    count4 = 0;
    while (1)
    {
        if (count4++ % _TEST_TIMES == 0)
        {
            SYS_LOG_DBG("%d", count4 / _TEST_TIMES);
            if (count4 >= (_TEST_TIMES * 10))
            {
                return;
            }
        }
        os_thread_resume(&thread_3);
        SYS_ASSERT_FALSE(count4 + 1 != count3, "count3 = %d, count4 = %d, thread 3 dead!", count3, count4);
    }
}

void os_test_yield(uint TestTime)
{
    uint start_tick, test_tick, time_nS;
    SYS_LOG_INF("");

    os_scheduler_suspend();

    os_test_create_thread(&thread_1, _test_os_t1, "task01-4", 4);
    os_test_create_thread(&thread_2, _test_os_t2, "task02-4", 4);

    start_tick = os_get_sys_ticks();
    os_scheduler_resume();
    test_tick = os_get_sys_ticks();
    time_nS = (test_tick - start_tick) * 1000000 / (count1 + count2);
    SYS_LOG_DBG("平均时间: %d.%03d uS", time_nS / 1000, time_nS % 1000);

    os_scheduler_suspend();

    os_test_create_thread(&thread_3, _test_os_t3, "task03-3", 3);
    os_test_create_thread(&thread_4, _test_os_t4, "task04-2", 2);

    start_tick = os_get_sys_ticks();
    os_scheduler_resume();
    test_tick = os_get_sys_ticks();
    time_nS = (test_tick - start_tick) * 1000000 / (count3 + count4);
    SYS_LOG_DBG("平均时间: %d.%03d uS", time_nS / 1000, time_nS % 1000);

    os_sys_print_info();
}
