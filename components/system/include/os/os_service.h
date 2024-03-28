/**
 * @file os_service.h
 * @author LokLiang (lokliang@163.com)
 * @brief
 * @version 0.1
 * @date 2023-05-01
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef __OS_SERVER_H__
#define __OS_SERVER_H__

#include "os/os_common.h"

#ifdef __cplusplus
extern "C"
{
#endif

int os_start(void *heap_mem, size_t heap_size);

void os_int_entry(void);
void os_int_exit(void);

bool os_is_isr_context(void);

void os_interrupt_disable(void);
void os_interrupt_enable(void);

void os_scheduler_suspend(void);
void os_scheduler_resume(void);

bool os_scheduler_is_running(void);

void os_sys_print_info(void);

os_time_t os_get_sys_time(void);

size_t os_get_sys_ticks(void);

os_time_t os_calc_ticks_to_msec(size_t ticks);
size_t os_calc_msec_to_ticks(os_time_t msec);

size_t os_cpu_usage(void);

int os_get_err(void);

void os_set_err(int err);

#ifdef __cplusplus
}
#endif

#endif /* __OS_SERVER_H__ */
