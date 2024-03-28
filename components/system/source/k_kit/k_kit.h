/**
 * @file k_kit.h
 * @author LokLiang (lokliang@163.com)
 * @brief 裸机版微内核
 * @version 1.1.0
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef __K_KIT_H__
#define __K_KIT_H__

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef int k_err_t;
typedef int k_tick_t;

typedef void* k_hdl_t;

typedef struct
{
    k_hdl_t hdl;
} k_work_q_t;

typedef struct
{
    void *(*malloc)(size_t);             // 执行申请内存（内部已通过 scheduler_disable()/scheduler_enable() 进行临界保护）
    void (*free)(void *);                // 执行释放内存（内部已通过 scheduler_disable()/scheduler_enable() 进行临界保护）
    unsigned (*get_sys_ticks)(void);     // 获取当前系统时间
    unsigned (*interrupt_save)(void);    // 禁止中断
    void (*interrupt_restore)(unsigned); // 恢复中断

    void (*scheduler_disable)(void);            // 禁止线程的调度（实时内核使用，非实时内核使用可设置为NULL）
    void (*scheduler_enable)(void);             // 恢复线程的调度（实时内核使用，非实时内核使用可设置为NULL）
    k_work_q_t *(*get_work_q_hdl)(void);        // 获取当前执行 k_work_q_handler() 的线程的 k_work_q_t 对象（实时内核使用，非实时内核使用可设置为NULL）
    void (*thread_sleep)(k_tick_t sleep_ticks); // 使线程休眠（实时内核使用，非实时内核使用可设置为NULL）
} k_init_t;

void k_entry(const k_init_t *init_struct); // 初始化并进入微内核模式

void k_init(const k_init_t *init_struct); // 初始化微内核
void k_deinit(void);                      // 取消初始化微内核

/** @defgroup work queue
 * @{
 */

typedef void (*k_work_q_fn)(void *arg);

extern k_work_q_t *default_work_q_hdl; // 预留的一个工默认的作队列对象内存，初始值为内部默认的对象

k_tick_t k_work_q_handler(k_work_q_t *work_q_handle); // 工作队列执行入口

k_err_t k_work_q_create(k_work_q_t *work_q_handle); // 创建一个工作队列的对象
void    k_work_q_delete(k_work_q_t *work_q_handle); // 删除一个工作队列的对象

void k_work_q_resume_regist(k_work_q_t *work_q_handle, k_work_q_fn work_q_resume, void *arg); // 注册一个函数：当有任务被提交时，此回调函数将被自动执行

bool k_work_q_is_valid(k_work_q_t *work_q_handle); // 获取工作队列是否有效

/**
 * @}
 */

/** @defgroup work
 * @{
 */

typedef struct
{
    k_hdl_t hdl;
} k_work_t;

typedef void (*k_work_fn)(void *arg);

void k_work_hook(k_work_q_t *work_q_handle, k_work_fn hook); // 设置切换任务时，进入和退出任务时回调函数

bool k_work_q_delayed_state(k_work_q_t *work_q_handle); // 获取是否有正在延时状态的任务
bool k_work_q_ready_state(k_work_q_t *work_q_handle);   // 获取工作队列中是否有就绪的任务

k_err_t k_work_create(k_work_t *work_handle,
                      const char *name,
                      k_work_fn work_route,
                      void *arg,
                      uint8_t priority);

void k_work_delete(k_work_t *work_handle); // 删除由工作队列管理的任务

bool     k_work_is_valid(k_work_t *work_handle);    // 获取任务对象是否有效
bool     k_work_is_pending(k_work_t *work_handle);  // 获取任务是否在就绪或延时的状态
k_tick_t k_work_time_remain(k_work_t *work_handle); // 获取任务距离下个执行的剩余时间

void k_work_submit(k_work_q_t *work_q_handle, k_work_t *work_handle, k_tick_t delay_ticks); // 使任务在指定工作队列中在指定时间后就绪
void k_work_resume(k_work_t *work_handle, k_tick_t delay_ticks);                            // 唤醒任务。注意需要先使用 k_work_submit 绑定一个 work_q_handle
void k_work_suspend(k_work_t *work_handle);                                                 // 挂起任务，任务不会被删除

void k_work_later(k_tick_t delay_ticks);       // 延时多少个系统节拍后再执行本任务
void k_work_later_until(k_tick_t delay_ticks); // 从最后一次唤醒的时间算起，延时多少个系统节拍后再执行本任务（固定周期的延时）
void k_work_yield(k_tick_t delay_ticks);       // 释放一次CPU的使用权，不调度低于当前任务优先级的任务
void k_work_sleep(k_tick_t delay_ticks);       // 释放一次CPU的使用权，可调度低于当前任务优先级的任务

k_work_t *k_get_curr_work_handle(k_work_q_t *work_q_handle); // 查询最近一次执行的任务

k_err_t k_work_mbox_create(k_work_t *work_handle); // 创建任务的邮箱
void    k_work_mbox_delete(k_work_t *work_handle); // 删除任务的邮箱

