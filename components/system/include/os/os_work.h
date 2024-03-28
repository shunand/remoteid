/**
 * @file os_work.h
 * @author LokLiang (lokliang@163.com)
 * @brief
 * @version 0.1
 * @date 2023-05-01
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef __OS_WORK_H__
#define __OS_WORK_H__

#include "os/os_common.h"

#ifdef __cplusplus
extern "C"
{
#endif /* #ifdef __cplusplus */

typedef os_handle_t os_work_q_handle_t;

typedef struct
{
    os_work_q_handle_t handle;
} os_work_q_t;

extern os_work_q_t *default_os_work_q_hdl;

os_state os_work_q_create(os_work_q_t *work_q_handle,
                          const char *name,
                          size_t stack_size,
                          os_priority priority);

os_state os_work_q_delete(os_work_q_t *work_q_handle);

bool os_work_q_is_valid(os_work_q_t *work_q_handle);
bool os_work_q_delayed_state(os_work_q_t *work_q_handle);
bool os_work_q_ready_state(os_work_q_t *work_q_handle);

typedef os_handle_t os_work_handle_t;

typedef struct
{
    os_work_handle_t handle;
} os_work_t;

typedef void (*os_work_fn)(void *arg);

os_state os_work_create(os_work_t *work_handle, const char *name, os_work_fn work_route, void *arg, uint8_t sub_prior);
void     os_work_delete(os_work_t *work_handle);

bool      os_work_is_valid(os_work_t *work_handle);
bool      os_work_is_pending(os_work_t *work_handle);
os_time_t os_work_time_remain(os_work_t *work_handle);

void os_work_submit(os_work_q_t *work_q_handle, os_work_t *work_handle, os_time_t delay_ms);
void os_work_resume(os_work_t *work_handle, os_time_t delay_ms);
void os_work_suspend(os_work_t *work_handle);

void os_work_yield(os_time_t ms);
void os_work_sleep(os_time_t ms);
void os_work_later(os_time_t ms);
void os_work_later_until(os_time_t ms);

#ifdef __cplusplus
} /* extern "C" */
#endif /* #ifdef __cplusplus */

#endif
