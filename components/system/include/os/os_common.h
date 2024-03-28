/**
 * @file os_common.h
 * @author LokLiang (lokliang@163.com)
 * @brief
 * @version 0.1
 * @date 2023-05-01
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef __OS_COMMON_H__
#define __OS_COMMON_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum  {
    OS_PRIORITY_IDLE     = 0,
    OS_PRIORITY_LOWEST   = 1,
    OS_PRIORITY_LOWER    = 2,
    OS_PRIORITY_LOW      = 3,
    OS_PRIORITY_NORMAL   = 4,
    OS_PRIORITY_HIGH     = 5,
    OS_PRIORITY_HIGHER   = 6,
    OS_PRIORITY_HIGHEST  = 7
} os_priority;

typedef enum {
    OS_OK           = 0,    /* success */
    OS_FAIL         = -1,   /* general failure */
    OS_E_NOMEM      = -2,   /* out of memory */
    OS_E_PARAM      = -3,   /* invalid parameter */
    OS_E_TIMEOUT    = -4,   /* operation timeout */
    OS_E_ISR        = -5,   /* not allowed in ISR context */
} os_state;

typedef void * os_handle_t;

typedef size_t os_time_t;

#define OS_INVALID_HANDLE       NULL        /* OS invalid handle */
#define OS_WAIT_FOREVER         0xffffffffU /* Wait forever timeout value */
#define OS_SEMAPHORE_MAX_COUNT  0xffffffffU /* Maximum count value for semaphore */

#define OS_MSEC_PER_SEC     1000U       /* milliseconds per second */
#define OS_USEC_PER_MSEC    1000U       /* microseconds per millisecond */
#define OS_USEC_PER_SEC     1000000U    /* microseconds per second */

#define OS_TICK (OS_USEC_PER_SEC / OS_TICK_RATE) /* microseconds per tick */

#define OS_SecsToTicks(sec)     ((os_time_t)(sec) * OS_TICK_RATE)
#define OS_MSecsToTicks(msec)   ((os_time_t)(OS_TICK_RATE < OS_MSEC_PER_SEC ? (msec) / (OS_TICK / OS_USEC_PER_MSEC) : (msec) * (OS_USEC_PER_MSEC / OS_TICK)))
#define OS_TicksToMSecs(t)      ((os_time_t)(OS_TICK_RATE < OS_MSEC_PER_SEC ? (t) * (OS_TICK / OS_USEC_PER_MSEC) : (t) / (OS_USEC_PER_MSEC / OS_TICK)))
#define OS_TicksToSecs(t)       ((os_time_t)(t) / (OS_USEC_PER_SEC / OS_TICK))

#define OS_CONTAINER_OF(PTR, TYPE, MEMBER) ((TYPE *)&((uint8_t *)PTR)[-(int)&((TYPE *)0)->MEMBER])

#ifdef __cplusplus
}
#endif

#endif /* __OS_COMMON_H__ */