void   *k_work_mbox_alloc(k_work_t *work_handle, size_t size); // 申请一个邮件
k_err_t k_work_mbox_cancel(void *mbox);                        // 取消已申请的邮件
k_err_t k_work_mbox_submit(void *mbox);                        // 发送邮件
void   *k_work_mbox_take(void);                                // 提取邮件
void   *k_work_mbox_peek(void);                                // 查询邮件
void    k_work_mbox_clr(void);                                 // 清空任务的所有邮件

/**
 * @}
 */

/** @defgroup timer queue
 * @{
 */

typedef struct
{
    k_hdl_t hdl;
} k_timer_q_t;

typedef void (*k_timer_q_fn)(void *arg);

extern k_timer_q_t *default_timer_q_hdl; // 预留的一个工默认的软定时器队列对象内存，初始值为内部默认的对象

k_tick_t k_timer_q_handler(k_timer_q_t *timer_q_handle); // 软定时器队列执行入口

k_err_t k_timer_q_create(k_timer_q_t *timer_q_handle); // 创建一个软定时器队列的对象
void    k_timer_q_delete(k_timer_q_t *timer_q_handle); // 删除一个软定时器队列的对象

void k_timer_q_resume_regist(k_timer_q_t *timer_q_handle, k_timer_q_fn timer_q_resume, void *arg); // 注册一个函数：当有任务被提交时，此回调函数将被自动执行

bool k_timer_q_is_valid(k_timer_q_t *timer_q_handle); // 获取软定时器队列是否有效

/**
 * @}
 */

/** @defgroup timer
 * @{
 */

typedef struct
{
    k_hdl_t hdl;
} k_timer_t;

typedef void (*k_timer_fn)(void *arg);

bool k_timer_q_delayed_state(k_timer_q_t *timer_q_handle); // 获取是否有正在延时状态的定时器
bool k_timer_q_ready_state(k_timer_q_t *timer_q_handle);   // 获取软定时器队列中是否有就绪的定时器

k_err_t k_timer_create(k_timer_t *timer_handle,
                       k_timer_q_t *timer_q_handle,
                       k_timer_fn timer_route,
                       void *arg,
                       uint8_t priority);

void k_timer_delete(k_timer_t *timer_handle);

void k_timer_set_period(k_timer_t *timer_handle, bool periodic, k_tick_t period); // 设置定时器的自动重装值
void k_timer_start(k_timer_t *timer_handle, k_tick_t delay_ticks);                // 使定时器在指定软定时器队列中在指定时间后就绪
void k_timer_stop(k_timer_t *timer_handle);                                       // 挂起定时器，定时器不会被删除

k_err_t k_timer_newsubmit(k_timer_q_t *timer_q_handle, k_timer_fn timer_route, void *arg, k_tick_t delay_ticks); // 生成一个临时定时器，执行完自动删除
k_err_t k_timer_resubmit(k_timer_q_t *timer_q_handle, k_timer_fn timer_route, void *arg, k_tick_t delay_ticks);  // 删除并重新生成一个新的临时定时器，执行完自动删除
void    k_timer_cancel(k_timer_q_t *timer_q_handle, k_timer_fn timer_route);                                     // 立即删除临时定时器

bool     k_timer_is_valid(k_timer_t *timer_handle);    // 获取定时器对象是否有效
bool     k_timer_is_pending(k_timer_t *timer_handle);  // 获取定时器是否在就绪或延时的状态
bool     k_timer_is_periodic(k_timer_t *timer_handle); // 查询当前是否自动重装
k_tick_t k_timer_get_period(k_timer_t *timer_handle);  // 查询当前的自动重装值
k_tick_t k_timer_time_remain(k_timer_t *timer_handle); // 获取定时器距离下个执行的剩余时间

/**
 * @}
 */

/** @defgroup fifo
 * @{
 */

typedef struct
{
    k_hdl_t hdl;
} k_fifo_t;

k_err_t k_fifo_q_create(k_fifo_t *fifo_handle); // 创建一个 FIFO 对象
void    k_fifo_q_delete(k_fifo_t *fifo_handle); // 删除一个 FIFO 的对象
void    k_fifo_q_clr(k_fifo_t *fifo_handle);    // 清除 FIFO 内的所有数据

bool k_fifo_q_is_valid(k_fifo_t *fifo_handle); // 获取 FIFO 对象是否有效

void k_fifo_q_regist(k_fifo_t *fifo_handle, k_work_t *work_handle, k_tick_t delay_ticks); // 注册当队列非空时被唤醒的任务
void k_fifo_q_unregist(k_fifo_t *fifo_handle);                                            // 取消注册任务

void   *k_fifo_alloc(size_t size); // 申请可用于 FIFO 的数据结构
k_err_t k_fifo_free(void *data);   // 释放由 k_fifo_alloc() 申请的数据结构

