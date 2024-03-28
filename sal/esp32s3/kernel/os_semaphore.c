#include "os/os.h"
#include "os_util.h"
#include "semphr.h"

os_state os_sem_create(os_sem_t *sem, size_t init_value, size_t max_value)
{
    OS_ASS_HDL(!os_sem_is_valid(sem), sem->handle);

    sem->handle = xSemaphoreCreateCounting(max_value, init_value);
    if (sem->handle == NULL)
    {
        OS_ERR("err %p\r\n", sem->handle);
        return OS_FAIL;
    }

    return OS_OK;
}

os_state os_sem_delete(os_sem_t *sem)
{
    OS_ASS_HDL(os_sem_is_valid(sem), sem->handle);

    vSemaphoreDelete(sem->handle);
    sem->handle = NULL;
    return OS_OK;
}

os_state os_sem_take(os_sem_t *sem, os_time_t wait_ms)
{
    BaseType_t ret;
    BaseType_t taskWoken;

    OS_ASS_HDL(os_sem_is_valid(sem), sem->handle);

    if (os_is_isr_context())
    {
        if (wait_ms != 0)
        {
            OS_ERR("%s() in ISR, wait %u ms\n", __func__, wait_ms);
            return OS_E_ISR;
        }
        taskWoken = pdFALSE;
        ret = xSemaphoreTakeFromISR(sem->handle, &taskWoken);
        if (ret != pdPASS)
        {
            OS_DBG("%s() fail @ %d\n", __func__, __LINE__);
            return OS_E_TIMEOUT;
        }
        portYIELD_FROM_ISR(taskWoken);
    }
    else
    {
        ret = xSemaphoreTake(sem->handle, os_calc_msec_to_ticks(wait_ms));
        if (ret != pdPASS)
        {
            OS_DBG("%s() fail @ %d, %u ms\n", __func__, __LINE__, wait_ms);
            return OS_E_TIMEOUT;
        }
    }

    return OS_OK;
}

os_state os_sem_release(os_sem_t *sem)
{
    BaseType_t ret;
    BaseType_t taskWoken;

    OS_ASS_HDL(os_sem_is_valid(sem), sem->handle);

    if (os_is_isr_context())
    {
        taskWoken = pdFALSE;
        ret = xSemaphoreGiveFromISR(sem->handle, &taskWoken);
        if (ret != pdPASS)
        {
            OS_DBG("%s() fail @ %d\n", __func__, __LINE__);
            return OS_FAIL;
        }
        portYIELD_FROM_ISR(taskWoken);
    }
    else
    {
        ret = xSemaphoreGive(sem->handle);
        if (ret != pdPASS)
        {
            OS_DBG("%s() fail @ %d\n", __func__, __LINE__);
            return OS_FAIL;
        }
    }

    return OS_OK;
}

bool os_sem_is_valid(os_sem_t *sem)
{
    if (sem && sem->handle)
    {
        return true;
    }
    else
    {
        return false;
    }
}
