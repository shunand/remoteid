#include "os/os.h"
#include "os_util.h"
#include "k_kit.h"

static os_work_q_t s_default_work_q_handle;
os_work_q_t *default_os_work_q_hdl = &s_default_work_q_handle;

static void _os_work_q_thread(void *arg);
static void _os_work_q_resume(void *arg);

static void _os_work_q_thread(void *arg)
{
    os_work_q_list_t *list = arg;
    for (;;)
    {
        k_tick_t tick = k_work_q_handler(&list->work_q_handle);
        os_sem_take(&list->sem_handle, OS_TicksToMSecs(tick));
    }
}

static void _os_work_q_resume(void *arg)
{
    os_sem_release(arg);
}

os_state os_work_q_create(os_work_q_t *work_q_handle, // 队列句柄
                          const char *name,           // 队列名
                          size_t stack_size,          // 栈大小（字节）
                          os_priority priority        // 优先级（0 为最低，CONFIG_OS_MAX_PRIORITY 为最高。若输入为负值则表示优先级为 (CONFIG_OS_MAX_PRIORITY + 1 + priority)）
)
{
    os_work_q_list_t *list = NULL;
    os_state ret;

    do
    {
        if (os_work_q_is_valid(work_q_handle) != false)
        {
            OS_WRN("work_q is initialized");
            ret = OS_FAIL;
            break;
        }

        list = os_malloc(sizeof(*list));
        if (list == NULL)
        {
            ret = OS_E_NOMEM;
            break;
        }
        else
        {
            memset(list, 0, sizeof(*list));
        }

        if (k_work_q_create(&list->work_q_handle) != 0)
        {
            ret = OS_E_NOMEM;
            break;
        }

        ret = os_sem_create(&list->sem_handle, 0, 1);
        if (ret != OS_OK)
        {
            break;
        }

        ret = os_thread_create(&list->thread,
                               name,
                               _os_work_q_thread,
                               list,
                               stack_size,
                               priority);
        if (ret != OS_OK)
        {
            break;
        }

        if (work_q_handle)
        {
            work_q_handle->handle = list;
        }

        struct os_thread_handle *thread_handle = list->thread.handle;
        thread_handle->work_q_list = list;

        k_work_q_resume_regist(&list->work_q_handle, _os_work_q_resume, &list->sem_handle);

        ret = OS_OK;

    } while (0);

    if (ret != OS_OK)
    {
        if (list)
        {
            if (list->thread.handle)
                os_thread_delete(&list->thread);
            if (list->sem_handle.handle)
                os_sem_delete(&list->sem_handle);
            if (list->work_q_handle.hdl)
                k_work_q_delete(&list->work_q_handle);
            os_free(list);
        }
    }

    return ret;
}

os_state os_work_q_delete(os_work_q_t *work_q_handle)
{
    OS_ASS_ISR();

    os_state ret;

    do
    {
        if (os_work_q_is_valid(work_q_handle) == false)
        {
            OS_WRN("work_q is invalid");
            ret = OS_FAIL;
            break;
        }

        os_scheduler_suspend();

        os_work_q_list_t *list = work_q_handle->handle;
        if (list)
        {
            os_thread_delete(&list->thread);
            os_sem_delete(&list->sem_handle);
            k_work_q_delete(&list->work_q_handle);
            os_free(list);
            work_q_handle->handle = NULL;
            ret = OS_OK;
        }
        else
        {
            OS_ERR("err %p", work_q_handle);
            ret = OS_E_PARAM;
        }

        os_scheduler_resume();

    } while (0);

    return ret;
}

bool os_work_q_is_valid(os_work_q_t *work_q_handle)
{
    bool ret;

    if (work_q_handle == NULL || work_q_handle->handle == NULL)
    {
        ret = false;
    }
    else
    {
        ret = k_work_q_is_valid(work_q_handle->handle);
    }

    return ret;
}

bool os_work_q_delayed_state(os_work_q_t *work_q_handle)
{
    bool ret;

    if (os_work_q_is_valid(work_q_handle) == false)
    {
        OS_WRN("work_q is invalid");
        ret = false;
    }
    else
    {
        ret = k_work_q_delayed_state(work_q_handle->handle);
    }

    return ret;
}

