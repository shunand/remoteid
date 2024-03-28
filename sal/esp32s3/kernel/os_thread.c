#include "os/os.h"
#include "os_util.h"
#include "task.h"
#include "list/slist.h"

/* Macro used to convert os_priority to the kernel's real priority */
#define OS_KERNEL_PRIO(prio) (prio)

static slist_t s_thread_list;

static void _os_delete(struct os_thread_handle *thread_handle)
{
    os_scheduler_suspend();

    slist_remove(&s_thread_list, &thread_handle->node);
    thread_handle->thread->handle = NULL;
    if (thread_handle->flag_free)
    {
        os_free(thread_handle->thread);
    }
    os_free(thread_handle);

    os_scheduler_resume();
}

static void _os_thread_start(void *arg)
{
    struct os_thread_handle *thread_handle = arg;
    thread_handle->entry(thread_handle->arg);
    _os_delete(thread_handle);
    vTaskDelete(NULL);
}

os_state os_thread_init(os_thread_t *thread,
                        const char *name,
                        os_thread_entry_t entry,
                        void *arg,
                        void *stack_base,
                        size_t stack_size,
                        os_priority priority)
{
#if (configSUPPORT_STATIC_ALLOCATION == 1)

    OS_ASS_HDL(!os_thread_is_valid(thread), thread->handle);

    if (stack_size <= sizeof(struct os_thread_handle) + sizeof(StaticTask_t))
    {
        OS_ERR("err size: %d <= %d\r\n", stack_size, sizeof(struct os_thread_handle) + sizeof(StaticTask_t));
        return OS_FAIL;
    }

    struct os_thread_handle *thread_handle = stack_base;
    StaticTask_t *pxTaskBuffer = (StaticTask_t *)&thread_handle[1];
    StackType_t *puxStackBuffer = (StackType_t *)&pxTaskBuffer[1];

    thread_handle->thread = thread;
    thread_handle->entry = entry;
    thread_handle->arg = arg;
    thread_handle->flag_free = 0;
    thread_handle->pxCreatedTask = xTaskCreateStatic(_os_thread_start,
                                                     name,
                                                     stack_size / sizeof(StackType_t),
                                                     thread_handle,
                                                     OS_KERNEL_PRIO(priority),
                                                     puxStackBuffer,
                                                     pxTaskBuffer);
    thread->handle = thread_handle;
    return OS_OK;

#else

    OS_ERR("err configSUPPORT_STATIC_ALLOCATION != 1\r\n");
    return OS_FAIL;

#endif
}

os_state os_thread_create(os_thread_t *thread,
                          const char *name,
                          os_thread_entry_t entry,
                          void *arg,
                          size_t stack_size,
                          os_priority priority)
{
    int ret;

    if (thread)
    {
        OS_ASS_HDL(!os_thread_is_valid(thread), thread->handle);
    }

    struct os_thread_handle *thread_handle = os_malloc(sizeof(struct os_thread_handle));
    if (thread_handle == NULL)
    {
        return OS_E_NOMEM;
    }

    if (thread == NULL)
    {
        thread = os_malloc(sizeof(os_thread_t));
        if (thread == NULL)
        {
            os_free(thread_handle);
            return OS_E_NOMEM;
        }
        memset(thread, 0, sizeof(os_thread_t));
        thread_handle->flag_free = 1;
    }
    else
    {
        thread_handle->flag_free = 0;
    }
    thread->handle = thread_handle;
    thread_handle->work_q_list = NULL;
    thread_handle->thread = thread;
    thread_handle->entry = entry;
    thread_handle->arg = arg;
    slist_init_node(&thread_handle->node);

    os_scheduler_suspend();
    slist_insert_tail(&s_thread_list, &thread_handle->node);
    os_scheduler_resume();

    ret = xTaskCreatePinnedToCore(_os_thread_start,
                                  name,
                                  stack_size / sizeof(StackType_t),
                                  thread_handle,
                                  OS_KERNEL_PRIO(priority),
                                  &thread_handle->pxCreatedTask,
                                  1);
    if (ret != pdPASS)
    {
        OS_ERR("err %d\r\n", ret);

        os_scheduler_suspend();
        slist_remove(&s_thread_list, &thread_handle->node);
        os_scheduler_resume();

        if (thread_handle->flag_free == 0)
        {
            thread->handle = NULL;
        }
        if (thread_handle->thread)
        {
            os_free(thread_handle->thread);
        }
        os_free(thread_handle);
        return OS_FAIL;
    }
    return OS_OK;
}