k_err_t k_fifo_put(k_fifo_t *fifo_handle, void *data); // 把数据结构压入到 FIFO 中
void   *k_fifo_take(k_fifo_t *fifo_handle);            // 从 FIFO 中弹出最先压入的数据
void   *k_fifo_peek_head(k_fifo_t *fifo_handle);       // 查询 FIFO 中头部的数据地址
void   *k_fifo_peek_tail(k_fifo_t *fifo_handle);       // 查询 FIFO 中尾部的数据地址

/**
 * @}
 */

/** @defgroup queue
 * @{
 */

typedef struct
{
    k_hdl_t hdl;
} k_queue_t;

k_err_t k_queue_create(k_queue_t *queue_handle, size_t queue_length, size_t item_size); // 创建一个 QUEUE 对象
void    k_queue_delete(k_queue_t *queue_handle);                                        // 删除一个 QUEUE 的对象
void    k_queue_clr(k_queue_t *queue_handle);                                           // 清除 QUEUE 内的所有数据

bool k_queue_is_valid(k_queue_t *queue_handle); // 获取 QUEUE 对象是否有效

void k_queue_regist(k_queue_t *queue_handle, k_work_t *work_handle, k_tick_t delay_ticks); // 注册当队列非空时被唤醒的任务
void k_queue_unregist(k_queue_t *queue_handle);                                            // 取消注册任务

k_err_t k_queue_recv(k_queue_t *queue_handle, void *dst);       // 接收并复制数据
k_err_t k_queue_send(k_queue_t *queue_handle, const void *src); // 复制数据并发送

void   *k_queue_alloc(k_queue_t *queue_handle); // 申请可用于 QUEUE 的数据结构
k_err_t k_queue_free(void *data);               // 释放由 k_queue_alloc() 申请的数据结构
k_err_t k_queue_put(void *data);                // 把数据压入到 QUEUE 中
void   *k_queue_take(k_queue_t *queue_handle);  // 从 QUEUE 中弹出最先压入的数据

void *k_queue_peek_head(k_queue_t *queue_handle); // 查询 QUEUE 中头部的数据地址
void *k_queue_peek_tail(k_queue_t *queue_handle); // 查询 QUEUE 中尾部的数据地址

size_t k_queue_get_item_size(k_queue_t *queue_handle); // 读回 k_queue_create() 中设置的 item_size 的值

k_queue_t *k_queue_read_handle(void *data); // 查询已申请数据的所属队列句柄

/**
 * @}
 */

/** @defgroup pipe
 * @{
 */

typedef struct
{
    k_hdl_t hdl;
} k_pipe_t;

k_err_t k_pipe_create(k_pipe_t *pipe_handle, size_t pipe_size); // 创一个管道对象
void    k_pipe_delete(k_pipe_t *pipe_handle);                   // 删除一个管道对象
void    k_pipe_clr(k_pipe_t *pipe_handle);                      // 清空管道的数据

bool k_pipe_is_valid(k_pipe_t *pipe_handle); // 获取 PIPE 对象是否有效

void k_pipe_regist(k_pipe_t *pipe_handle, k_work_t *work_handle, k_tick_t delay_ticks); // 注册当队列非空时被唤醒的任务
void k_pipe_unregist(k_pipe_t *pipe_handle);                                            // 取消注册任务

size_t k_pipe_poll_write(k_pipe_t *pipe_handle, uint8_t data);                 // 写一个字节到缓存中（写入缓存），返回实际复制成功的字节数
size_t k_pipe_fifo_fill(k_pipe_t *pipe_handle, const void *data, size_t size); // 把内存数据复制到缓存中（写入缓存），返回实际复制成功的字节数
size_t k_pipe_poll_read(k_pipe_t *pipe_handle, uint8_t *data);                 // 从缓存复制一个字节到指定地址中，返回 0 表示缓存空
size_t k_pipe_fifo_read(k_pipe_t *pipe_handle, void *data, size_t size);       // 从管道中复制数据到内存（从缓存读取），返回实际复制成功的字节数。注：参数 data 值可以为 NULL，此时不复制数据，只释放相应的数据量

bool   k_pipe_is_ne(k_pipe_t *pipe_handle);          // 获取管道非空, true 有数据
size_t k_pipe_get_valid_size(k_pipe_t *pipe_handle); // 获取管道的数据大小（字节数）
size_t k_pipe_get_empty_size(k_pipe_t *pipe_handle); // 获取管道的剩余空间（字节数）

void k_pipe_peek_valid(k_pipe_t *pipe_handle, void **dst_base, size_t *dst_size); // 获取当前已写入的连续的内存信息
void k_pipe_peek_empty(k_pipe_t *pipe_handle, void **dst_base, size_t *dst_size); // 获取当前空闲的连续的内存信息

/**
 * @}
 */

/** @defgroup miscellaneous
 * @{
 */

k_tick_t k_get_sys_ticks(void);     // 获取当前系统时间
void     k_disable_interrupt(void); // 禁止中断（屏蔽中断并自动记录嵌套数）
void     k_enable_interrupt(void);  // 恢复中断（根据嵌套数自动恢复中断）

void k_log_sched(void); // 打印当前调度日志

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