bool os_work_q_ready_state(os_work_q_t *work_q_handle)
{
    bool ret;

    if (os_work_q_is_valid(work_q_handle) == false)
    {
        OS_WRN("work_q is invalid");
        ret = false;
    }
    else
    {
        ret = k_work_q_ready_state(work_q_handle->handle);
    }

    return ret;
}

os_state os_work_create(os_work_t *work_handle, const char *name, os_work_fn work_route, void *arg, uint8_t sub_prior)
{
    OS_ASS_ISR();
    k_err_t ret = k_work_create((k_work_t *)work_handle, name, (k_work_fn)work_route, arg, sub_prior);

    if (ret == 0)
    {
        return OS_OK;
    }
    else
    {
        return OS_E_NOMEM;
    }
}

void os_work_delete(os_work_t *work_handle)
{
    OS_ASS_ISR();
    k_work_delete((k_work_t *)work_handle);
}

bool os_work_is_valid(os_work_t *work_handle)
{
    bool ret = k_work_is_valid((k_work_t *)work_handle);
    return ret;
}

bool os_work_is_pending(os_work_t *work_handle)
{
    bool ret = k_work_is_pending((k_work_t *)work_handle);
    return ret;
}

os_time_t os_work_time_remain(os_work_t *work_handle)
{
    k_tick_t ret = k_work_time_remain((k_work_t *)work_handle);
    return os_calc_ticks_to_msec(ret);
}

void os_work_submit(os_work_q_t *work_q_handle, os_work_t *work_handle, os_time_t delay_ms)
{
    if (os_work_q_is_valid(work_q_handle) == false)
    {
        OS_WRN("work_q is invalid");
    }
    else
    {
        k_work_submit(work_q_handle->handle, (k_work_t *)work_handle, os_calc_msec_to_ticks(delay_ms));
    }
}

void os_work_resume(os_work_t *work_handle, os_time_t delay_ms)
{
    k_work_resume((k_work_t *)work_handle, os_calc_msec_to_ticks(delay_ms));
}

void os_work_suspend(os_work_t *work_handle)
{
    k_work_suspend((k_work_t *)work_handle);
}

void os_work_yield(os_time_t ms)
{
    k_work_yield(os_calc_msec_to_ticks(ms));
}

void os_work_sleep(os_time_t ms)
{
    k_work_sleep(os_calc_msec_to_ticks(ms));
}

void os_work_later(os_time_t ms)
{
    k_work_later(os_calc_msec_to_ticks(ms));
}

void os_work_later_until(os_time_t ms)
{
    k_work_later_until(os_calc_msec_to_ticks(ms));
}

#include "os/os.h"
#include "os_util.h"

#include "k_kit.h"
#include "os/os_semaphore.h"
#include "queue.h"

#include <stdarg.h>

static void *_os_wait_memory(void *(*fn)(void *func_handle, va_list arp), void *func_handle, os_sem_t *sem, os_time_t wait_ms, ...);
static void *_os_ipc_fifo_take(void *fifo_handle, va_list arp);
static void *_os_ipc_queue_recv(void *queue_handle, va_list arp);
static void *_os_ipc_queue_send(void *queue_handle, va_list arp);
static void *_os_ipc_queue_alloc(void *queue_handle, va_list arp);
static void *_os_ipc_queue_take(void *queue_handle, va_list arp);

struct os_fifo_q_handle
{
    k_fifo_t handle;
    os_sem_t sem_take;
};

os_state os_fifo_q_create(os_fifo_t *fifo_handle)
{
    os_state ret;

    do
    {
        struct os_fifo_q_handle *fifo = os_malloc(sizeof(*fifo));
        if (fifo == NULL)
        {
            fifo_handle->handle = NULL;
            ret = OS_E_NOMEM;
            break;
        }

        memset(fifo, 0, sizeof(*fifo));

        if (k_fifo_q_create(&fifo->handle) != 0)
        {
            os_free(fifo);
            ret = OS_E_NOMEM;
            break;
        }

        os_sem_create(&fifo->sem_take, 0, 1);
        fifo_handle->handle = fifo;

        ret = OS_OK;

    } while (0);

    return ret;
}

