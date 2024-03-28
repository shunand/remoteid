/**
 * @file kk.h
 * @author lokliang
 * @brief k_kit.h 衍生的易用接口，简化部分接口中的参数，去除不常用的接口
 * @version 1.0
 * @date 2022-12-09
 * @date 2021-01-13
 *
 * @copyright Copyright (c) 2021
 *
 */

#ifndef __KK_H__
#define __KK_H__

#include "k_kit.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef k_err_t    kk_err_t;
typedef k_tick_t   kk_time_t;
typedef k_work_q_t kk_work_q_t;
typedef k_work_t   kk_work_t;
typedef k_timer_t  kk_timer_t;
typedef k_fifo_t   kk_fifo_t;
typedef k_queue_t  kk_queue_t;
typedef k_pipe_t   kk_pipe_t;

#define WORK_Q_HDL default_work_q_hdl
#define TIMER_Q_HDL default_timer_q_hdl

#define kk_handler()       k_work_q_handler(WORK_Q_HDL)   // 工作队列执行入口
#define kk_timer_handler() k_timer_q_handler(TIMER_Q_HDL) // 软定时器队列执行入口

#define kk_work_q_delayed_state() k_work_q_delayed_state(WORK_Q_HDL) // 获取是否有正在延时状态的任务
#define kk_work_q_ready_state()   k_work_q_ready_state(WORK_Q_HDL)   // 获取工作队列中是否有就绪的任务
#define kk_work_q_delete()        k_work_q_delete(WORK_Q_HDL)        // 删除一个工作队列的对象
#define kk_work_q_is_valid()      k_work_q_is_valid(WORK_Q_HDL)      // 获取工作队列是否有效

#define kk_work_create(work_handle, work_route, arg, priority) k_work_create(work_handle, #work_route, work_route, arg, priority) // 创建由工作队列管理的任务
#define kk_work_delete(work_handle)                            k_work_delete(work_handle)                                         // 删除由工作队列管理的任务
#define kk_work_submit(work_q_handle, work_handle, delay)      k_work_submit(work_q_handle, work_handle, delay)                   // 多少时间后唤醒任务

#define kk_work_is_valid(work_handle)    k_work_is_valid(work_handle)    // 获取任务对象是否有效
#define kk_work_is_pending(work_handle)  k_work_is_pending(work_handle)  // 获取任务是否在就绪或延时的状态
#define kk_work_time_remain(work_handle) k_work_time_remain(work_handle) // 获取任务距离下个执行的剩余时间

#define kk_timer_create(timer_handle, timer_route, arg) k_timer_create(timer_handle, TIMER_Q_HDL, timer_route, arg, 0) // 创建由软定时器队列管理的定时器任务
#define kk_timer_delete(timer_handle)                   k_timer_delete(timer_handle)                                   // 删除由软定时器队列管理的定时器任务

#define kk_timer_start(timer_handle, periodic, period) do { k_timer_set_period(timer_handle, periodic, period); k_timer_start(timer_handle, period); } while (0) // 启动定时器
#define kk_timer_stop(timer_handle)                    k_timer_stop(timer_handle) // 挂起定时器，定时器不会被删除

#define kk_timer_is_valid(timer_handle)    k_timer_is_valid(timer_handle)    // 获取定时器对象是否有效
#define kk_timer_is_pending(timer_handle)  k_timer_is_pending(timer_handle)  // 获取定时器是否在就绪或延时的状态
#define kk_timer_time_remain(timer_handle) k_timer_time_remain(timer_handle) // 获取定时器距离下个执行的剩余时间

#define kk_work_mbox_create(work_handle) k_work_mbox_create(work_handle) // 创建任务的邮箱
#define kk_work_mbox_delete(work_handle) k_work_mbox_delete(work_handle) // 删除任务的邮箱

#define kk_fifo_q_create(fifo_handle)                     k_fifo_q_create(fifo_handle)                     // 创建一个FIFO对象
#define kk_fifo_q_delete(fifo_handle)                     k_fifo_q_delete(fifo_handle)                     // 删除一个FIFO的对象
#define kk_fifo_q_clr(fifo_handle)                        k_fifo_q_clr(fifo_handle)                        // 清除FIFO内的所有数据
#define kk_fifo_q_is_valid(fifo_handle)                   k_fifo_q_is_valid(fifo_handle)                   // 获取 FIFO 对象是否有效
#define kk_fifo_q_regist(fifo_handle, work_handle, ticks) k_fifo_q_regist(fifo_handle, work_handle, ticks) // 注册当队列非空时被唤醒的任务

