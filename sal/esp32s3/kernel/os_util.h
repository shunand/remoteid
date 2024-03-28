#ifndef __OS_UTIL_H__
#define __OS_UTIL_H__

#include "list/slist.h"
#include "os/os_common.h"
#include "os_debug.h"

#include "k_kit.h"

#include "FreeRTOS.h"
#include "task.h"

#define OS_TICK_RATE configTICK_RATE_HZ

typedef struct os_work_q_list
{
    k_work_q_t work_q_handle;
    os_sem_t sem_handle;
    os_thread_t thread;
} os_work_q_list_t;

struct os_thread_handle
{
    slist_node_t node;
    TaskHandle_t pxCreatedTask;
    os_thread_t *thread;
    os_work_q_list_t *work_q_list;
    os_thread_entry_t entry;
    void *arg;
    uint8_t flag_free;
};

#endif /* __OS_UTIL_H__ */