os_state os_fifo_q_delete(os_fifo_t *fifo_handle)
{
    OS_ASS_HDL(os_fifo_q_is_valid(fifo_handle), fifo_handle->handle);

    struct os_fifo_q_handle *fifo = fifo_handle->handle;

    os_scheduler_suspend();
    os_sem_delete(&fifo->sem_take);
    k_fifo_q_delete(&fifo->handle);
    os_free(fifo);
    fifo_handle->handle = NULL;
    os_scheduler_resume();

    return OS_OK;
}

os_state os_fifo_q_clr(os_fifo_t *fifo_handle)
{
    OS_ASS_HDL(os_fifo_q_is_valid(fifo_handle), fifo_handle->handle);

    struct os_fifo_q_handle *fifo = fifo_handle->handle;
    k_fifo_q_clr(&fifo->handle);

    return OS_OK;
}

static void *_os_wait_memory(void *(*fn)(void *func_handle, va_list arp), void *func_handle, os_sem_t *sem, os_time_t wait_ms, ...)
{
    va_list arp;
    va_start(arp, wait_ms);
    void *ret = fn(func_handle, arp);
    va_end(arp);

    if (ret == NULL && os_scheduler_is_running() != false && !os_is_isr_context())
    {
        TickType_t wait_ticks = os_calc_msec_to_ticks(wait_ms);
        int wakeup_tick = os_get_sys_ticks() + wait_ticks;

        if (wait_ms)
        {
            while (wait_ticks == portMAX_DELAY || (int)(wakeup_tick - os_get_sys_ticks()) > 0)
            {
                os_sem_take(sem, wait_ms);

                va_start(arp, wait_ms);
                ret = fn(func_handle, arp);
                va_end(arp);

                if (ret)
                {
                    os_sem_release(sem);
                    break;
                }

                wait_ms = wakeup_tick - os_get_sys_ticks();
            }
        }

        if (ret == NULL)
        {
            OS_DBG("waiting semaphore %p timeout", sem);
        }
    }

    return ret;
}

void os_fifo_q_regist(os_fifo_t *fifo_handle, os_work_t *work_handle, int delay_ticks)
{
    struct os_fifo_q_handle *fifo = fifo_handle->handle;
    k_fifo_q_regist(&fifo->handle, (k_work_t *)work_handle, delay_ticks);
}

void os_fifo_q_unregist(os_fifo_t *fifo_handle)
{
    struct os_fifo_q_handle *fifo = fifo_handle->handle;
    k_fifo_q_unregist(&fifo->handle);
}

void *os_fifo_alloc(size_t size)
{
    OS_ASS_ISR();
    return k_fifo_alloc(size);
}

os_state os_fifo_free(void *fifo_data)
{
    os_state ret;
    if (k_fifo_free(fifo_data) == 0)
    {
        ret = OS_OK;
    }
    else
    {
        ret = OS_E_PARAM;
    }

    return ret;
}

os_state os_fifo_put(os_fifo_t *fifo_handle, void *fifo_data)
{
    OS_ASS_HDL(os_fifo_q_is_valid(fifo_handle), fifo_handle->handle);

    struct os_fifo_q_handle *fifo = fifo_handle->handle;
    os_state ret;

    if (k_fifo_put(&fifo->handle, fifo_data) == 0)
    {
        os_sem_release(&fifo->sem_take);
        ret = OS_OK;
    }
    else
    {
        ret = OS_E_PARAM;
    }

    return ret;
}

static void *_os_ipc_fifo_take(void *fifo_handle, va_list arp)
{
    return k_fifo_take(fifo_handle);
}

void *os_fifo_take(os_fifo_t *fifo_handle, os_time_t wait_ms)
{
    OS_ASS_HDL(os_fifo_q_is_valid(fifo_handle), fifo_handle->handle);

    struct os_fifo_q_handle *fifo = fifo_handle->handle;
    void *ret = _os_wait_memory(_os_ipc_fifo_take, &fifo->handle, &fifo->sem_take, wait_ms);
    return ret;
}

