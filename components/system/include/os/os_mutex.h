/**
 * @file os_mutex.h
 * @author LokLiang (lokliang@163.com)
 * @brief
 * @version 0.1
 * @date 2023-05-01
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef __OS_MUTEX_H__
#define __OS_MUTEX_H__

#include "os/os_common.h"
#include "os/os_thread.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef os_handle_t os_mutex_handle_t;

typedef struct
{
    os_mutex_handle_t handle;
} os_mutex_t;

os_state os_mutex_create(os_mutex_t *mutex);
os_state os_mutex_delete(os_mutex_t *mutex);

os_state os_mutex_lock(os_mutex_t *mutex, os_time_t wait_ms);
os_state os_mutex_unlock(os_mutex_t *mutex);

os_thread_handle_t os_mutex_get_holder(os_mutex_t *mutex);

bool os_mutex_is_valid(os_mutex_t *mutex);

#ifdef __cplusplus
}
#endif

#endif /* __OS_MUTEX_H__ */
