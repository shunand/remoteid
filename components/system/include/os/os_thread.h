/**
 * @file os_thread.h
 * @author LokLiang (lokliang@163.com)
 * @brief
 * @version 0.1
 * @date 2023-05-01
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef __OS_THREAD_H__
#define __OS_THREAD_H__

#include "os/os_common.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef os_handle_t os_thread_handle_t;

typedef struct
{
    os_thread_handle_t handle;
} os_thread_t;

typedef void (*os_thread_entry_t)(void *);

os_state os_thread_init(os_thread_t *thread,
                          const char *name,
                          os_thread_entry_t entry,
                          void *arg,
                          void *stack_base,
                          size_t stack_size,
                          os_priority priority);

os_state os_thread_create(os_thread_t *thread,
                          const char *name,
                          os_thread_entry_t entry,
                          void *arg,
                          size_t stack_size,
                          os_priority priority);

os_state os_thread_delete(os_thread_t *thread);

void os_thread_sleep(os_time_t msec);

void os_thread_yield(void);

void os_thread_suspend(os_thread_t *thread);

void os_thread_resume(os_thread_t *thread);

os_thread_handle_t os_thread_get_self(void);

const char *os_thread_get_name(os_thread_t *thread);

size_t os_thread_stack_min(os_thread_t *thread);

bool os_thread_is_valid(os_thread_t *thread);

#ifdef __cplusplus
}
#endif

#endif /* __OS_THREAD_H__ */