#define kk_queue_create(queue_handle, queue_length, item_size) k_queue_create(queue_handle, queue_length, item_size) // 创建一个QUEUE对象
#define kk_queue_delete(queue_handle)                          k_queue_delete(queue_handle)                          // 删除一个QUEUE的对象
#define kk_queue_clr(queue_handle)                             k_queue_clr(queue_handle)                             // 清除QUEUE内的所有数据
#define kk_queue_is_valid(queue_handle)                        k_queue_is_valid(queue_handle)                        // 获取 QUEUE 对象是否有效
#define kk_queue_regist(queue_handle, work_handle, ticks)      k_queue_regist(queue_handle, work_handle, ticks)      // 注册当队列非空时被唤醒的任务

#define kk_pipe_create(pipe_handle, pipe_size)          k_pipe_create(pipe_handle, pipe_size)          // 创一个管道对象
#define kk_pipe_delete(pipe_handle)                     k_pipe_delete(pipe_handle)                     // 删除一个管道对象
#define kk_pipe_clr(pipe_handle)                        k_pipe_clr(pipe_handle)                        // 清空管道的数据
#define kk_pipe_is_valid(pipe_handle)                   k_pipe_is_valid(pipe_handle)                   // 获取 PIPE 对象是否有效
#define kk_pipe_regist(pipe_handle, work_handle, ticks) k_pipe_regist(pipe_handle, work_handle, ticks) // 注册当队列非空时被唤醒的任务

#define kk_work_resume(work_handle, delay) k_work_resume(work_handle, delay) // 唤醒任务。注意需要先使用 k_work_submit 绑定一个 work_q_handle
#define kk_work_suspend(work_handle)       k_work_suspend(work_handle)       // 挂起任务，任务不会被删除
#define kk_work_later(delay)               k_work_later(delay)               // 设置下个执行延时
#define kk_work_later_until(delay)         k_work_later_until(delay)         // 从最后一次唤醒的时间算起，延时多少个系统节拍后再执行本任务（固定周期的延时）
#define kk_work_yield(delay)               k_work_yield(delay)               // 释放一次CPU的使用权，不调度低于当前任务优先级的任务
#define kk_work_sleep(delay)               k_work_sleep(delay)               // 释放一次CPU的使用权，可调度低于当前任务优先级的任务

#define kk_work_mbox_alloc(work_handle, size) k_work_mbox_alloc(work_handle, size) // 申请一个邮件
#define kk_work_mbox_cancel(mbox)             k_work_mbox_cancel(mbox)             // 取消已申请的邮件
#define kk_work_mbox_submit(mbox)             k_work_mbox_submit(mbox)             // 发送邮件
#define kk_work_mbox_take()                   k_work_mbox_take()                   // 提取邮件
#define kk_work_mbox_peek()                   k_work_mbox_peek()                   // 查询邮件
#define kk_work_mbox_clr()                    k_work_mbox_clr()                    // 清空任务的所有邮件

#define kk_fifo_alloc(size)            k_fifo_alloc(size)            // 申请可用于FIFO的数据结构
#define kk_fifo_free(data)             k_fifo_free(data)             // 释放由 kk_fifo_alloc() 申请的数据结构
#define kk_fifo_put(fifo_handle, data) k_fifo_put(fifo_handle, data) // 把数据结构压入到FIFO中
#define kk_fifo_take(fifo_handle)      k_fifo_take(fifo_handle)      // 从FIFO中弹出最先压入的数据
#define kk_fifo_peek_head(fifo_handle) k_fifo_peek_head(fifo_handle) // 查询FIFO中头部的数据地址
#define kk_fifo_peek_tail(fifo_handle) k_fifo_peek_tail(fifo_handle) // 查询FIFO中尾部的数据地址

