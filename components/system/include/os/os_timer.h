/**
 * @file os_timer.h
 * @author LokLiang (lokliang@163.com)
 * @brief
 * @version 0.1
 * @date 2023-05-01
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef __OS_TIMER_H__
#define __OS_TIMER_H__

#include "os/os_common.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef os_handle_t os_timer_handle_t;

typedef struct
{
    os_timer_handle_t handle;
} os_timer_t;

typedef enum
{
    OS_TIMER_ONCE = 0,    /* one shot timer */
    OS_TIMER_PERIODIC = 1 /* periodic timer */
} os_timer_type_t;

typedef void (*os_timer_cb_fn)(void *arg);

os_state os_timer_create(os_timer_t *timer, os_timer_cb_fn cb, void *arg);

os_state os_timer_delete(os_timer_t *timer);

os_state os_timer_set_period(os_timer_t *timer, os_timer_type_t type, os_time_t period_ms);

os_state os_timer_start(os_timer_t *timer);

os_state os_timer_stop(os_timer_t *timer);

bool os_timer_is_pending(os_timer_t *timer);

bool os_timer_is_valid(os_timer_t *timer);

#ifdef __cplusplus
}
#endif

#endif /* __OS_TIMER_H__ */
