#include "os/os.h"
#include "os_util.h"
#include "task.h"

static size_t int_flag;
static portMUX_TYPE s_os_service = portMUX_INITIALIZER_UNLOCKED;

int os_start(void *heap_mem, size_t heap_size)
{
    extern int os_entry(void *heap, size_t size);
    return os_entry(heap_mem, heap_size);
}

void os_int_entry(void)
{
    os_interrupt_disable();
    ++int_flag;
    os_interrupt_enable();
}

void os_int_exit(void)
{
    os_interrupt_disable();
    int_flag -= !!int_flag;
    os_interrupt_enable();
}

bool os_is_isr_context(void)
{
    if (int_flag)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void os_interrupt_disable(void)
{
    portENTER_CRITICAL(&s_os_service);
}

void os_interrupt_enable(void)
{
    portEXIT_CRITICAL(&s_os_service);
}

void os_scheduler_suspend(void)
{
    vTaskSuspendAll();
}

void os_scheduler_resume(void)
{
    xTaskResumeAll();
}

bool os_scheduler_is_running(void)
{
    return (bool)(xTaskGetSchedulerState() == taskSCHEDULER_RUNNING);
}

#ifdef CONFIG_FREERTOS_USE_STATS_FORMATTING_FUNCTIONS
void os_sys_print_info(void)
{
    const size_t bytes_per_task = 40; /* see vTaskList description */
    char *task_list_buffer = malloc(uxTaskGetNumberOfTasks() * bytes_per_task);
    if (task_list_buffer == NULL)
    {
        OS_LOG(1, "failed to allocate buffer for vTaskList output\r\n");
        return;
    }
    fputs("Task Name\tStatus\tPrio\tHWM\tTask#", stdout);
#ifdef CONFIG_FREERTOS_VTASKLIST_INCLUDE_COREID
    fputs("\tAffinity", stdout);
#endif
    fputs("\n", stdout);
    vTaskList(task_list_buffer);
    fputs(task_list_buffer, stdout);
    free(task_list_buffer);
}

#else

void os_sys_print_info(void)
{
    OS_LOG(1, "os_sys_print_info() not supported, please set configUSE_TRACE_FACILITY to 1\n");
}

#endif

os_time_t os_get_sys_time(void)
{
    os_time_t ticks_count = xTaskGetTickCount();
    return OS_TicksToMSecs(ticks_count);
}

size_t os_get_sys_ticks(void)
{
    return (size_t)xTaskGetTickCount();
}

os_time_t os_calc_ticks_to_msec(size_t ticks)
{
    os_time_t msec;

    if (ticks == OS_WAIT_FOREVER)
    {
        msec = portMAX_DELAY;
    }
    else if (ticks == 0)
    {
        msec = 0;
    }
    else
    {
        msec = OS_TicksToMSecs(ticks);
        if (msec == 0)
        {
            msec = 1;
        }
    }
    return msec;
}

size_t os_calc_msec_to_ticks(os_time_t msec)
{
    size_t tick;

    if (msec == OS_WAIT_FOREVER)
    {
        tick = portMAX_DELAY;
    }
    else if (msec == 0)
    {
        tick = 0;
    }
    else
    {
        tick = OS_MSecsToTicks(msec);
        if (tick == 0)
        {
            tick = 1;
        }
    }
    return tick;
}

size_t os_cpu_usage(void)
{
    return 100;
}

#if (configUSE_POSIX_ERRNO == 1)

extern int FreeRTOS_errno;

int os_get_err(void)
{
    return FreeRTOS_errno;
}

void os_set_err(int err)
{
    FreeRTOS_errno = err;
}

#elif (configNUM_THREAD_LOCAL_STORAGE_POINTERS != 0)

#define OS_ERRNO_LOCATION_IDX 0

int os_get_err(void)
{
    return (int)pvTaskGetThreadLocalStoragePointer(NULL, OS_ERRNO_LOCATION_IDX);
}

void os_set_err(int err)
{
    vTaskSetThreadLocalStoragePointer(NULL, OS_ERRNO_LOCATION_IDX, (void *)err);
}

#endif