void *os_fifo_peek_head(os_fifo_t *fifo_handle)
{
    OS_ASS_HDL(os_fifo_q_is_valid(fifo_handle), fifo_handle->handle);
    struct os_fifo_q_handle *fifo = fifo_handle->handle;
    void *ret = k_fifo_peek_head(&fifo->handle);
    return ret;
}

void *os_fifo_peek_tail(os_fifo_t *fifo_handle)
{
    OS_ASS_HDL(os_fifo_q_is_valid(fifo_handle), fifo_handle->handle);
    struct os_fifo_q_handle *fifo = fifo_handle->handle;
    void *ret = k_fifo_peek_tail(&fifo->handle);
    return ret;
}

bool os_fifo_q_is_valid(os_fifo_t *fifo_handle)
{
    if (fifo_handle && fifo_handle->handle)
    {
        return true;
    }
    else
    {
        return false;
    }
}

struct os_queue_handle
{
    k_queue_t handle;
    os_sem_t sem_send;
    os_sem_t sem_recv;
};

os_state os_queue_create(os_queue_t *queue_handle, size_t queue_length, size_t item_size)
{
    os_state ret;

    do
    {
        struct os_queue_handle *queue = os_malloc(sizeof(*queue));
        if (queue == NULL)
        {
            queue_handle->handle = NULL;
            ret = OS_E_NOMEM;
            break;
        }

        memset(queue, 0, sizeof(*queue));

        if (k_queue_create(&queue->handle, queue_length, item_size) != 0)
        {
            os_free(queue);
            ret = OS_E_NOMEM;
            break;
        }

        os_sem_create(&queue->sem_recv, 0, 1);
        os_sem_create(&queue->sem_send, 0, 1);
        queue_handle->handle = queue;

        ret = OS_OK;

    } while (0);

    return ret;
}

os_state os_queue_delete(os_queue_t *queue_handle)
{
    OS_ASS_HDL(os_queue_is_valid(queue_handle), queue_handle->handle);

    struct os_queue_handle *queue = queue_handle->handle;

    os_scheduler_suspend();
    os_sem_delete(&queue->sem_recv);
    os_sem_delete(&queue->sem_send);
    k_queue_delete(&queue->handle);
    os_free(queue);
    queue_handle->handle = NULL;
    os_scheduler_resume();

    return OS_OK;
}

os_state os_queue_clr(os_queue_t *queue_handle)
{
    OS_ASS_HDL(os_queue_is_valid(queue_handle), queue_handle->handle);

    struct os_queue_handle *queue = queue_handle->handle;
    k_queue_clr(&queue->handle);

    return OS_OK;
}

void os_queue_regist(os_queue_t *queue_handle, os_work_t *work_handle, int delay_ticks)
{
    struct os_queue_handle *queue = queue_handle->handle;
    k_queue_regist(&queue->handle, (k_work_t *)work_handle, delay_ticks);
}

void os_queue_unregist(os_queue_t *queue_handle)
{
    struct os_queue_handle *queue = queue_handle->handle;
    k_queue_unregist(&queue->handle);
}

static void *_os_ipc_queue_recv(void *queue_handle, va_list arp)
{
    void *dst = va_arg(arp, void *);
    if (k_queue_recv(queue_handle, dst) == 0)
    {
        return (void *)!NULL;
    }
    else
    {
        return NULL;
    }
}

os_state os_queue_recv(os_queue_t *queue_handle, void *dst, os_time_t wait_ms)
{
    OS_ASS_HDL(os_queue_is_valid(queue_handle), queue_handle->handle);

    struct os_queue_handle *queue = queue_handle->handle;
    os_state ret;
    if (_os_wait_memory(_os_ipc_queue_recv, &queue->handle, &queue->sem_recv, wait_ms, dst) == NULL)
    {
        ret = OS_E_TIMEOUT;
    }
    else
    {
        os_sem_release(&queue->sem_send);
        ret = OS_OK;
    }

    return ret;
}

