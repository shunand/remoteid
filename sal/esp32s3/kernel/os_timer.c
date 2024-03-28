#include "os/os.h"
#include "os_util.h"
#include "timers.h"

/* TODO: what block time should be used ? */
#define OS_TIMER_WAIT_FOREVER portMAX_DELAY
#define OS_TIMER_WAIT_NONE 0

/* Timer private data definition */
typedef struct OS_TimerPriv
{
    TimerHandle_t handle;    /* Timer handle */
    os_timer_cb_fn callback; /* Timer expire callback function */
    void *argument;          /* Argument of timer expire callback function */
} OS_TimerPriv_t;

static void _os_timer_cb(TimerHandle_t xTimer)
{
    OS_TimerPriv_t *priv;

    priv = pvTimerGetTimerID(xTimer);
    if (priv && priv->callback)
    {
        priv->callback(priv->argument);
    }
    else
    {
        OS_WRN("Invalid timer callback\n");
    }
}

os_state os_timer_create(os_timer_t *timer, os_timer_cb_fn cb, void *arg)
{
    OS_TimerPriv_t *priv;

    OS_ASS_HDL(!os_timer_is_valid(timer), timer->handle);

    priv = os_malloc(sizeof(OS_TimerPriv_t));
    if (priv == NULL)
    {
        return OS_E_NOMEM;
    }

    priv->callback = cb;
    priv->argument = arg;
    priv->handle = xTimerCreate("",
                                os_calc_msec_to_ticks(OS_WAIT_FOREVER),
                                pdFALSE,
                                priv,
                                _os_timer_cb);
    if (priv->handle == NULL)
    {
        OS_ERR("err %p\r\n", priv->handle);
        os_free(priv);
        return OS_FAIL;
    }
    timer->handle = priv;
    return OS_OK;
}

static TimerHandle_t _os_timer_get_handle(os_timer_t *timer)
{
    OS_TimerPriv_t *priv = timer->handle;
    return priv->handle;
}

os_state os_timer_delete(os_timer_t *timer)
{
    TimerHandle_t handle;
    int ret;

    OS_ASS_HDL(os_timer_is_valid(timer), timer->handle);

    handle = _os_timer_get_handle(timer);
    ret = xTimerDelete(handle, OS_TIMER_WAIT_FOREVER);
    if (ret != pdPASS)
    {
        OS_ERR("err %d\r\n", ret);
        return OS_FAIL;
    }

    OS_TimerPriv_t *priv = timer->handle;
    timer->handle = NULL;

    os_free(priv);

    return OS_OK;
}

os_state os_timer_start(os_timer_t *timer)
{
    TimerHandle_t handle;
    int ret;
    BaseType_t taskWoken;

    OS_ASS_HDL(os_timer_is_valid(timer), timer->handle);

    handle = _os_timer_get_handle(timer);

    if (os_is_isr_context())
    {
        taskWoken = pdFALSE;
        ret = xTimerStartFromISR(handle, &taskWoken);
        if (ret != pdPASS)
        {
            OS_ERR("err %d\r\n", ret);
            return OS_FAIL;
        }
        portYIELD_FROM_ISR(taskWoken);
    }
    else
    {
        ret = xTimerStart(handle, OS_TIMER_WAIT_NONE);
        if (ret != pdPASS)
        {
            OS_ERR("err %d\r\n", ret);
            return OS_FAIL;
        }
    }

    return OS_OK;
}

os_state os_timer_set_period(os_timer_t *timer, os_timer_type_t type, os_time_t period_ms)
{
    TimerHandle_t handle;
    TickType_t ticks;
    int ret;

    OS_ASS_HDL(os_timer_is_valid(timer), timer->handle);

    handle = _os_timer_get_handle(timer);
    ticks = os_calc_msec_to_ticks(period_ms);

    if (os_is_isr_context())
    {
        BaseType_t taskWoken = pdFALSE;
        ret = xTimerChangePeriodFromISR(handle, ticks, &taskWoken);
        if (ret != pdPASS)
        {
            OS_ERR("err %d\r\n", ret);
            return OS_FAIL;
        }
        portYIELD_FROM_ISR(taskWoken);
    }
    else
    {
        ret = xTimerChangePeriod(handle, ticks, OS_TIMER_WAIT_NONE);
        if (ret != pdPASS)
        {
            OS_ERR("err %d\r\n", ret);
            return OS_FAIL;
        }
    }

    vTimerSetReloadMode(handle, type == OS_TIMER_PERIODIC ? pdTRUE : pdFALSE);

    return OS_OK;
}

os_state os_timer_stop(os_timer_t *timer)
{
    TimerHandle_t handle;
    int ret;
    BaseType_t taskWoken;

    OS_ASS_HDL(os_timer_is_valid(timer), timer->handle);

    handle = _os_timer_get_handle(timer);

    if (os_is_isr_context())
    {
        taskWoken = pdFALSE;
        ret = xTimerStopFromISR(handle, &taskWoken);
        if (ret != pdPASS)
        {
            OS_ERR("err %d\r\n", ret);
            return OS_FAIL;
        }
        portYIELD_FROM_ISR(taskWoken);
    }
    else
    {
        ret = xTimerStop(handle, OS_TIMER_WAIT_FOREVER);
        if (ret != pdPASS)
        {
            OS_ERR("err %d\r\n", ret);
            return OS_FAIL;
        }
    }

    return OS_OK;
}

bool os_timer_is_pending(os_timer_t *timer)
{
    TimerHandle_t handle;

    if (!os_timer_is_valid(timer))
    {
        return 0;
    }

    handle = _os_timer_get_handle(timer);

    return (xTimerIsTimerActive(handle) != pdFALSE);
}

bool os_timer_is_valid(os_timer_t *timer)
{
    if (timer && timer->handle)
    {
        return true;
    }
    else
    {
        return false;
    }
}
