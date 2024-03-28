/**
 * @file os_semaphore.h
 * @author LokLiang (lokliang@163.com)
 * @brief
 * @version 0.1
 * @date 2023-05-01
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef __OS_SEMAPHORE_H__
#define __OS_SEMAPHORE_H__

#include "os/os_common.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef os_handle_t os_sem_handle_t;

typedef struct
{
    os_sem_handle_t handle;
} os_sem_t;

os_state os_sem_create(os_sem_t *sem, size_t init_value, size_t max_value);
os_state os_sem_delete(os_sem_t *sem);

os_state os_sem_take(os_sem_t *sem, os_time_t wait_ms);
os_state os_sem_release(os_sem_t *sem);

bool os_sem_is_valid(os_sem_t *sem);

#ifdef __cplusplus
}
#endif

#endif /* __OS_SEMAPHORE_H__ */