static void *_os_ipc_queue_send(void *queue_handle, va_list arp)
{
    const void *src = va_arg(arp, void *);
    if (k_queue_send(queue_handle, src) == 0)
    {
        return (void *)!NULL;
    }
    else
    {
        return NULL;
    }
}

os_state os_queue_send(os_queue_t *queue_handle, const void *src, os_time_t wait_ms)
{
    OS_ASS_HDL(os_queue_is_valid(queue_handle), queue_handle->handle);

    struct os_queue_handle *queue = queue_handle->handle;
    os_state ret;

    if (_os_wait_memory(_os_ipc_queue_send, &queue->handle, &queue->sem_send, wait_ms, src) == NULL)
    {
        ret = OS_E_TIMEOUT;
    }
    else
    {
        os_sem_release(&queue->sem_recv);
        ret = OS_OK;
    }

    return ret;
}

static void *_os_ipc_queue_alloc(void *queue_handle, va_list arp)
{
    return k_queue_alloc(queue_handle);
}

void *os_queue_alloc(os_queue_t *queue_handle, os_time_t wait_ms)
{
    OS_ASS_HDL(os_queue_is_valid(queue_handle), queue_handle->handle);

    struct os_queue_handle *queue = queue_handle->handle;
    void *ret = _os_wait_memory(_os_ipc_queue_alloc, &queue->handle, &queue->sem_send, wait_ms);

    return ret;
}

os_state os_queue_free(void *queue_data)
{
    os_state ret;

    if (k_queue_free(queue_data) == 0)
    {
        k_queue_t *handle = k_queue_read_handle(queue_data);
        struct os_queue_handle *queue = OS_CONTAINER_OF(handle, struct os_queue_handle, handle);
        os_sem_release(&queue->sem_send);
        ret = OS_OK;
    }
    else
    {
        ret = OS_E_PARAM;
    }

    return ret;
}

os_state os_queue_put(void *queue_data)
{
    os_state ret;

    if (k_queue_put(queue_data) == 0)
    {
        k_queue_t *handle = k_queue_read_handle(queue_data);
        struct os_queue_handle *queue = OS_CONTAINER_OF(handle, struct os_queue_handle, handle);
        os_sem_release(&queue->sem_recv);
        ret = OS_OK;
    }
    else
    {
        ret = OS_E_PARAM;
    }

    return ret;
}

static void *_os_ipc_queue_take(void *queue_handle, va_list arp)
{
    return k_queue_take(queue_handle);
}

void *os_queue_take(os_queue_t *queue_handle, os_time_t wait_ms)
{
    OS_ASS_HDL(os_queue_is_valid(queue_handle), queue_handle->handle);

    struct os_queue_handle *queue = queue_handle->handle;
    void *ret = _os_wait_memory(_os_ipc_queue_take, &queue->handle, &queue->sem_recv, wait_ms);

    return ret;
}

void *os_queue_peek_head(os_queue_t *queue_handle)
{
    OS_ASS_HDL(os_queue_is_valid(queue_handle), queue_handle->handle);
    struct os_queue_handle *queue = queue_handle->handle;
    void *ret = k_queue_peek_head(&queue->handle);
    return ret;
}

void *os_queue_peek_tail(os_queue_t *queue_handle)
{
    OS_ASS_HDL(os_queue_is_valid(queue_handle), queue_handle->handle);
    struct os_queue_handle *queue = queue_handle->handle;
    void *ret = k_queue_peek_tail(&queue->handle);
    return ret;
}

size_t os_queue_get_item_size(os_queue_t *queue_handle)
{
    struct os_queue_handle *queue = queue_handle->handle;
    size_t ret = k_queue_get_item_size(&queue->handle);
    return ret;
}

bool os_queue_is_valid(os_queue_t *queue_handle)
{
    if (queue_handle && queue_handle->handle)
    {
        return true;
    }
    else
    {
        return false;
    }
}

struct os_pipe_handle
{
    k_pipe_t handle;
};

