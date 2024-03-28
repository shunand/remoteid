/**
 * @file os_ipc.h
 * @author LokLiang (lokliang@163.com)
 * @brief
 * @version 0.1
 * @date 2023-05-01
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef __OS_IPC_H__
#define __OS_IPC_H__

#include "os/os_work.h"

#ifdef __cplusplus
extern "C"
{
#endif /* #ifdef __cplusplus */

typedef os_handle_t os_fifo_handle_t;

typedef struct
{
    os_fifo_handle_t handle;
} os_fifo_t;

os_state os_fifo_q_create(os_fifo_t *fifo_handle);
os_state os_fifo_q_delete(os_fifo_t *fifo_handle);
os_state os_fifo_q_clr(os_fifo_t *fifo_handle);

void os_fifo_q_regist(os_fifo_t *fifo_handle, os_work_t *work_handle, int delay_ticks);
void os_fifo_q_unregist(os_fifo_t *fifo_handle);

void    *os_fifo_alloc(size_t size);
os_state os_fifo_put(os_fifo_t *fifo_handle, void *fifo_data);

void    *os_fifo_take(os_fifo_t *fifo_handle, os_time_t wait_ms);
os_state os_fifo_free(void *fifo_data);

void *os_fifo_peek_head(os_fifo_t *fifo_handle);
void *os_fifo_peek_tail(os_fifo_t *fifo_handle);

bool os_fifo_q_is_valid(os_fifo_t *fifo_handle);

typedef os_handle_t os_queue_handle_t;

typedef struct
{
    os_queue_handle_t handle;
} os_queue_t;

os_state os_queue_create(os_queue_t *queue_handle, size_t queueLen, size_t itemSize);
os_state os_queue_delete(os_queue_t *queue_handle);
os_state os_queue_clr(os_queue_t *queue_handle);

void os_queue_regist(os_queue_t *queue_handle, os_work_t *work_handle, int delay_ticks);
void os_queue_unregist(os_queue_t *queue_handle);

os_state os_queue_send(os_queue_t *queue_handle, const void *item, os_time_t wait_ms);
os_state os_queue_recv(os_queue_t *queue_handle, void *item, os_time_t wait_ms);

void *os_queue_peek_head(os_queue_t *queue_handle);
void *os_queue_peek_tail(os_queue_t *queue_handle);

size_t os_queue_get_item_size(os_queue_t *queue_handle);

bool os_queue_is_valid(os_queue_t *queue_handle);

typedef os_handle_t os_pipe_handle_t;

typedef struct
{
    os_pipe_handle_t handle;
} os_pipe_t;

os_state os_pipe_create(os_pipe_t *pipe_handle, size_t pipe_size);
os_state os_pipe_delete(os_pipe_t *pipe_handle);
os_state os_pipe_clr(os_pipe_t *pipe_handle);

void os_pipe_regist(os_pipe_t *pipe_handle, os_work_t *work_handle, int delay_ticks);
void os_pipe_unregist(os_pipe_t *pipe_handle);

size_t os_pipe_poll_write(os_pipe_t *pipe_handle, uint8_t data);
size_t os_pipe_fifo_fill(os_pipe_t *pipe_handle, const void *data, size_t size);
size_t os_pipe_poll_read(os_pipe_t *pipe_handle, uint8_t *data);
size_t os_pipe_fifo_read(os_pipe_t *pipe_handle, void *data, size_t size);

bool   os_pipe_is_ne(os_pipe_t *pipe_handle);
size_t os_pipe_get_valid_size(os_pipe_t *pipe_handle);
size_t os_pipe_get_empty_size(os_pipe_t *pipe_handle);

void os_pipe_peek_valid(os_pipe_t *pipe_handle, void **dst_base, size_t *dst_size);
void os_pipe_peek_empty(os_pipe_t *pipe_handle, void **dst_base, size_t *dst_size);

bool os_pipe_is_valid(os_pipe_t *pipe_handle);

#ifdef __cplusplus
} /* extern "C" */
#endif /* #ifdef __cplusplus */

#endif /* __OS_IPC_H__ */
