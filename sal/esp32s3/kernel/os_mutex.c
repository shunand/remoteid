#include "os/os.h"
#include "os_util.h"
#include "semphr.h"

os_state os_mutex_create(os_mutex_t *mutex)
{
    OS_ASS_HDL(!os_mutex_is_valid(mutex), mutex->handle);

    mutex->handle = xSemaphoreCreateMutex();
    if (mutex->handle == NULL)
    {
        OS_ERR("err %p\r\n", mutex->handle);
        return OS_FAIL;
    }

    return OS_OK;
}

os_state os_mutex_delete(os_mutex_t *mutex)
{
    OS_ASS_HDL(os_mutex_is_valid(mutex), mutex->handle);

    vSemaphoreDelete(mutex->handle);
    mutex->handle = NULL;
    return OS_OK;
}

os_state os_mutex_lock(os_mutex_t *mutex, os_time_t wait_ms)
{
    BaseType_t ret;

    OS_ASS_HDL(os_mutex_is_valid(mutex), mutex->handle);

    ret = xSemaphoreTake(mutex->handle, os_calc_msec_to_ticks(wait_ms));
    if (ret != pdPASS)
    {
        OS_DBG("%s() fail @ %d, %u ms\n", __func__, __LINE__, wait_ms);
        return OS_FAIL;
    }

    return OS_OK;
}

os_state os_mutex_unlock(os_mutex_t *mutex)
{
    BaseType_t ret;

    OS_ASS_HDL(os_mutex_is_valid(mutex), mutex->handle);

    ret = xSemaphoreGive(mutex->handle);
    if (ret != pdPASS)
    {
        OS_DBG("%s() fail @ %d\n", __func__, __LINE__);
        return OS_FAIL;
    }

    return OS_OK;
}

os_thread_handle_t os_mutex_get_holder(os_mutex_t *mutex)
{
    if (!os_mutex_is_valid(mutex))
    {
        return OS_INVALID_HANDLE;
    }

    return (os_thread_handle_t)xSemaphoreGetMutexHolder(mutex->handle);
}

bool os_mutex_is_valid(os_mutex_t *mutex)
{
    if (mutex && mutex->handle)
    {
        return true;
    }
    else
    {
        return false;
    }
}