#define kk_queue_recv(queue_handle, dst)     k_queue_recv(queue_handle, dst)      // 接收并复制数据
#define kk_queue_send(queue_handle, src)     k_queue_send(queue_handle, src)      // 复制数据并发送
#define kk_queue_alloc(queue_handle)         k_queue_alloc(queue_handle)          // 申请可用于QUEUE的数据结构
#define kk_queue_free(data)                  k_queue_free(data)                   // 释放由 kk_queue_alloc() 申请的数据结构
#define kk_queue_put(data)                   k_queue_put(data)                    // 把数据压入到QUEUE中
#define kk_queue_take(queue_handle)          k_queue_take(queue_handle)           // 从QUEUE中弹出最先压入的数据
#define kk_queue_peek_head(queue_handle)     k_queue_peek_head(queue_handle)      // 查询QUEUE中头部的数据地址
#define kk_queue_peek_tail(queue_handle)     k_queue_peek_tail(queue_handle)      // 查询QUEUE中尾部的数据地址
#define kk_queue_get_item_size(queue_handle) k_queue_get_item_size(queue_handle); // 读回 k_queue_create() 中设置的 item_size 的值

#define kk_pipe_fifo_fill(pipe_handle, data, size)  k_pipe_fifo_fill(pipe_handle, data, size)  // 把内存数据复制到缓存中（写入缓存），返回实际复制成功的字节数
#define kk_pipe_poll_write(pipe_handle, data)       k_pipe_poll_write(pipe_handle, data)       // 写一个字节到缓存中（写入缓存），返回实际复制成功的字节数
#define kk_pipe_fifo_read(pipe_handle, data, size)  k_pipe_fifo_read(pipe_handle, data, size)  // 从管道中复制数据到内存（从缓存读取），返回实际复制成功的字节数。注：参数 data 值可以为 NULL，此时不复制数据，只释放相应的数据量
#define kk_pipe_poll_read(pipe_handle, data)        k_pipe_poll_read(pipe_handle, data)        // 从缓存复制一个字节到指定地址中，返回 0 表示缓存空

#define kk_pipe_is_ne(pipe_handle)          k_pipe_is_ne(pipe_handle)          // 获取管道非空, true 有数据
#define kk_pipe_get_valid_size(pipe_handle) k_pipe_get_valid_size(pipe_handle) // 获取管道的数据大小（字节数）
#define kk_pipe_get_empty_size(pipe_handle) k_pipe_get_empty_size(pipe_handle) // 获取管道的剩余空间（字节数）

#define kk_pipe_peek_valid(pipe_handle, dst_data, dst_size) k_pipe_peek_valid(pipe_handle, dst_data, dst_size) // 获取当前已写入的连续的内存信息
#define kk_pipe_peek_empty(pipe_handle, dst_data, dst_size) k_pipe_peek_empty(pipe_handle, dst_data, dst_size) // 获取当前空闲的连续的内存信息

#define kk_get_sys_ticks() k_get_sys_ticks() // 获取当前系统时间
#define kk_get_heap_mem()  k_get_heap_mem() // 获取初始化时指定的堆内存

#define kk_disable_interrupt() k_disable_interrupt() // 禁止中断
#define kk_enable_interrupt()  k_enable_interrupt()  // 恢复中断

#define kk_log_sched() k_log_sched() // 打印当前调度日志

#if defined(MIX_COMMON)
#include "heap.h"
#endif

#define kk_malloc(size)       heap_malloc(NULL, size)       // 申请内存
#define kk_calloc(size)       heap_calloc(NULL, size)       // 申请内存并置0
#define kk_realloc(ptr, size) heap_realloc(NULL, ptr, size) // 重定义已申请的内存大小
#define kk_free(ptr)          heap_free(NULL, ptr)          // 释放内存

#define kk_heap_is_valid(ptr)    heap_is_valid(NULL, ptr) // 获取内存指针是否有效
#define kk_heap_block_size(ptr)  heap_block_size(ptr)     // 已申请内存块的实际占用空间大小（不含所有控制信息）（字节）
#define kk_heap_space_size(ptr)  heap_space_size(ptr)     // 已申请内存块的实际占用空间大小（含所有控制信息）（字节）
#define kk_heap_used_size()      heap_used_size(NULL)     // 获取总已使用空间
#define kk_heap_free_size()      heap_free_size(NULL)     // 获取总空闲空间（包含所有碎片）
#define kk_heap_block_max()      heap_block_max(NULL)     // 获取当前最大的连续空间

#ifdef __cplusplus
}
#endif

#endif