os_state os_thread_delete(os_thread_t *thread)
{
    if (thread == NULL) /* delete self */
    {
        struct os_thread_handle *thread_handle = os_thread_get_self();
        _os_delete(thread_handle);
        vTaskDelete(NULL);
        return OS_OK;
    }
    else
    {
        OS_ASS_HDL(os_thread_is_valid(thread), thread->handle);
        struct os_thread_handle *thread_handle = thread->handle;
        vTaskDelete(thread_handle->pxCreatedTask);
        _os_delete(thread_handle);
        return OS_OK;
    }
}

void os_thread_sleep(os_time_t msec)
{
    vTaskDelay((TickType_t)os_calc_msec_to_ticks(msec));
}

void os_thread_yield(void)
{
    taskYIELD();
}

void os_thread_suspend(os_thread_t *thread)
{
    if (thread)
    {
        OS_ASS_HDL(os_thread_is_valid(thread), thread->handle);
        struct os_thread_handle *thread_handle = thread->handle;
        vTaskSuspend(thread_handle->pxCreatedTask);
    }
    else
    {
        vTaskSuspend(NULL);
    }
}

void os_thread_resume(os_thread_t *thread)
{
    OS_ASS_HDL(os_thread_is_valid(thread), thread->handle);
    struct os_thread_handle *thread_handle = thread->handle;
    vTaskResume(thread_handle->pxCreatedTask);
}

os_thread_handle_t os_thread_get_self(void)
{
    os_thread_handle_t ret = NULL;

    os_scheduler_suspend();
    TaskHandle_t xTask = xTaskGetCurrentTaskHandle();
    struct os_thread_handle *thread_handle;
    struct os_thread_handle *prev_handle = NULL;
    int cnt = 0;
    SLIST_FOR_EACH_CONTAINER(&s_thread_list, thread_handle, node)
    {
        if (thread_handle->pxCreatedTask == xTask)
        {
            ret = thread_handle->thread->handle;
            break;
        }
        prev_handle = thread_handle;
        cnt++;
    }
    if (cnt > 5)
    {
        slist_remove_next(&s_thread_list, &prev_handle->node, &thread_handle->node);
        slist_insert_font(&s_thread_list, &thread_handle->node);
    }
    os_scheduler_resume();
    SYS_ASSERT(ret, "xTask: %p", xTask);
    return ret;
}

const char *os_thread_get_name(os_thread_t *thread)
{
    struct os_thread_handle *thread_handle = thread->handle;
    return pcTaskGetName(thread_handle->pxCreatedTask);
}

size_t os_thread_stack_min(os_thread_t *thread)
{
#if INCLUDE_uxTaskGetStackHighWaterMark
    TaskHandle_t xTask;

    if (thread != NULL)
    {
        if (os_thread_is_valid(thread))
        {
            struct os_thread_handle *thread_handle = thread->handle;
            xTask = thread_handle->pxCreatedTask;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        xTask = NULL;
    }

    extern UBaseType_t uxTaskGetStackHighWaterMark(TaskHandle_t xTask);
    return (uxTaskGetStackHighWaterMark(xTask) * sizeof(StackType_t));
#else
    return 0;
#endif
}

bool os_thread_is_valid(os_thread_t *thread)
{
    if (thread && thread->handle)
    {
        return true;
    }
    else
    {
        return false;
    }
}