os_state os_pipe_create(os_pipe_t *pipe_handle, size_t pipe_size)
{
    os_state ret;

    do
    {
        struct os_pipe_handle *pipe = os_malloc(sizeof(*pipe));
        if (pipe == NULL)
        {
            pipe_handle->handle = NULL;
            ret = OS_E_NOMEM;
        }

        memset(pipe, 0, sizeof(*pipe));

        if (k_pipe_create(&pipe->handle, pipe_size) != 0)
        {
            os_free(pipe);
            ret = OS_E_NOMEM;
        }

        pipe_handle->handle = pipe;

        ret = OS_OK;

    } while (0);

    return ret;
}

os_state os_pipe_delete(os_pipe_t *pipe_handle)
{
    OS_ASS_HDL(os_pipe_is_valid(pipe_handle), pipe_handle->handle);

    struct os_pipe_handle *pipe = pipe_handle->handle;

    k_pipe_delete(&pipe->handle);
    os_free(pipe);
    pipe_handle->handle = NULL;

    return OS_OK;
}

os_state os_pipe_clr(os_pipe_t *pipe_handle)
{
    OS_ASS_HDL(os_pipe_is_valid(pipe_handle), pipe_handle->handle);

    struct os_pipe_handle *pipe = pipe_handle->handle;
    k_pipe_clr(&pipe->handle);

    return OS_OK;
}

void os_pipe_regist(os_pipe_t *pipe_handle, os_work_t *work_handle, int delay_ticks)
{
    struct os_pipe_handle *pipe = pipe_handle->handle;
    k_pipe_regist(&pipe->handle, (k_work_t *)work_handle, delay_ticks);
}

void os_pipe_unregist(os_pipe_t *pipe_handle)
{
    struct os_pipe_handle *pipe = pipe_handle->handle;
    k_pipe_unregist(&pipe->handle);
}

size_t os_pipe_poll_write(os_pipe_t *pipe_handle, uint8_t data)
{
    struct os_pipe_handle *pipe = pipe_handle->handle;
    size_t ret = k_pipe_poll_write(&pipe->handle, data);
    return ret;
}

size_t os_pipe_fifo_fill(os_pipe_t *pipe_handle, const void *data, size_t size)
{
    struct os_pipe_handle *pipe = pipe_handle->handle;
    size_t ret = k_pipe_fifo_fill(&pipe->handle, data, size);
    return ret;
}

size_t os_pipe_poll_read(os_pipe_t *pipe_handle, uint8_t *data)
{
    struct os_pipe_handle *pipe = pipe_handle->handle;
    size_t ret = k_pipe_poll_read(&pipe->handle, data);
    return ret;
}

size_t os_pipe_fifo_read(os_pipe_t *pipe_handle, void *data, size_t size)
{
    struct os_pipe_handle *pipe = pipe_handle->handle;
    size_t ret = k_pipe_fifo_read(&pipe->handle, data, size);
    return ret;
}

bool os_pipe_is_ne(os_pipe_t *pipe_handle)
{
    struct os_pipe_handle *pipe = pipe_handle->handle;
    bool ret = k_pipe_is_ne(&pipe->handle);
    return ret;
}

size_t os_pipe_get_valid_size(os_pipe_t *pipe_handle)
{
    struct os_pipe_handle *pipe = pipe_handle->handle;
    size_t ret = k_pipe_get_valid_size(&pipe->handle);
    return ret;
}

size_t os_pipe_get_empty_size(os_pipe_t *pipe_handle)
{
    struct os_pipe_handle *pipe = pipe_handle->handle;
    size_t ret = k_pipe_get_empty_size(&pipe->handle);
    return ret;
}

void os_pipe_peek_valid(os_pipe_t *pipe_handle, void **dst_base, size_t *dst_size)
{
    struct os_pipe_handle *pipe = pipe_handle->handle;
    k_pipe_peek_valid(&pipe->handle, dst_base, dst_size);
}

void os_pipe_peek_empty(os_pipe_t *pipe_handle, void **dst_base, size_t *dst_size)
{
    struct os_pipe_handle *pipe = pipe_handle->handle;
    k_pipe_peek_empty(&pipe->handle, dst_base, dst_size);
}

bool os_pipe_is_valid(os_pipe_t *pipe_handle)
{
    if (pipe_handle && pipe_handle->handle)
    {
        return true;
    }
    else
    {
        return false;
    }
}
