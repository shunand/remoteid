/**
 * @file k_kit.c
 * @author LokLiang (lokliang@163.com)
 * @brief 裸机版微内核
 * @version 1.1.0
 *
 * @date 2023-01-01 添加 k_timer
 * @date 2022-12-09
 * @date 2021-01-13
 *
 * @copyright Copyright (c) 2022
 *
 * 可被任意实时内核继承
 * 可管理任意数量的任务
 * 8个任务优先级
 * 允许任务 yield 和 sleep
 * 极少的移植操作
 * 所有接口访问线程安全
 *
 * 效率：在72MHz主频下
 * k_work_submit -- 3uS
 * work scheduler -- 4uS
 * k_fifo_put/k_fifo_take -- 2.5uS
 * k_queue_alloc/k_queue_put -- 2.5uS
 * k_pipe_poll_write/k_pipe_poll_read -- 1uS
 *
 * 任务和时间管理
 * -- 独立的任务队列
 * -- 所有任务对象可自由创建和删除
 * -- 拥有运行、就绪、延时、挂起四个状态
 * -- 栈来自所在的工作队列
 * -- 所有任务可挂到任意工作队列中
 * -- 可设置8档优先级（0..7）
 * -- 在运行过程中允许让出CPU使用权
 *
 * 软件定时器
 * -- 独立的任务队列
 * -- 与任务数据结构一致
 * -- 所有定时器对象可自由创建和删除
 * -- 拥有运行、就绪、延时、挂起四个状态
 * -- 栈来自所在的软定时器队列
 * -- 可设置8档优先级（0..7）
 *
 * 邮箱和邮件
 * -- 邮箱和邮件的数据空间由堆管理，邮件数量不限
 * -- 每个任务可创建和删除一个专属邮箱
 * -- 收到邮件的任务被置到就绪状态
 * -- 已提取的邮件在任务返回后被自动清理
 * -- 依赖临界操作和堆管理，效率较低
 *
 * FIFO
 * -- 队列和数据块的空间由堆管理，数量不限
 * -- 专用的数据块
 * -- 每个数据块长度不限
 * -- 依赖临界操作和堆管理，效率较低
 * -- 独立的对象，可脱离内核单独使用
 *
 * 队列
 * -- 队列空间由堆管理，数量不限
 * -- 数据块由队列管理，数据结构化，数量和长度在创建队列时指定，无内存碎片
 * -- 临界时间短，效率较高
 * -- 独立的对象，可脱离内核单独使用
 *
 * 管道
 * -- 管道空间由堆管理，数量不限
 * -- 允许两个线程同时读写数据流
 * -- 代码简短，适合在内存中执行
 * -- 不依赖临界操作和堆管理，通讯效率高
 * -- 独立的对象，可脱离内核单独使用
 *
 * 堆
 * -- 管理对象为任意的内存空间
 * -- 申请的内存块管理数据长度为 sizeof(size_t) * 4
 * -- 效率和速度较高
 * -- 独立的对象，可脱离内核单独使用
 */

#include "k_kit.h"

#include <string.h>
#include <stdio.h>

/**
 * @defgroup debug
 * @{
 */

#ifndef CONFIG_K_KIT_LOG_ON
#define CONFIG_K_KIT_LOG_ON 1 /* 0: k_log_sched() 无效; 1: k_log_sched() 生效 */
#endif

#ifndef CONFIG_K_KIT_DBG_ON
#define CONFIG_K_KIT_DBG_ON 1 /* 允许 k_kit 打印错误日志 */
#endif

#ifndef CONFIG_K_KIT_DBG_COLOR_ON
#define CONFIG_K_KIT_DBG_COLOR_ON 1 /* 在打印的日志中添加颜色控制 */
#endif

#ifndef CONFIG_K_KIT_PRINT
#define CONFIG_K_KIT_PRINT printf /* 设置打印函数 */
#endif

#define _CONS_PRINT(FMT, ARG...)            \
    do                                      \
    {                                       \
        if (CONFIG_K_KIT_DBG_ON)            \
        {                                   \
            CONFIG_K_KIT_PRINT(FMT, ##ARG); \
        }                                   \
    } while (0)

#if (CONFIG_K_KIT_DBG_COLOR_ON == 1)
#define _COLOR_R "\033[31m"     /* 红 RED    */
#define _COLOR_G "\033[32m"     /* 绿 GREEN  */
#define _COLOR_Y "\033[33m"     /* 黄 YELLOW */
#define _COLOR_B "\033[34m"     /* 蓝 BLUE   */
#define _COLOR_P "\033[35m"     /* 紫 PURPLE */
#define _COLOR_C "\033[36m"     /* 青 CYAN   */
#define _COLOR_RY "\033[41;33m" /* 红底黄字  */
#define _COLOR_END "\033[0m"    /* 结束      */
#else
#define _COLOR_R ""   /* 红 */
#define _COLOR_G ""   /* 绿 */
#define _COLOR_Y ""   /* 黄 */
#define _COLOR_B ""   /* 蓝 */
#define _COLOR_P ""   /* 紫 */
#define _COLOR_C ""   /* 青 */
#define _COLOR_RY ""  /* 红底黄字 */
#define _COLOR_END "" /* 结束 */
#endif

#define _DO_SYS_LOG(FLAG, FMT, ARG...) \
    do                                 \
    {                                  \
        if (FLAG)                      \
        {                              \
            _CONS_PRINT(FMT, ##ARG);   \
        }                              \
    } while (0)

#define _FILENAME(FILE) (strrchr(FILE, '/') ? (strrchr(FILE, '/') + 1) : (strrchr(FILE, '\\') ? (strrchr(FILE, '\\') + 1) : FILE))

#define _GEN_DOMAIN "[KIT] "

#define _SYS_LOG_COLOR(FLAG, INFO, COLOR, FMT, ...) _DO_SYS_LOG(FLAG, INFO "%s:%d \t%s -> " COLOR FMT _COLOR_END "\r\n", \
                                                                _FILENAME(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__)

#define _DBG_WRN(FMT, ...) _SYS_LOG_COLOR(CONFIG_K_KIT_DBG_ON, "[WRN] * " _GEN_DOMAIN, _COLOR_Y, FMT, ##__VA_ARGS__)

#define _ASSERT_FALSE(EXP, FMT, ...)                                                                    \
    do                                                                                                  \
    {                                                                                                   \
        if (EXP)                                                                                        \
        {                                                                                               \
            _CONS_PRINT("[ASS] # " _GEN_DOMAIN "%s:%d \t%s -> %s; ## " _COLOR_RY FMT _COLOR_END "\r\n", \
                        _FILENAME(__FILE__), __LINE__, __FUNCTION__, #EXP, ##__VA_ARGS__);              \
            do                                                                                          \
            {                                                                                           \
                volatile int FLAG = 0;                                                                  \
                do                                                                                      \
                {                                                                                       \
                } while (FLAG == 0);                                                                    \
            } while (0);                                                                                \
        }                                                                                               \
    } while (0)

/**
 * @} debug
 */

#define _SIGNED_MAX(N) ((int)((1u << (sizeof(N) * 8 - 1)) - 1))

#define _WORK ((k_work_handle_t *)work_handle->hdl)
#define _WORK_Q ((k_work_q_handle_t *)_WORK->work_q_handle->hdl)
#define _K_PORT (&s_kit_init_struct)
#define _K_CONTAINER_OF(PTR, TYPE, MEMBER) ((TYPE *)&((uint8_t *)PTR)[-(int)&((TYPE *)0)->MEMBER])

#define _TRUE true
#define _FALSE false

#define _K_DIS_INT() _k_interrupt_save()
#define _K_DIS_SCHED() _k_scheduler_disable()

#define _K_EN_INT() _k_interrupt_restore()
#define _K_EN_SCHED() _k_scheduler_enable()

#define _K_FIFO_FREE 0x83459321
#define _K_QUEUE_FREE 0x32478065

#define _SLIST_SET(Value1, Value2) (Value1 = (__list_node_t)Value2)
#define _SLIST_GET(Value) ((__slist_node_t *)(Value))

#define _DLIST_SET(Value1, Value2) (Value1 = (__list_node_t)Value2)
#define _DLIST_GET(Value) ((__dlist_node_t *)(Value))

typedef void *__list_node_t;

typedef struct
{
    __list_node_t next;
} __slist_node_t;
typedef struct
{
    __list_node_t head;
    __list_node_t tail;
} __slist_t;

typedef struct
{
    __list_node_t next;
    __list_node_t prev;
} __dlist_node_t;
typedef struct
{
    __list_node_t head;
} __dlist_t;

typedef struct
{
    __slist_t mbox_valid;
    __slist_t mbox_idle;
} k_work_mb_list_t;

typedef struct
{
    __slist_node_t state_node;
    __slist_t *state_list;
    __dlist_node_t event_node;
    k_tick_t timeout;
    k_work_fn work_route;
    void *arg;
    k_work_q_t *work_q_handle;
    k_work_t *work_handle;
    uint8_t priority;
    uint8_t exec_flag;
    uint8_t exec_repeat;
    uint8_t delete_flag : 1;       /* 表示任务正在运行时被关闭，当运行完毕后资源被回收 */
    uint8_t obj_flag_type : 1;     /* 0: 表示任务类型为 work; 1: 表示任务类型为 timer */
    uint8_t obj_flag_periodic : 1; /* 任务类型为 timer 时有效，表示使用自动重装 */
    uint8_t obj_flag_auto_del : 1; /* 任务类型为 timer 时有效，表示执行完成后自动删除对象 */
    union
    {
        k_work_mb_list_t *mbox_list;
        k_tick_t period;
    } obj_data;

#if (CONFIG_K_KIT_LOG_ON)
    const char *name;
#endif
} k_work_handle_t;

typedef struct
{
    __dlist_t event_list;
    __slist_t ready_list[8];
    __slist_t delayed_list;
    unsigned priority_tab;
    k_work_t *curr_work;
    k_work_handle_t *yield_work;
    k_work_q_fn work_q_resume;
    void *work_q_resume_arg;
    k_work_fn hook_entry;
    k_tick_t timeout_early;
    volatile unsigned resume_flag;
} k_work_q_handle_t;

typedef struct
{
    union
    {
        __slist_node_t state_node;
        k_work_t *work_handle;
    } ctrl;
} k_work_mb_handle_t;

typedef struct
{
    __slist_t list;
    k_work_t *resume_work;
    k_tick_t resume_delay;
} k_fifo_q_handle_t;

typedef struct
{
    union
    {
        __slist_node_t state_node;
        unsigned free_flag;
    } ctrl;
} k_fifo_handle_t;

typedef struct
{
    __slist_t list_valid;
    __slist_t list_idle;
    size_t item_size;
    k_work_t *resume_work;
    k_tick_t resume_delay;
} k_queue_handle_t;

typedef struct
{
    union
    {
        __slist_node_t state_node;
        unsigned free_flag;
    } ctrl;
    k_queue_t *queue_handle;
} k_queue_data_t;

typedef volatile size_t pipe_id_t;

typedef struct
{
    size_t size;   // 缓存总大小
    pipe_id_t wid; // 已写进的下标
    pipe_id_t rid; // 已读出的下标
    k_work_t *resume_work;
    k_tick_t resume_delay;
} k_pipe_handle_t;

static struct
{
    unsigned interrupt_call;
    unsigned interrupt_nest;

    k_work_t *curr_hook;
    k_work_t *curr_log;

#if (CONFIG_K_KIT_LOG_ON)
    k_work_t *work_log[8];
    uint8_t log_index;
    uint8_t log_max;
#endif
} s_cm_kit;

static k_work_q_t work_q_hdl_main;
static k_timer_q_t timer_q_hdl_main;
k_work_q_t *default_work_q_hdl = &work_q_hdl_main;
k_timer_q_t *default_timer_q_hdl = &timer_q_hdl_main;

static k_init_t s_kit_init_struct;

static const uint8_t _tab_clz_8[] = {
    7, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4, // 0x00
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, // 0x10
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // 0x20
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // 0x30
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0x40
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0x50
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0x60
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0x70
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0x80
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0x90
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0xA0
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0xB0
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0xC0
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0xD0
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0xE0
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0xF0
};

static void _slist_init_list(__slist_t *list);
static void _slist_init_node(__slist_node_t *node);
static int _slist_insert_font(__slist_t *list, __slist_node_t *node);
static int _slist_insert_tail(__slist_t *list, __slist_node_t *node);
static int _slist_remove(__slist_t *list, __slist_node_t *node);
static int _slist_remove_next(__slist_t *list, __slist_node_t *prev_node, __slist_node_t *remove_node);
static int _slist_insert_next(__slist_t *list, __slist_node_t *tar_node, __slist_node_t *new_node);
static __slist_node_t *_slist_take_head(__slist_t *list);
static __slist_node_t *_slist_peek_head(__slist_t *list);
static __slist_node_t *_slist_peek_tail(__slist_t *list);
static __slist_node_t *_slist_peek_next(__slist_node_t *node);
static __slist_node_t *_slist_peek_prev(__slist_t *list, __slist_node_t *node);
static int _slist_is_pending(__slist_node_t *node);

static void _dlist_init_list(__dlist_t *list);
static void _dlist_init_node(__dlist_node_t *node);
static int _dlist_insert_tail(__dlist_t *list, __dlist_node_t *node);
static int _dlist_remove(__dlist_t *list, __dlist_node_t *node);
static __dlist_node_t *_dlist_take_head(__dlist_t *list);
static __dlist_node_t *_dlist_peek_head(__dlist_t *list);
static __dlist_node_t *_dlist_peek_tail(__dlist_t *list);
static __dlist_node_t *_dlist_peek_next(__dlist_t *list, __dlist_node_t *node);
static int _dlist_is_pending(__dlist_node_t *node);

static void _slist_init_list(__slist_t *list)
{
    _SLIST_SET(list->head, 0);
    _SLIST_SET(list->tail, 0);
}

static void _slist_init_node(__slist_node_t *node)
{
    _SLIST_SET(node->next, 0);
}

static int _slist_insert_font(__slist_t *list, __slist_node_t *node)
{
    __slist_node_t *head_node;

    if (_SLIST_GET(node->next) != NULL)
    {
        return -1;
    }

    head_node = _SLIST_GET(list->head);
    if (head_node)
    {
        _SLIST_SET(node->next, head_node);
    }
    else
    {
        _SLIST_SET(node->next, node);
        _SLIST_SET(list->tail, node);
    }
    _SLIST_SET(list->head, node);

    return 0;
}

static int _slist_insert_tail(__slist_t *list, __slist_node_t *node)
{
    if (_SLIST_GET(node->next) != NULL)
    {
        return -1;
    }

    _SLIST_SET(node->next, node);
    if (_SLIST_GET(list->head) == NULL)
    {
        _SLIST_SET(list->head, node);
    }
    else
    {
        __slist_node_t *tail_node = (__slist_node_t *)_SLIST_GET(list->tail);

        _SLIST_SET(tail_node->next, node);
    }
    _SLIST_SET(list->tail, node);

    return 0;
}

static int _slist_remove(__slist_t *list, __slist_node_t *node)
{
    __slist_node_t *list_head;
    __slist_node_t *node_next;

    if (node == NULL || list == NULL)
    {
        return -1;
    }

    list_head = _SLIST_GET(list->head);
    node_next = _SLIST_GET(node->next);
    if (node_next == NULL || list_head == NULL)
    {
        return -1;
    }

    if (node == list_head) // node 是首个节点
    {
        if (node == _SLIST_GET(list->tail)) // node 是最后一个节点
        {
            _SLIST_SET(list->head, 0);
            _SLIST_SET(list->tail, 0);
        }
        else
        {
            _SLIST_SET(list->head, node_next);
        }
    }
    else // node 不是首个节点
    {
        __slist_node_t *prev_node = _slist_peek_prev(list, node);
        if (prev_node == NULL)
        {
            return -1;
        }

        if (node == _SLIST_GET(list->tail)) // node 是最后一个节点
        {
            _SLIST_SET(list->tail, prev_node);
            _SLIST_SET(prev_node->next, prev_node);
        }
        else
        {
            _SLIST_SET(prev_node->next, node_next);
        }
    }

    _SLIST_SET(node->next, 0);

    return 0;
}

static int _slist_remove_next(__slist_t *list, __slist_node_t *prev_node, __slist_node_t *remove_node)
{
    __slist_node_t *list_head;
    __slist_node_t *node_next;

    if (remove_node == NULL || list == NULL)
    {
        return -1;
    }

    list_head = _SLIST_GET(list->head);
    node_next = _SLIST_GET(remove_node->next);
    if (node_next == NULL || list_head == NULL)
    {
        return -1;
    }

    if (remove_node == list_head) // remove_node 是首个节点
    {
        if (remove_node == _SLIST_GET(list->tail)) // remove_node 是最后一个节点
        {
            _SLIST_SET(list->head, 0);
            _SLIST_SET(list->tail, 0);
        }
        else
        {
            _SLIST_SET(list->head, node_next);
        }
    }
    else // remove_node 不是首个节点
    {
        if (prev_node == NULL)
        {
            prev_node = _slist_peek_prev(list, remove_node);
            if (prev_node == NULL)
            {
                return -1;
            }
        }

        if (remove_node == _SLIST_GET(list->tail)) // remove_node 是最后一个节点
        {
            _SLIST_SET(list->tail, prev_node);
            _SLIST_SET(prev_node->next, prev_node);
        }
        else
        {
            _SLIST_SET(prev_node->next, node_next);
        }
    }

    _SLIST_SET(remove_node->next, 0);

    return 0;
}

static int _slist_insert_next(__slist_t *list, __slist_node_t *tar_node, __slist_node_t *new_node)
{
    __slist_node_t *tar_node_next = _SLIST_GET(tar_node->next);

    if (_SLIST_GET(list->head) == NULL || tar_node_next == NULL || _SLIST_GET(new_node->next) != NULL)
    {
        return -1;
    }

    if (tar_node == _SLIST_GET(list->tail)) // tar_node 是最后一个节点
    {
        _SLIST_SET(new_node->next, new_node);
        _SLIST_SET(tar_node->next, new_node);
        _SLIST_SET(list->tail, new_node);
    }
    else
    {
        _SLIST_SET(new_node->next, tar_node_next);
        _SLIST_SET(tar_node->next, new_node);
    }

    return 0;
}

static __slist_node_t *_slist_take_head(__slist_t *list)
{
    __slist_node_t *ret = _SLIST_GET(list->head);
    _slist_remove(list, ret);
    return ret;
}

static __slist_node_t *_slist_peek_head(__slist_t *list)
{
    return _SLIST_GET(list->head);
}

static __slist_node_t *_slist_peek_tail(__slist_t *list)
{
    return _SLIST_GET(list->tail);
}

static __slist_node_t *_slist_peek_next(__slist_node_t *node)
{
    __slist_node_t *next_node = _SLIST_GET(node->next);
    if (next_node == node)
    {
        return NULL;
    }
    else
    {
        return next_node;
    }
}

static __slist_node_t *_slist_peek_prev(__slist_t *list, __slist_node_t *node)
{
    __slist_node_t *prev_node = NULL;
    __slist_node_t *test_node = _SLIST_GET(list->head);
    while (test_node && test_node != node && prev_node != test_node)
    {
        prev_node = test_node;
        test_node = _SLIST_GET(test_node->next);
    }
    if (test_node == node)
    {
        return prev_node;
    }

    return NULL;
}

static int _slist_is_pending(__slist_node_t *node)
{
    return (_SLIST_GET(node->next) != NULL);
}

static void _dlist_init_list(__dlist_t *list)
{
    _DLIST_SET(list->head, 0);
}

static void _dlist_init_node(__dlist_node_t *node)
{
    _DLIST_SET(node->next, 0);
    _DLIST_SET(node->prev, 0);
}

static int _dlist_insert_tail(__dlist_t *list, __dlist_node_t *node)
{
    __dlist_node_t *first_node;

    if (_DLIST_GET(node->next) != NULL || _DLIST_GET(node->prev) != NULL)
    {
        return -1;
    }

    first_node = _DLIST_GET(list->head);
    if (first_node == NULL)
    {
        /* 直接设置链头 */
        _DLIST_SET(node->next, node);
        _DLIST_SET(node->prev, node);
        _DLIST_SET(list->head, node);
    }
    else
    {
        __dlist_node_t *first_node_prev = _DLIST_GET(first_node->prev);

        _DLIST_SET(node->next, first_node);
        _DLIST_SET(node->prev, first_node_prev);
        _DLIST_SET(first_node_prev->next, node);
        _DLIST_SET(first_node->prev, node);
    }

    return 0;
}

static int _dlist_remove(__dlist_t *list, __dlist_node_t *node)
{
    __dlist_node_t *first_node;
    __dlist_node_t *node_next;

    if (node == NULL || list == NULL)
    {
        return -1;
    }
    first_node = _DLIST_GET(list->head);
    node_next = _DLIST_GET(node->next);
    if (node_next == NULL || first_node == NULL)
    {
        return -1;
    }

    if (node_next == node) // 是最后一节点
    {
        _DLIST_SET(list->head, 0);
    }
    else // 不是最后一节点
    {
        __dlist_node_t *node_prev = _DLIST_GET(node->prev);
        if (first_node == node)
        {
            _DLIST_SET(list->head, node_next); // 链头指向下一节点
        }
        _DLIST_SET(node_prev->next, node_next);
        _DLIST_SET(node_next->prev, node_prev);
    }

    _DLIST_SET(node->next, 0);
    _DLIST_SET(node->prev, 0);

    return 0;
}

static __dlist_node_t *_dlist_take_head(__dlist_t *list)
{
    __dlist_node_t *ret = _DLIST_GET(list->head);
    _dlist_remove(list, ret);
    return ret;
}

static __dlist_node_t *_dlist_peek_head(__dlist_t *list)
{
    return _DLIST_GET(list->head);
}

static __dlist_node_t *_dlist_peek_tail(__dlist_t *list)
{
    __dlist_node_t *list_head = _DLIST_GET(list->head);
    if (list_head == NULL)
    {
        return NULL;
    }
    else
    {
        return _DLIST_GET(list_head->prev);
    }
}

static __dlist_node_t *_dlist_peek_next(__dlist_t *list, __dlist_node_t *node)
{
    __dlist_node_t *ret = _DLIST_GET(node->next);
    if (_dlist_peek_head(list) == ret)
    {
        ret = NULL;
    }
    return ret;
}

static int _dlist_is_pending(__dlist_node_t *node)
{
    return (_DLIST_GET(node->next) != NULL);
}

static void _k_interrupt_save(void)
{
    if (s_cm_kit.interrupt_call == 0)
    {
        s_cm_kit.interrupt_nest = _K_PORT->interrupt_save();
    }
    s_cm_kit.interrupt_call++;
}

static void _k_interrupt_restore(void)
{
    s_cm_kit.interrupt_call--;
    _ASSERT_FALSE((int)s_cm_kit.interrupt_call < 0, "error");
    if (s_cm_kit.interrupt_call == 0)
    {
        _K_PORT->interrupt_restore(s_cm_kit.interrupt_nest);
    }
}

static void _k_scheduler_disable(void)
{
    _K_PORT->scheduler_disable();
}

static void _k_scheduler_enable(void)
{
    _K_PORT->scheduler_enable();
}

static void _k_nop(void)
{
}

static k_work_q_t *_k_nop_get_work_q_hdl(void)
{
    return default_work_q_hdl;
}

static void _k_read_curr_handle(k_work_q_t **work_q_handle, k_work_t **work_handle)
{
    *work_q_handle = _K_PORT->get_work_q_hdl();
    *work_handle = ((k_work_q_handle_t *)(*work_q_handle)->hdl)->curr_work;
}

/**
 * @brief 删除指定邮箱队列中的邮件
 *
 * @param list 邮箱队列，mbox_idle 或 mbox_valid
 */
static void _k_work_mbox_clr_list(__slist_t *list)
{
    for (;;)
    {
        __slist_node_t *node = _slist_take_head(list);
        if (node == NULL)
        {
            break;
        }
        else
        {
            k_work_mb_handle_t *mbox_data = _K_CONTAINER_OF(node, k_work_mb_handle_t, ctrl.state_node);
            _K_PORT->free(mbox_data);
        }
    }
}

/**
 * @brief 执行删除任务
 *
 * @param work_handle 由 k_work_create() 初始化
 */
static void _k_work_delete(k_work_t *work_handle)
{
    if (work_handle == NULL)
    {
        k_work_q_t *work_q_handle;
        _k_read_curr_handle(&work_q_handle, &work_handle);
    }

    if (work_handle->hdl != NULL)
    {
        k_work_handle_t *work = work_handle->hdl;

        if (work->obj_flag_type == 0 && work->obj_data.mbox_list != NULL)
        {
            k_work_mb_list_t *mbox_list = work->obj_data.mbox_list;
            if (mbox_list)
            {
                _k_work_mbox_clr_list(&mbox_list->mbox_idle);
                _k_work_mbox_clr_list(&mbox_list->mbox_valid);
                _K_PORT->free(mbox_list);
                work->obj_data.mbox_list = NULL;
            }
        }
        _slist_remove(work->state_list, &work->state_node);
        if (_dlist_is_pending(&work->event_node))
        {
            k_work_q_handle_t *work_q = work->work_q_handle->hdl;
            _dlist_remove(&work_q->event_list, &work->event_node);
        }
        _K_PORT->free(work_handle->hdl);

#if (CONFIG_K_KIT_LOG_ON)
        {
            unsigned max_index = sizeof(s_cm_kit.work_log) / sizeof(s_cm_kit.work_log[0]);
            unsigned index;

            if (max_index > s_cm_kit.log_index)
            {
                max_index = s_cm_kit.log_index;
            }

            for (index = 0; index < max_index; index++)
            {
                if (s_cm_kit.work_log[index] == work_handle)
                {
                    unsigned i;
                    for (i = index; i + 1 < max_index; i++)
                    {
                        s_cm_kit.work_log[i] = s_cm_kit.work_log[i + 1];
                    }
                    s_cm_kit.work_log[i] = NULL;
                    s_cm_kit.log_index--;
                    s_cm_kit.log_max--;
                    break;
                }
            }
        }
#endif

        work_handle->hdl = NULL;
    }
}

/**
 * @brief 从信号链 (work_q->event_list) 提取任务，并根据时间设置插入到延时链或直接插入到就绪链或中
 *
 * @param event_list 来源链
 * @param sys_time 当前系统时间
 */
static void _k_work_q_handler_take_event_list(__dlist_t *event_list, k_tick_t sys_time)
{
    if (!_dlist_peek_head(event_list))
    {
        return;
    }

    for (;;)
    {
        __dlist_node_t *event_node;

        _K_DIS_INT();
        event_node = _dlist_take_head(event_list);
        if (event_node == NULL)
        {
            _K_EN_INT();
            break;
        }
        else
        {
            k_work_handle_t *opt_work = _K_CONTAINER_OF(event_node, k_work_handle_t, event_node);
            _K_EN_INT();

            /* 根据时间设置，把任务插入到延时链或直接插入到就绪链或中 */
            _K_DIS_SCHED();
            if (opt_work->timeout - sys_time > 0)
            {
                /* 按时间排序，插入到 work_q->delayed_list */
                k_work_q_handle_t *work_q = opt_work->work_q_handle->hdl;
                __slist_t *new_list = &work_q->delayed_list;
                __slist_node_t *work_node;
                __slist_node_t *test_node;

                _slist_remove(opt_work->state_list, &opt_work->state_node);
                opt_work->state_list = new_list;
                work_node = &opt_work->state_node;
                test_node = _slist_peek_head(new_list);

                if (test_node == NULL)
                {
                    work_q->timeout_early = opt_work->timeout;
                    _slist_insert_font(new_list, work_node);
                }
                else
                {
                    __slist_node_t *end_node = _slist_peek_tail(new_list);
                    __slist_node_t *prev_node = NULL;
                    k_work_handle_t *test_work = _K_CONTAINER_OF(test_node, k_work_handle_t, state_node);

                    work_q->timeout_early = test_work->timeout;

                    do
                    {
                        if ((int)(opt_work->timeout - test_work->timeout) < 0)
                        {
                            break;
                        }

                        prev_node = test_node;
                        test_node = _slist_peek_next(test_node);

                        test_work = _K_CONTAINER_OF(test_node, k_work_handle_t, state_node);

                    } while (prev_node != end_node);

                    if (prev_node == NULL)
                    {
                        work_q->timeout_early = opt_work->timeout;
                        _slist_insert_font(new_list, work_node);
                    }
                    else
                    {
                        _slist_insert_next(new_list, prev_node, work_node);
                    }
                }
            }
            else
            {
                /* 直接插入到 work_q->ready_list[] */
                k_work_q_handle_t *work_q = opt_work->work_q_handle->hdl;
                __slist_t *new_list = &work_q->ready_list[opt_work->priority];
                if (opt_work->state_list != new_list)
                {
                    _slist_remove(opt_work->state_list, &opt_work->state_node);
                    _slist_insert_tail(new_list, &opt_work->state_node);
                    opt_work->state_list = new_list;
                    work_q->priority_tab |= 1 << opt_work->priority;
                }
            }
            _K_EN_SCHED();
        }
    }
}

/**
 * @brief 从延时链 (work_q->delayed_list) 中把到所有到达时间的任务插入到就绪链中
 *
 * @param src_list 来源链
 * @return k_tick_t 距离下个延时任务的时间
 */
static k_tick_t _k_work_q_handler_take_delay_list(k_work_q_handle_t *work_q, k_tick_t sys_time)
{
    int ret;

    _k_work_q_handler_take_event_list(&work_q->event_list, sys_time);

    if ((int)(sys_time - *(volatile int *)&work_q->timeout_early) >= 0)
    {
        for (;;)
        {
            __slist_node_t *delayed_node;

            _K_DIS_SCHED();
            delayed_node = _slist_peek_head(&work_q->delayed_list);

            if (delayed_node != NULL)
            {
                k_work_handle_t *opt_work = _K_CONTAINER_OF(delayed_node, k_work_handle_t, state_node);

                if (opt_work->work_q_handle->hdl == NULL)
                {
                    _k_work_delete(opt_work->work_handle);
                }
                else
                {
                    if ((int)(sys_time - opt_work->timeout) >= 0)
                    {
                        _slist_remove(opt_work->state_list, &opt_work->state_node);
                        opt_work->state_list = &work_q->ready_list[7 - _tab_clz_8[1 << opt_work->priority]];
                        _slist_insert_tail(opt_work->state_list, &opt_work->state_node);
                        work_q->priority_tab |= 1 << opt_work->priority;
                    }
                    else
                    {
                        work_q->timeout_early = opt_work->timeout;
                        _K_EN_SCHED();
                        break;
                    }
                }
            }
            else
            {
                work_q->timeout_early = sys_time + _SIGNED_MAX(sys_time);
                _K_EN_SCHED();
                break;
            }
            _K_EN_SCHED();
        }
    }

    ret = work_q->timeout_early - sys_time;
    if (ret < 0)
    {
        ret = 0;
    }
    return ret;
}

/**
 * @brief 获取下个可执行的任务对象
 *
 * @param work_q_handle 由 k_work_q_create() 初始化
 * @return k_work_handle_t* 下个可执行的任务对象
 */
static k_work_handle_t *_k_work_q_handler_take_ready_work(k_work_q_t *work_q_handle)
{
    for (;;)
    {
        k_work_q_handle_t *work_q;

        _K_DIS_SCHED();
        work_q = work_q_handle->hdl;
        if (work_q == NULL || work_q->priority_tab == 0)
        {
            _K_EN_SCHED();
            return NULL;
        }
        else
        {
            int prio;
            __slist_t *state_list;
            __slist_node_t *state_node;
            k_work_handle_t *exec_work;

            prio = 7 - _tab_clz_8[work_q->priority_tab];
            if (work_q->yield_work != NULL && prio < work_q->yield_work->priority)
            {
                _K_EN_SCHED();
                return NULL;
            }
            state_list = &work_q->ready_list[prio];
            state_node = _slist_take_head(state_list);
            exec_work = NULL;
            if (state_node == NULL)
            {
                work_q->priority_tab &= ~(1 << prio);
            }
            else
            {
                exec_work = _K_CONTAINER_OF(state_node, k_work_handle_t, state_node);
                if (_slist_peek_head(state_list) == NULL)
                {
                    work_q->priority_tab &= ~(1 << prio);
                }
                if (exec_work->exec_flag == 0)
                {
                    work_q->curr_work = exec_work->work_handle;
                }
                exec_work->state_list = NULL;
            }
            _K_EN_SCHED();

            if (exec_work != NULL)
            {
                if (exec_work->exec_flag == 0)
                {
                    return exec_work;
                }
                else
                {
                    exec_work->exec_repeat = 1;
                }
            }
        }
    }
}

/**
 * @brief 用于记录和打印调度日志
 *
 * @param work_handle 由 k_work_create() 初始化
 */
static void _k_work_log(void *arg)
{
#if (CONFIG_K_KIT_LOG_ON)

    k_work_q_t *work_q_handle;
    k_work_t *work_handle;
    unsigned max_index;

    _K_DIS_SCHED();

    _k_read_curr_handle(&work_q_handle, &work_handle);
    max_index = sizeof(s_cm_kit.work_log) / sizeof(s_cm_kit.work_log[0]);
    if (max_index > s_cm_kit.log_index)
    {
        max_index = s_cm_kit.log_index;
    }
    while (max_index)
    {
        if (s_cm_kit.work_log[--max_index] == work_handle)
        {
            s_cm_kit.log_index = max_index + 1;
            break;
        }
    }

    if (s_cm_kit.work_log[max_index] != work_handle)
    {
        if (s_cm_kit.log_index < sizeof(s_cm_kit.work_log) / sizeof(s_cm_kit.work_log[0]))
        {
            s_cm_kit.work_log[s_cm_kit.log_index] = work_handle;
        }

        if (s_cm_kit.log_index <= sizeof(s_cm_kit.work_log) / sizeof(s_cm_kit.work_log[0]))
        {
            s_cm_kit.log_index++;
            s_cm_kit.log_max = s_cm_kit.log_index;
        }
    }

    _K_EN_SCHED();

#endif
}

/**
 * @brief 把任务插入到 work_q->event_list 中
 *
 * @param work_q_handle 由 k_work_q_create() 初始化
 * @param work_handle 由 k_work_create() 初始化
 * @param timeout 指定的唤醒时间点
 * @param wait_forever 是否永远等待
 */
static void _k_work_do_submit(k_work_q_t *work_q_handle, k_work_t *work_handle, k_tick_t timeout, bool wait_forever)
{
    k_work_handle_t *work = work_handle->hdl;
    k_work_q_handle_t *work_q = work_q_handle->hdl;
    int resume_flag = 1;

    if (work_handle == NULL || work == NULL)
    {
        _DBG_WRN("work_handle invalid");
        return;
    }
    if (work_q_handle == NULL || work_q == NULL)
    {
        _DBG_WRN("work_q_handle invalid");
        return;
    }

    if (wait_forever)
    {
        if (work->work_q_handle != work_q_handle ||
            _dlist_is_pending(&work->event_node))
        {
            _K_DIS_INT();

            if (work_handle->hdl != NULL)
            {
                if (_dlist_is_pending(&work->event_node))
                {
                    k_work_q_handle_t *work_q_old = work->work_q_handle->hdl;
                    _dlist_remove(&work_q_old->event_list, &work->event_node);
                }
                work->work_q_handle = work_q_handle;
                work->work_handle = work_handle;
            }

            _K_EN_INT();
        }
    }
    else
    {
        if (work->timeout != timeout ||
            work->work_q_handle != work_q_handle ||
            !_dlist_is_pending(&work->event_node))
        {
            _K_DIS_INT();

            if (work_handle->hdl != NULL)
            {
                work->timeout = timeout;
                if (work->work_q_handle != work_q_handle)
                {
                    if (_dlist_is_pending(&work->event_node))
                    {
                        k_work_q_handle_t *work_q_old = work->work_q_handle->hdl;
                        _dlist_remove(&work_q_old->event_list, &work->event_node);
                    }
                }
                work->work_q_handle = work_q_handle;
                if (!_dlist_is_pending(&work->event_node))
                {
                    _dlist_insert_tail(&work_q->event_list, &work->event_node);
                    work->work_handle = work_handle;
                    resume_flag = work_q->resume_flag;
                }
            }

            _K_EN_INT();
        }

        if (resume_flag == 0)
        {
            work_q->resume_flag = 1;
            if (work_q->work_q_resume != NULL)
            {
                work_q->work_q_resume(work_q->work_q_resume_arg);
            }
        }
    }
}

/**
 * @brief 由内部通讯模块所用，执行唤醒已注册的任务。
 *
 * @param resume_work 已注册的任务
 * @param resume_delay 唤醒延时
 * @param resume_mode 预留的。 resume_delay 为非 0 时有效：
 * 0 - 计时从首次收到信号后开始；
 * 1 - 计时从最后一次收到信号后开始；
 */
static void _k_ipc_resume(k_work_t *resume_work, k_tick_t resume_delay, int resume_mode)
{
    if (resume_work != NULL)
    {
        k_work_handle_t *work = resume_work->hdl;
        if (work != NULL && work->work_q_handle != NULL)
        {
            if (resume_delay == 0)
            {
                if (!k_work_is_pending(resume_work))
                {
                    _k_work_do_submit(work->work_q_handle, resume_work, _K_PORT->get_sys_ticks(), _FALSE);
                }
            }
            else
            {
                if (resume_mode != 0 || !k_work_is_pending(resume_work))
                {
                    k_work_submit(work->work_q_handle, resume_work, resume_delay);
                }
            }
        }
    }
}

/**
 * @brief 当被执行完的任务为软件定时器时，按时间重新提交一次到队列中
 *
 * @param work_q_handle 由 k_work_q_create() 初始化
 * @param exec_work 被执行的任务对象
 */
static void _k_timer_end(k_work_q_t *work_q_handle, k_work_handle_t *exec_work)
{
    if (exec_work->obj_flag_auto_del)
    {
        k_work_t work_handle;

        work_handle.hdl = exec_work;
        exec_work->work_handle = &work_handle;
        k_work_delete(&work_handle);
    }
    else if (exec_work->obj_flag_periodic)
    {
        k_tick_t timeout = exec_work->timeout +
                           exec_work->obj_data.period +
                           (_K_PORT->get_sys_ticks() - exec_work->timeout) / exec_work->obj_data.period * exec_work->obj_data.period;
        _k_work_do_submit(work_q_handle, exec_work->work_handle, timeout, _FALSE);
    }
}

/**
 * @brief 初始化微内核
 * 建议初始化后再配置系统的定时器，每1毫秒产生一次中断
 *
 * @param init_struct 初始化参数
 */
void k_init(const k_init_t *init_struct)
{
    if (s_kit_init_struct.malloc != NULL)
    {
        _DBG_WRN("Duplicate initialization");
    }
    _ASSERT_FALSE(init_struct->malloc == NULL ||
                      init_struct->free == 0 ||
                      init_struct->interrupt_save == NULL ||
                      init_struct->interrupt_restore == NULL,
                  "Parameter error");

    memcpy(&s_kit_init_struct, init_struct, sizeof(s_kit_init_struct));

    if (init_struct->scheduler_disable == NULL || init_struct->scheduler_enable == NULL)
    {
        s_kit_init_struct.scheduler_disable = _k_nop;
        s_kit_init_struct.scheduler_enable = _k_nop;
    }
    if (init_struct->get_work_q_hdl == NULL)
    {
        s_kit_init_struct.get_work_q_hdl = _k_nop_get_work_q_hdl;
    }

    memset(&s_cm_kit, 0, sizeof(s_cm_kit));
}

/**
 * @brief 取消初始化微内核
 *
 */
void k_deinit(void)
{
    memset(&s_kit_init_struct, 0, sizeof(s_kit_init_struct));
}

/**
 * @brief 工作队列执行入口
 * 所有在队列中就绪的任务都将被按顺序逐个执行。
 * 执行顺序为：
 * 1. 按任务的优先级
 * 2. 按就绪的顺序
 * 栈用量 32 bytes
 *
 * @param work_q_handle 由 k_work_q_create() 初始化
 * @return k_tick_t 距离下个延时任务的时间
 */
k_tick_t k_work_q_handler(k_work_q_t *work_q_handle)
{
    k_work_q_handle_t *work_q;
    k_work_t *last_work;
    k_tick_t ret;

    if (s_kit_init_struct.malloc == NULL)
    {
        return -1u;
    }

    work_q = work_q_handle->hdl;

    if (work_q == NULL)
    {
        return -1u;
    }

    last_work = work_q->curr_work;

    if (last_work != NULL)
    {
        _DBG_WRN("Not recommended executing this function in work_handler");
    }

    do
    {
        k_work_handle_t *exec_work;

        if (work_q->priority_tab != 0)
        {
            exec_work = _k_work_q_handler_take_ready_work(work_q_handle);
            if (exec_work != NULL)
            {
                exec_work->exec_flag = 1;

                if (exec_work->obj_flag_type)
                {
                    exec_work->work_route(exec_work->arg);
                    _k_timer_end(work_q_handle, exec_work);
                }
                else
                {
                    if (s_cm_kit.curr_hook != exec_work->work_handle)
                    {
                        k_work_fn hook_entry;

                        s_cm_kit.curr_hook = exec_work->work_handle;
                        _k_work_log(exec_work->work_handle);

                        hook_entry = work_q->hook_entry;
                        if (hook_entry)
                        {
                            hook_entry(exec_work->work_handle);
                        }
                    }

                    exec_work->work_route(exec_work->arg);
                }
                work_q->curr_work = last_work;

                if (exec_work->delete_flag == 0)
                {
                    _ASSERT_FALSE(!k_work_is_valid(exec_work->work_handle), "exec_work->work_handle maybe be deleted");

                    exec_work->exec_flag = 0;
                    if (exec_work->obj_flag_type == 0 && exec_work->obj_data.mbox_list != NULL)
                    {
                        if (_slist_peek_head(&exec_work->obj_data.mbox_list->mbox_idle) != NULL)
                        {
                            _K_DIS_SCHED();
                            _k_work_mbox_clr_list(&exec_work->obj_data.mbox_list->mbox_idle);
                            _K_EN_SCHED();
                        }
                    }
                }
                else
                {
                    k_work_delete(exec_work->work_handle);
                }
            }
        }
        else
        {
            exec_work = NULL;
        }

        ret = _k_work_q_handler_take_delay_list(work_q, _K_PORT->get_sys_ticks());

        if (exec_work != NULL)
        {
            break;
        }

    } while (*(volatile uint8_t *)&work_q->priority_tab != 0);

    if (work_q->priority_tab != 0)
    {
        ret = 0;
    }

    if (last_work != NULL)
    {
        if (((k_work_handle_t *)last_work->hdl)->exec_repeat != 0)
        {
            ((k_work_handle_t *)last_work->hdl)->exec_repeat = 0;
            k_work_submit(((k_work_handle_t *)last_work->hdl)->work_q_handle,
                          ((k_work_handle_t *)last_work->hdl)->work_handle,
                          0);
        }
    }

    if (work_q->resume_flag)
    {
        work_q->resume_flag = 0;
        ret = 0;
    }

    if (work_q->curr_work)
    {
        if (s_cm_kit.curr_hook != work_q->curr_work)
        {
            k_work_fn hook_entry;

            s_cm_kit.curr_hook = work_q->curr_work;
            _k_work_log(work_q->curr_work);

            hook_entry = work_q->hook_entry;
            if (hook_entry)
            {
                hook_entry(work_q->curr_work);
            }
        }
    }

    return ret;
}

/**
 * @brief 创建一个工作队列的对象
 * 对象数据由堆自动分配
 * 对象内存: 112 bytes
 *
 * @param work_q_handle[out] 初始化队列对象指针
 * @return k_err_t 0 -- 成功, -1 -- 堆内存不足
 */
k_err_t k_work_q_create(k_work_q_t *work_q_handle)
{
    k_work_q_handle_t *work_q;

    _ASSERT_FALSE(s_kit_init_struct.malloc == NULL, "Never use 'k_init()' to initialize");

    _K_DIS_SCHED();
    work_q = _K_PORT->malloc(sizeof(k_work_q_handle_t));
    if (work_q != NULL)
    {
        unsigned i;

        memset(work_q, 0, sizeof(*work_q));
        work_q_handle->hdl = work_q;
        work_q->work_q_resume = NULL;
        work_q->work_q_resume_arg = NULL;
        for (i = 0; i < sizeof(work_q->ready_list) / sizeof(work_q->ready_list[0]); i++)
        {
            _slist_init_list(&work_q->ready_list[i]);
        }
        _slist_init_list(&work_q->delayed_list);
        _dlist_init_list(&work_q->event_list);
    }
    _K_EN_SCHED();

    if (work_q == NULL)
    {
        _DBG_WRN("Insufficient memory");
        return -1;
    }
    else
    {
        return 0;
    }
}

/**
 * @brief 删除一个工作队列的对象
 * 1. 在该队列中所有已就绪的任务被立即删除
 * 2. 延时中的任务在到达时间后被立即删除
 * 3. 工作队列被删除
 * 4. 工作队列及相关任务的堆内存被自动回收
 *
 * @param work_q_handle 由 k_work_q_create() 初始化
 */
void k_work_q_delete(k_work_q_t *work_q_handle)
{
    _ASSERT_FALSE(s_kit_init_struct.malloc == NULL, "Never use 'k_init()' to initialize");

    _K_DIS_SCHED();
    if (work_q_handle->hdl != NULL)
    {
        k_work_q_handle_t *work_q = work_q_handle->hdl;

        while (*(volatile uint8_t *)&work_q->priority_tab != 0)
        {
            __slist_node_t *state_node = _slist_take_head(&work_q->ready_list[7 - _tab_clz_8[work_q->priority_tab]]);
            if (state_node == NULL)
            {
                break;
            }
            else
            {
                k_work_handle_t *work = _K_CONTAINER_OF(state_node, k_work_handle_t, state_node);
                _k_work_delete(work->work_handle);
            }
        }

        for (;;)
        {
            __dlist_node_t *node = _dlist_take_head(&work_q->event_list);
            if (node == NULL)
            {
                break;
            }
            else
            {
                k_work_handle_t *work = _K_CONTAINER_OF(node, k_work_handle_t, event_node);
                _k_work_delete(work->work_handle);
            }
        }

        for (;;)
        {
            __slist_node_t *state_node = _slist_take_head(&work_q->delayed_list);
            if (state_node == NULL)
            {
                break;
            }
            else
            {
                k_work_handle_t *work = _K_CONTAINER_OF(state_node, k_work_handle_t, state_node);
                _k_work_delete(work->work_handle);
            }
        }

        if (work_q->curr_work != NULL)
        {
            _k_work_delete(work_q->curr_work);
        }

        _K_PORT->free(work_q_handle->hdl);
        work_q_handle->hdl = NULL;
    }
    _K_EN_SCHED();
}

/**
 * @brief 注册一个函数：当有任务被提交时，此回调函数将被自动执行
 *
 * @param work_q_handle 由 k_work_q_create() 初始化
 * @param work_q_resume 当有任务被提交时，被执行的回调函数将
 * @param arg 回调函数将附带的参数
 */
void k_work_q_resume_regist(k_work_q_t *work_q_handle, k_work_q_fn work_q_resume, void *arg)
{
    if (k_work_q_is_valid(work_q_handle))
    {
        k_work_q_handle_t *work_q = work_q_handle->hdl;
        work_q->work_q_resume_arg = arg;
        work_q->work_q_resume = work_q_resume;
    }
}

/**
 * @brief 获取工作队列是否有效
 *
 * @param work_q_handle 由 k_work_q_create() 初始化
 * @retval true 对象有效
 * @retval false 对象无效
 */
bool k_work_q_is_valid(k_work_q_t *work_q_handle)
{
    if (s_kit_init_struct.malloc == NULL || work_q_handle == NULL || work_q_handle->hdl == NULL)
    {
        return _FALSE;
    }
    else
    {
        return _TRUE;
    }
}

/**
 * @brief 设置切换任务时，进入和退出任务时回调函数
 *
 * @param work_q_handle 由 k_work_q_create() 初始化
 * @param entry 在执行一个新任务时的回调函数，为 NULL 时不回调，注意同一个任务不会被连续回调
 */
void k_work_hook(k_work_q_t *work_q_handle, k_work_fn hook)
{
    k_work_q_handle_t *work_q = work_q_handle->hdl;
    work_q->hook_entry = hook;
}

/**
 * @brief 获取是否有正在延时状态的任务
 *
 * @retval true 有延时状态的任务
 * @retval false 队列已空
 */
bool k_work_q_delayed_state(k_work_q_t *work_q_handle)
{
    k_work_q_handle_t *work_q = work_q_handle->hdl;
    if (_dlist_peek_head(&work_q->event_list) != NULL)
    {
        return _TRUE;
    }
    if (_slist_peek_head(&work_q->delayed_list) != NULL)
    {
        return _TRUE;
    }
    return _FALSE;
}

/**
 * @brief 获取工作队列中是否有就绪的任务
 *
 * @param work_q_handle 由 k_work_q_create() 初始化
 * @retval true 有就绪的任务
 * @retval false 队列已空
 */
bool k_work_q_ready_state(k_work_q_t *work_q_handle)
{
    k_work_q_handle_t *work_q = work_q_handle->hdl;
    if (work_q == NULL || work_q->priority_tab == 0)
    {
        return _FALSE;
    }
    else
    {
        return _FALSE;
    }
}

/**
 * @brief 创建由工作队列管理的任务
 * 任务对象数据由堆内存自动分配
 * 任务必须返回
 * 任务堆栈来自 k_work_q_handler()
 * 对象内存: 48 bytes
 *
 * @param work_handle[out] 初始化任务对象数据
 * @param name 任务名
 * @param work_route 任务的入口地址
 * @param arg 附带的一个参数指针
 * @param priority 任务优先级，0 .. 7，0 为最低
 * @return k_err_t 0 -- 成功，-1 -- 失败：对象非空或堆内存不足
 */
k_err_t k_work_create(k_work_t *work_handle,
                      const char *name,
                      k_work_fn work_route,
                      void *arg,
                      uint8_t priority)
{
    k_work_handle_t *work;

    _ASSERT_FALSE(s_kit_init_struct.malloc == NULL, "Never use 'k_init()' to initialize");

    if (work_route == NULL)
    {
        _DBG_WRN("work_route invalid");
        return -1;
    }

    if (priority >= (8 - _tab_clz_8[sizeof(_tab_clz_8) - 1]))
    {
        _DBG_WRN("Param over range: priority = %d, New value is %d",
                 priority,
                 priority % (8 - _tab_clz_8[sizeof(_tab_clz_8) - 1]) %
                     (sizeof(((k_work_q_handle_t *)0)->ready_list) / sizeof(((k_work_q_handle_t *)0)->ready_list[0])));
    }

    _K_DIS_SCHED();
    work = _K_PORT->malloc(sizeof(k_work_handle_t));
    _K_EN_SCHED();

    if (work)
    {
        memset(work, 0, sizeof(*work));
        work->work_handle = work_handle;
        work->work_route = work_route;
        work->arg = arg;
        work->priority = priority % (8 - _tab_clz_8[sizeof(_tab_clz_8) - 1]) %
                         (sizeof(((k_work_q_handle_t *)0)->ready_list) / sizeof(((k_work_q_handle_t *)0)->ready_list[0]));
        _slist_init_node(&work->state_node);
        _dlist_init_node(&work->event_node);
#if (CONFIG_K_KIT_LOG_ON)
        work->name = name;
#endif
        work_handle->hdl = work;
        return 0;
    }
    else
    {
        _DBG_WRN("Insufficient memory");
        return -1;
    }
}

/**
 * @brief 删除由工作队列管理的任务
 * 任务立即失效
 * 任务对象数据被立即回收
 *
 * @param work_handle 由 k_work_create() 初始化
 */
void k_work_delete(k_work_t *work_handle)
{
    _ASSERT_FALSE(s_kit_init_struct.malloc == NULL, "Never use 'k_init()' to initialize");

    if (work_handle == NULL)
    {
        k_work_q_t *work_q_handle;
        _k_read_curr_handle(&work_q_handle, &work_handle);
    }

    _K_DIS_SCHED();
    k_work_handle_t *work = work_handle->hdl;
    if (work->exec_flag == 0)
    {
        _k_work_delete(work_handle);
    }
    else
    {
        work->delete_flag = 1;
    }
    _K_EN_SCHED();
}

/**
 * @brief 获取任务对象是否有效
 *
 * @param work_handle 由 k_work_create() 初始化
 * @retval true 任务对象有效
 * @retval false 任务对象无效
 */
bool k_work_is_valid(k_work_t *work_handle)
{
    if (work_handle == NULL || work_handle->hdl == NULL)
    {
        return _FALSE;
    }
    else
    {
        return _TRUE;
    }
}

/**
 * @brief 获取任务是否在就绪或延时的状态
 *
 * @param work_handle 由 k_work_create() 初始化
 * @retval true 任务就绪或延时
 * @retval false 任务被挂起
 */
bool k_work_is_pending(k_work_t *work_handle)
{
    k_work_handle_t *work = work_handle->hdl;

    if (work != NULL)
    {
        if (work->exec_flag != 0)
        {
            return _TRUE;
        }
        if (_dlist_is_pending(&work->event_node))
        {
            return _TRUE;
        }
        if (_slist_is_pending(&work->state_node))
        {
            return _TRUE;
        }
    }

    return _FALSE;
}

/**
 * @brief 获取任务距离下个执行的剩余时间
 *
 * @param work_handle
 * @return k_tick_t
 */
k_tick_t k_work_time_remain(k_work_t *work_handle)
{
    k_work_handle_t *work = work_handle->hdl;
    k_tick_t ret = 0;

    if (!_dlist_is_pending(&work->event_node) &&
        !_slist_is_pending(&work->state_node))
    {
        return -1;
    }

    ret = work->timeout - _K_PORT->get_sys_ticks();
    if (ret < 0)
    {
        ret = 0;
    }
    return ret;
}

/**
 * @brief 把任务插入到 work_q->event_list 中
 *
 * @param work_q_handle 由 k_work_q_create() 初始化
 * @param work_handle 由 k_work_create() 初始化
 * @param delay_ticks 指定的唤醒时间，为0时立即就绪，-1为永不就绪
 */
void k_work_submit(k_work_q_t *work_q_handle, k_work_t *work_handle, k_tick_t delay_ticks)
{
    if (delay_ticks == -1)
    {
        _k_work_do_submit(work_q_handle, work_handle, 0, _TRUE);
    }
    else
    {
        if ((unsigned)delay_ticks > _SIGNED_MAX(delay_ticks))
        {
            delay_ticks = _SIGNED_MAX(delay_ticks);
        }
        _k_work_do_submit(work_q_handle, work_handle, _K_PORT->get_sys_ticks() + delay_ticks, _FALSE);
    }
}

/**
 * @brief 唤醒任务。注意需要先使用 k_work_submit 绑定一个 work_q_handle
 *
 * @param work_handle 由 k_work_create() 初始化
 * @param delay_ticks 让出CPU的时间
 */
void k_work_resume(k_work_t *work_handle, k_tick_t delay_ticks)
{
    k_work_handle_t *work = work_handle->hdl;
    if (work->work_q_handle == NULL)
    {
        _DBG_WRN("you must using k_work_submit() to define one work_queue first");
    }
    else
    {
        if ((unsigned)delay_ticks > _SIGNED_MAX(delay_ticks))
        {
            delay_ticks = _SIGNED_MAX(delay_ticks);
        }
        k_work_submit(work->work_q_handle, work_handle, delay_ticks);
    }
}

/**
 * @brief 延时多少个系统节拍后再执行本任务
 *
 * @param delay_ticks 指定的唤醒时间，为0时立即就绪
 */
void k_work_later(k_tick_t delay_ticks)
{
    k_work_q_t *work_q_handle;
    k_work_t *work_handle;
    _k_read_curr_handle(&work_q_handle, &work_handle);
    if ((unsigned)delay_ticks > _SIGNED_MAX(delay_ticks))
    {
        delay_ticks = _SIGNED_MAX(delay_ticks);
    }
    k_work_submit(work_q_handle, work_handle, delay_ticks);
}

/**
 * @brief 挂起任务，任务不会被删除
 *
 * @param work_handle 由 k_work_create() 初始化
 */
void k_work_suspend(k_work_t *work_handle)
{
    k_work_handle_t *work;

    _K_DIS_INT();
    work = work_handle->hdl;
    if (work != NULL)
    {
        if (_dlist_is_pending(&work->event_node))
        {
            k_work_q_handle_t *work_q = work->work_q_handle->hdl;
            _dlist_remove(&work_q->event_list, &work->event_node);
        }
    }
    _K_EN_INT();

    _K_DIS_SCHED();
    work = work_handle->hdl;
    if (work != NULL)
    {
        _slist_remove(work->state_list, &work->state_node);
        work->state_list = NULL;
        work->exec_flag = 0;
        work->obj_flag_periodic = 0;
    }
    _K_EN_SCHED();
}

/**
 * @brief 从最后一次唤醒的时间算起，延时多少个系统节拍后再执行本任务（固定周期的延时）
 *
 * @param delay_ticks 指定的唤醒时间，为0时立即就绪
 */
void k_work_later_until(k_tick_t delay_ticks)
{
    k_work_q_t *work_q_handle;
    k_work_t *work_handle;
    k_work_handle_t *work;
    k_tick_t timeout;

    _k_read_curr_handle(&work_q_handle, &work_handle);
    work = work_handle->hdl;

    if ((unsigned)delay_ticks > _SIGNED_MAX(delay_ticks))
    {
        delay_ticks = _SIGNED_MAX(delay_ticks);
    }
    timeout = work->timeout + delay_ticks + (_K_PORT->get_sys_ticks() - work->timeout) / delay_ticks * delay_ticks;

    _k_work_do_submit(work_q_handle, work_handle, timeout, _FALSE);
}

/**
 * @brief 释放一次CPU的使用权，不调度低于当前任务优先级的任务
 * 返回机制：
 * 给予的队列中有高于当前任务优先级的任务：直到所有更高优先给的任务被执行完后；
 * 给予的队列中首个任务的优先级等于当前任务的优先级：执行完首个任务后返回。
 * 注意：不允许低优先级的任务堵塞高优先级的任务
 *
 * @param delay_ticks 让出CPU的时间
 */
void k_work_yield(k_tick_t delay_ticks)
{
    k_work_q_t *work_q_handle;
    k_work_t *work_handle;
    k_work_handle_t *yield_work;

    _ASSERT_FALSE(s_kit_init_struct.malloc == NULL, "Never use 'k_init()' to initialize");

    _k_read_curr_handle(&work_q_handle, &work_handle);
    yield_work = _WORK_Q->yield_work;
    _WORK_Q->yield_work = _WORK;

    k_work_sleep(delay_ticks);

    _WORK_Q->yield_work = yield_work;
}

/**
 * @brief 释放一次CPU的使用权，可调度低于当前任务优先级的任务
 * 返回机制：
 * 给予的队列中有高于当前任务优先级的任务：直到所有更高优先给的任务被执行完后；
 * 给予的队列中首个任务的优先级等于当前任务的优先级：执行完首个任务后返回。
 * 栈用量 24 bytes
 *
 * @param delay_ticks 让出CPU的时间
 */
void k_work_sleep(k_tick_t delay_ticks)
{
    k_work_q_t *work_q_handle;
    k_work_t *work_handle;
    k_tick_t sys_time;

    _ASSERT_FALSE(s_kit_init_struct.malloc == NULL, "Never use 'k_init()' to initialize");

    if ((unsigned)delay_ticks > _SIGNED_MAX(delay_ticks))
    {
        delay_ticks = _SIGNED_MAX(delay_ticks);
    }

    _k_read_curr_handle(&work_q_handle, &work_handle);
    sys_time = _K_PORT->get_sys_ticks();
    _WORK->timeout = sys_time + delay_ticks;

    _k_work_q_handler_take_event_list(&_WORK_Q->event_list, sys_time);

    do
    {
        if (default_timer_q_hdl->hdl != NULL &&              /* 已创建软定时器队列对象 */
            s_kit_init_struct.scheduler_disable == _k_nop && /* 非多线程 */
            k_timer_q_handler(default_timer_q_hdl) == 0)     /* 有就绪的定时任务 */
        {
            sys_time = _K_PORT->get_sys_ticks();
            continue;
        }

        if (_WORK_Q->priority_tab != 0)
        {
            k_work_handle_t *exec_work = _k_work_q_handler_take_ready_work(_WORK->work_q_handle);
            if (exec_work != NULL)
            {
                exec_work->exec_flag = 1;

                if (exec_work->obj_flag_type)
                {
                    exec_work->work_route(exec_work->arg);
                    _k_timer_end(work_q_handle, exec_work);
                }
                else
                {
                    if (s_cm_kit.curr_hook != exec_work->work_handle)
                    {
                        k_work_fn hook_entry;

                        s_cm_kit.curr_hook = exec_work->work_handle;
                        _k_work_log(exec_work->work_handle);

                        hook_entry = _WORK_Q->hook_entry;
                        if (hook_entry)
                        {
                            hook_entry(exec_work->work_handle);
                        }
                    }

                    exec_work->work_route(exec_work->arg);
                }
                _WORK_Q->curr_work = work_handle;

                if (exec_work->delete_flag == 0)
                {
                    _ASSERT_FALSE(!k_work_is_valid(exec_work->work_handle), "exec_work->work_handle maybe be deleted");

                    exec_work->exec_flag = 0;
                    if (exec_work->obj_flag_type == 0 && exec_work->obj_data.mbox_list != NULL)
                    {
                        if (_slist_peek_head(&exec_work->obj_data.mbox_list->mbox_idle) != NULL)
                        {
                            _K_DIS_SCHED();
                            _k_work_mbox_clr_list(&exec_work->obj_data.mbox_list->mbox_idle);
                            _K_EN_SCHED();
                        }
                    }
                }
                else
                {
                    k_work_delete(exec_work->work_handle);
                }
            }
        }

        if (work_handle->hdl == NULL)
        {
            break;
        }

        sys_time = _K_PORT->get_sys_ticks();
        if (_k_work_q_handler_take_delay_list(_WORK_Q, sys_time) > 0 &&
            _K_PORT->thread_sleep &&
            *(volatile int *)&_WORK->timeout - sys_time > 0)
        {
            _K_PORT->thread_sleep(1);
            sys_time = _K_PORT->get_sys_ticks();
            _k_work_q_handler_take_delay_list(_WORK_Q, sys_time);
        }

    } while (*(volatile int *)&_WORK->timeout - sys_time > 0);

    if (_WORK->exec_repeat != 0)
    {
        _WORK->exec_repeat = 0;
        k_work_submit(_WORK->work_q_handle, _WORK->work_handle, 0);
    }

    if (_WORK_Q->curr_work)
    {
        if (s_cm_kit.curr_hook != _WORK_Q->curr_work)
        {
            k_work_fn hook_entry;

            s_cm_kit.curr_hook = _WORK_Q->curr_work;
            _k_work_log(_WORK_Q->curr_work);

            hook_entry = _WORK_Q->hook_entry;
            if (hook_entry)
            {
                hook_entry(_WORK_Q->curr_work);
            }
        }
    }
}

/**
 * @brief 查询最近一次执行的任务
 *
 * @param work_q_handle 由 k_work_q_create() 初始化
 * @return k_work_t* 指向任务对象
 */
k_work_t *k_get_curr_work_handle(k_work_q_t *work_q_handle)
{
    k_work_q_handle_t *work_q = work_q_handle->hdl;
    return work_q->curr_work;
}

/**
 * @brief 创建任务的邮箱
 * 对象内存: 20 bytes
 *
 * k_mbox 与 k_fifo 的主要区别：
 * 1. 邮箱是任务的扩展，与任务关联；
 * 2. 邮件被发送的同时，对应的任务将被自动唤醒；
 * 3. 邮件收取后，任务结束时被自动删除，也可使用 k_work_mbox_cancel() 来显式删除
 *
 * @param work_handle 由 k_work_create() 初始化
 * @return k_err_t 0 -- 成功，-1 -- 任务对象无效或内存不足
 */
k_err_t k_work_mbox_create(k_work_t *work_handle)
{
    k_work_handle_t *work;
    k_work_mb_list_t *mbox_list;

    _ASSERT_FALSE(s_kit_init_struct.malloc == NULL, "Never use 'k_init()' to initialize");

    work = work_handle->hdl;
    if (work == NULL || work->obj_flag_type != 0)
    {
        _DBG_WRN("work_handle invalid");
        return -1;
    }

    if (work->obj_data.mbox_list != NULL)
    {
        _DBG_WRN("work mbox was be created");
        return -1;
    }

    if (work->work_q_handle == NULL)
    {
        _DBG_WRN("work_handle = %p was not submit to any work_q_handle handle", work);
    }

    _K_DIS_SCHED();

    mbox_list = _K_PORT->malloc(sizeof(*mbox_list));
    if (mbox_list != NULL)
    {
        _slist_init_list(&mbox_list->mbox_idle);
        _slist_init_list(&mbox_list->mbox_valid);
        work->obj_data.mbox_list = mbox_list;
    }

    _K_EN_SCHED();

    if (mbox_list == NULL)
    {
        _DBG_WRN("Insufficient memory");
        return -1;
    }
    else
    {
        return 0;
    }
}

/**
 * @brief 删除任务的邮箱
 *
 * @param work_handle 由 k_work_create() 初始化
 */
void k_work_mbox_delete(k_work_t *work_handle)
{
    k_work_handle_t *work;

    _ASSERT_FALSE(s_kit_init_struct.malloc == NULL, "Never use 'k_init()' to initialize");

    work = work_handle->hdl;
    if (work == NULL)
    {
        _DBG_WRN("work_handle invalid");
        return;
    }

    if (work->obj_flag_type != 0)
    {
        _DBG_WRN("work_handle invalid");
        return;
    }

    if (work->obj_data.mbox_list == NULL)
    {
        _DBG_WRN("work mbox never be created");
        return;
    }

    _K_DIS_SCHED();

    if (work->obj_data.mbox_list != NULL)
    {
        k_work_mb_list_t *mbox_list = work->obj_data.mbox_list;
        _k_work_mbox_clr_list(&mbox_list->mbox_idle);
        _k_work_mbox_clr_list(&mbox_list->mbox_valid);
        _K_PORT->free(mbox_list);
        work->obj_data.mbox_list = NULL;
    }

    _K_EN_SCHED();
}

/**
 * @brief 申请一个邮件
 * 邮件数据没有限制
 * 不能申请给自己的邮件
 * 管理块内存 8 bytes
 *
 * @param work_handle 由 k_work_create() 初始化
 * @param size 数据大小
 * @return void* 内存地址
 */
void *k_work_mbox_alloc(k_work_t *work_handle, size_t size)
{
    k_work_handle_t *work;
    k_work_mb_handle_t *mbox_data;

    _ASSERT_FALSE(s_kit_init_struct.malloc == NULL, "Never use 'k_init()' to initialize");

    work = work_handle->hdl;
    if (work == NULL)
    {
        _DBG_WRN("work_handle invalid");
        return NULL;
    }

    if (work->work_q_handle == NULL)
    {
        _DBG_WRN("work was not pending or to be canceled");
        return NULL;
    }

    if (work->obj_flag_type != 0)
    {
        _DBG_WRN("work_handle invalid");
        return NULL;
    }

    if (work->obj_data.mbox_list == NULL)
    {
        _DBG_WRN("never create mbox with function 'k_work_mbox_create()'");
        return NULL;
    }

    _K_DIS_SCHED();
    mbox_data = _K_PORT->malloc(sizeof(*mbox_data) + size);
    if (mbox_data != NULL)
    {
        mbox_data->ctrl.work_handle = work_handle;
    }
    _K_EN_SCHED();

    if (mbox_data == NULL)
    {
        _DBG_WRN("Insufficient memory");
        return NULL;
    }
    else
    {
        return (void *)&mbox_data[1];
    }
}

/**
 * @brief 取消已申请的邮件
 *
 * @param mbox 通过 k_work_mbox_alloc() 申请的数据
 * @return k_err_t 0 -- 成功，-1 -- 工作队列对象或任务对象无效
 */
k_err_t k_work_mbox_cancel(void *mbox)
{
    k_work_mb_handle_t *mbox_data = &((k_work_mb_handle_t *)mbox)[-1];
    k_err_t ret = 0;

    k_work_handle_t *work = mbox_data->ctrl.work_handle->hdl;
    if (work == NULL)
    {
        _DBG_WRN("work_handle invalid");
        ret = -1;
    }

    if (work->work_q_handle == NULL)
    {
        _DBG_WRN("work was not pending or to be canceled");
        ret = -1;
    }

    if (work->obj_data.mbox_list == NULL)
    {
        _DBG_WRN("never create mbox with function 'k_work_mbox_create()'");
        ret = -1;
    }

    if (ret == 0)
    {
        _K_DIS_SCHED();
        _K_PORT->free(mbox_data);
        _K_EN_SCHED();
    }

    return ret;
}

/**
 * @brief 发送邮件
 * 如果任务在延时的状态则立即被切换到就绪状态
 *
 * @param mbox 通过 k_work_mbox_alloc() 申请的数据
 * @return k_err_t 0 -- 成功，-1 -- 工作队列对象或任务对象无效
 */
k_err_t k_work_mbox_submit(void *mbox)
{
    k_work_mb_handle_t *mbox_data = &((k_work_mb_handle_t *)mbox)[-1];
    k_work_handle_t *work = mbox_data->ctrl.work_handle->hdl;
    k_err_t ret = 0;

    if (work == NULL)
    {
        _DBG_WRN("work_handle invalid");
        ret = -1;
    }
    else if (work->work_q_handle == NULL)
    {
        _DBG_WRN("work was not pending or to be canceled");
        ret = -1;
    }
    else if (work->obj_flag_type != 0)
    {
        _DBG_WRN("work_handle invalid");
        ret = -1;
    }
    else if (work->obj_data.mbox_list == NULL)
    {
        _DBG_WRN("never create mbox with function 'k_work_mbox_create()'");
        ret = -1;
    }

    if (ret != 0)
    {
        _K_DIS_SCHED();
        _K_PORT->free(mbox_data);
        _K_EN_SCHED();
        return ret;
    }

    _slist_init_node(&mbox_data->ctrl.state_node);
    _K_DIS_SCHED();
    _slist_insert_tail(&work->obj_data.mbox_list->mbox_valid, &mbox_data->ctrl.state_node);
    _K_EN_SCHED();

    k_work_submit(work->work_q_handle, work->work_handle, 0);

    return ret;
}

/**
 * @brief 提取邮件
 * 当任务返回后自动被清除
 *
 * @return void* 数据地址，NULL -- 邮箱队列为空
 */
void *k_work_mbox_take(void)
{
    k_work_q_t *work_q_handle;
    k_work_t *work_handle;
    k_work_handle_t *work;
    __slist_node_t *node;

    _k_read_curr_handle(&work_q_handle, &work_handle);
    work = work_handle->hdl;

    if (work == NULL)
    {
        _DBG_WRN("work_handle invalid");
        return NULL;
    }

    if (work->obj_flag_type != 0)
    {
        _DBG_WRN("work_handle invalid");
        return NULL;
    }

    if (work->obj_data.mbox_list == NULL)
    {
        _DBG_WRN("never create mbox with function 'k_work_mbox_create()'");
        return NULL;
    }

    _K_DIS_SCHED();

    node = _slist_take_head(&work->obj_data.mbox_list->mbox_valid);
    if (node == NULL)
    {
        _K_EN_SCHED();
        return NULL;
    }
    else
    {
        k_work_mb_handle_t *mbox_data = _K_CONTAINER_OF(node, k_work_mb_handle_t, ctrl.state_node);
        _slist_insert_tail(&work->obj_data.mbox_list->mbox_idle, &mbox_data->ctrl.state_node);
        _K_EN_SCHED();
        return &mbox_data[1];
    }
}

/**
 * @brief 查询邮件
 *
 * @return void* 数据地址，NULL -- 邮箱队列为空
 */
void *k_work_mbox_peek(void)
{
    k_work_q_t *work_q_handle;
    k_work_t *work_handle;
    k_work_handle_t *work;
    __slist_node_t *node;

    _k_read_curr_handle(&work_q_handle, &work_handle);
    work = work_handle->hdl;

    if (work == NULL)
    {
        _DBG_WRN("work_handle invalid");
        return NULL;
    }

    if (work->obj_flag_type != 0)
    {
        _DBG_WRN("work_handle invalid");
        return NULL;
    }

    if (work->obj_data.mbox_list == NULL)
    {
        _DBG_WRN("never create mbox with function 'k_work_mbox_create()'");
        return NULL;
    }

    node = _slist_peek_head(&work->obj_data.mbox_list->mbox_valid);
    if (node == NULL)
    {
        return NULL;
    }
    else
    {
        k_work_mb_handle_t *mbox_data = _K_CONTAINER_OF(node, k_work_mb_handle_t, ctrl.state_node);
        return (void *)&mbox_data[1];
    }
}

/**
 * @brief 清空任务的所有邮件
 */
void k_work_mbox_clr(void)
{
    k_work_q_t *work_q_handle;
    k_work_t *work_handle;
    k_work_handle_t *work;

    _ASSERT_FALSE(s_kit_init_struct.malloc == NULL, "Never use 'k_init()' to initialize");

    _k_read_curr_handle(&work_q_handle, &work_handle);
    work = work_handle->hdl;

    if (work == NULL)
    {
        _DBG_WRN("work_handle invalid");
        return;
    }

    if (work->obj_flag_type != 0)
    {
        _DBG_WRN("work_handle invalid");
        return;
    }

    if (work->obj_data.mbox_list == NULL)
    {
        _DBG_WRN("work mbox never be created");
        return;
    }

    _K_DIS_SCHED();

    if (work->obj_data.mbox_list != NULL)
    {
        _k_work_mbox_clr_list(&work->obj_data.mbox_list->mbox_idle);
        _k_work_mbox_clr_list(&work->obj_data.mbox_list->mbox_valid);
    }

    _K_EN_SCHED();
}

/**
 * @brief 软定时器队列执行入口
 * 所有在队列中就绪的定时器都将被按顺序逐个执行。
 * 执行顺序为：
 * 1. 按定时器的优先级
 * 2. 按就绪的顺序
 * 栈用量 32 bytes
 *
 * @param timer_q_handle 由 k_timer_q_create() 初始化
 * @return k_tick_t 距离下个延时定时器的时间
 */
k_tick_t k_timer_q_handler(k_timer_q_t *timer_q_handle)
{
    return k_work_q_handler((k_work_q_t *)timer_q_handle);
}

/**
 * @brief 创建一个软定时器队列的对象
 * 对象数据由堆自动分配
 * 对象内存: 104 bytes
 *
 * @param timer_q_handle[out] 初始化队列对象指针
 * @return k_err_t 0 -- 成功, -1 -- 堆内存不足
 */
k_err_t k_timer_q_create(k_timer_q_t *timer_q_handle)
{
    return k_work_q_create((k_work_q_t *)timer_q_handle);
}

/**
 * @brief 删除一个软定时器队列的对象
 * 1. 在该队列中所有已就绪的定时器被立即删除
 * 2. 延时中的定时器在到达时间后被立即删除
 * 3. 软定时器队列被删除
 * 4. 软定时器队列及相关定时器的堆内存被自动回收
 *
 * @param timer_q_handle 由 k_timer_q_create() 初始化
 */
void k_timer_q_delete(k_timer_q_t *timer_q_handle)
{
    k_work_q_delete((k_work_q_t *)timer_q_handle);
}

/**
 * @brief 注册一个函数：当有任务被提交时，此回调函数将被自动执行
 *
 * @param timer_q_handle 由 k_timer_q_create() 初始化
 * @param timer_q_resume 当有任务被提交时，被执行的回调函数将
 * @param arg 回调函数将附带的参数
 */
void k_timer_q_resume_regist(k_timer_q_t *timer_q_handle, k_timer_q_fn timer_q_resume, void *arg)
{
    k_work_q_resume_regist((k_work_q_t *)timer_q_handle, (k_work_q_fn)timer_q_resume, arg);
}

/**
 * @brief 获取软定时器队列是否有效
 *
 * @param timer_q_handle 由 k_timer_q_create() 初始化
 * @retval true 对象有效
 * @retval false 对象无效
 */
bool k_timer_q_is_valid(k_timer_q_t *timer_q_handle)
{
    return k_work_q_is_valid((k_work_q_t *)timer_q_handle);
}

/**
 * @brief 获取是否有正在延时状态的定时器
 *
 * @retval true 有延时状态的定时器
 * @retval false 队列已空
 */
bool k_timer_q_delayed_state(k_timer_q_t *timer_q_handle)
{
    return k_work_q_delayed_state((k_work_q_t *)timer_q_handle);
}

/**
 * @brief 获取软定时器队列中是否有就绪的定时器
 *
 * @param timer_q_handle 由 k_timer_q_create() 初始化
 * @retval true 有就绪的定时器
 * @retval false 队列已空
 */
bool k_timer_q_ready_state(k_timer_q_t *timer_q_handle)
{
    return k_work_q_ready_state((k_work_q_t *)timer_q_handle);
}

/**
 * @brief 创建由软定时器队列管理的定时器
 * 定时器对象数据由堆内存自动分配
 * 定时器必须返回
 * 定时器堆栈来自 k_timer_q_handler()
 * 对象内存: 48 bytes
 *
 * @param timer_handle[out] 初始化定时器对象数据
 * @param timer_q_handle 由 k_timer_q_create() 初始化
 * @param timer_route 定时器的入口地址
 * @param arg 附带的一个参数指针
 * @param priority 定时器优先级，0 .. 7，0 为最低
 * @return k_err_t 0 -- 成功，-1 -- 失败：对象非空或堆内存不足
 */
k_err_t k_timer_create(k_timer_t *timer_handle,
                       k_timer_q_t *timer_q_handle,
                       k_timer_fn timer_route,
                       void *arg,
                       uint8_t priority)
{
    _ASSERT_FALSE(s_kit_init_struct.malloc == NULL, "Never use 'k_init()' to initialize");

    if (timer_q_handle == NULL || timer_q_handle->hdl == NULL)
    {
        _DBG_WRN("timer_q_handle invalid");
        return -1;
    }

    if (k_work_create((k_work_t *)timer_handle, "", (k_work_fn)timer_route, arg, priority) == 0)
    {
        k_work_handle_t *work = timer_handle->hdl;
        work->work_q_handle = (k_work_q_t *)timer_q_handle;
        work->obj_flag_type = 1;
        work->obj_data.period = _SIGNED_MAX(work->obj_data.period);
        return 0;
    }
    else
    {
        return -1;
    }
}

/**
 * @brief 删除由软定时器队列管理的定时器
 * 定时器立即失效
 * 定时器对象数据被立即回收
 *
 * @param timer_handle 由 k_timer_create() 初始化
 */
void k_timer_delete(k_timer_t *timer_handle)
{
    _ASSERT_FALSE(s_kit_init_struct.malloc == NULL, "Never use 'k_init()' to initialize");

    if (timer_handle == NULL)
    {
        k_work_q_t *work_q_handle;
        _k_read_curr_handle(&work_q_handle, (k_work_t **)&timer_handle);
    }

    _K_DIS_SCHED();
    k_work_handle_t *work = timer_handle->hdl;
    if (work->exec_flag == 0)
    {
        _k_work_delete((k_work_t *)timer_handle);
    }
    else
    {
        work->delete_flag = 1;
    }
    _K_EN_SCHED();
}

/**
 * @brief 设置定时器的自动重装值
 *
 * @param timer_handle 由 k_timer_create() 初始化
 * @param periodic 是否自动重装
 * @param period 重装值
 */
void k_timer_set_period(k_timer_t *timer_handle, bool periodic, k_tick_t period)
{
    k_work_handle_t *work = timer_handle->hdl;
    if (work)
    {
        if ((unsigned)period > _SIGNED_MAX(period))
        {
            period = _SIGNED_MAX(period);
        }
        _K_DIS_SCHED();
        work->obj_flag_periodic = periodic;
        work->obj_data.period = period;
        _K_EN_SCHED();
    }
    else
    {
        _DBG_WRN("you must using k_timer_create() to create one timer_handle first");
    }
}

/**
 * @brief 启动定时器
 *
 * @param timer_handle 由 k_timer_create() 初始化
 * @param periodic 是否自动重装
 * @param period 重装值
 * @param delay_ticks 延时启动
 */
void k_timer_start(k_timer_t *timer_handle, k_tick_t delay_ticks)
{
    k_work_handle_t *work = timer_handle->hdl;
    if (work)
    {
        _k_work_do_submit(work->work_q_handle, (k_work_t *)timer_handle, _K_PORT->get_sys_ticks() + delay_ticks, _FALSE);
    }
}

/**
 * @brief 挂起定时器，定时器不会被删除
 *
 * @param timer_handle 由 k_timer_create() 初始化
 */
void k_timer_stop(k_timer_t *timer_handle)
{
    k_work_suspend((k_work_t *)timer_handle);
}

/**
 * @brief 生成一个临时定时器，执行完自动删除
 *
 * @param timer_q_handle 由 k_timer_q_create() 初始化
 * @param timer_route
 * @param arg
 * @param delay_ticks
 * @return k_err_t
 */
k_err_t k_timer_newsubmit(k_timer_q_t *timer_q_handle, k_timer_fn timer_route, void *arg, k_tick_t delay_ticks)
{
    k_timer_t timer_handle;

    memset(&timer_handle, 0, sizeof(timer_handle));

    if (k_timer_create(
            &timer_handle,
            timer_q_handle,
            timer_route,
            arg,
            0) == 0)
    {
        k_work_handle_t *work = timer_handle.hdl;
        work->obj_flag_auto_del = 1;
        k_timer_set_period(&timer_handle, _FALSE, delay_ticks);
        k_timer_start(&timer_handle, delay_ticks);
        return 0;
    }
    else
    {
        return -1;
    }
}

/**
 * @brief 删除并重新生成一个新的临时定时器，执行完自动删除
 *
 * @param timer_q_handle 由 k_timer_q_create() 初始化
 * @param timer_route
 * @param arg
 * @param delay_ticks
 * @return k_err_t
 */
k_err_t k_timer_resubmit(k_timer_q_t *timer_q_handle, k_timer_fn timer_route, void *arg, k_tick_t delay_ticks)
{
    k_timer_cancel(timer_q_handle, timer_route);
    return k_timer_newsubmit(timer_q_handle, timer_route, arg, delay_ticks);
}

/**
 * @brief 立即删除临时定时器
 *
 * @param timer_q_handle 由 k_timer_q_create() 初始化
 * @param timer_route
 */
void k_timer_cancel(k_timer_q_t *timer_q_handle, k_timer_fn timer_route)
{
    k_work_q_handle_t *work_q;

    _K_DIS_SCHED();

    work_q = timer_q_handle->hdl;
    if (work_q != NULL)
    {
        do
        {
            __dlist_t *test_list = &work_q->event_list;
            __dlist_node_t *test_node = _dlist_peek_head(test_list);
            __dlist_node_t *end_node = _dlist_peek_tail(test_list);
            __dlist_node_t *prev_node = NULL;

            do
            {
                k_work_handle_t *test_work;

                if (test_node == NULL)
                {
                    break;
                }
                test_work = _K_CONTAINER_OF(test_node, k_work_handle_t, event_node);

                if ((void *)test_work->work_route == (void *)timer_route)
                {
                    _dlist_remove(test_list, test_node);
                    if (prev_node == NULL)
                    {
                        test_node = _dlist_peek_head(test_list);
                    }
                    else
                    {
                        test_node = _dlist_peek_next(test_list, prev_node);
                    }
                    _K_PORT->free(test_work);
                }
                else
                {
                    prev_node = test_node;
                    test_node = _dlist_peek_next(test_list, prev_node);
                }

            } while (prev_node != end_node);

        } while (0);

        do
        {
            __slist_t *test_list = &work_q->delayed_list;
            __slist_node_t *test_node = _slist_peek_head(test_list);
            __slist_node_t *end_node = _slist_peek_tail(test_list);
            __slist_node_t *prev_node = NULL;

            do
            {
                k_work_handle_t *test_work;

                if (test_node == NULL)
                {
                    break;
                }
                test_work = _K_CONTAINER_OF(test_node, k_work_handle_t, state_node);

                if ((void *)test_work->work_route == (void *)timer_route)
                {
                    _slist_remove_next(test_list, prev_node, test_node);
                    if (prev_node == NULL)
                    {
                        test_node = _slist_peek_head(test_list);
                    }
                    else
                    {
                        test_node = _slist_peek_next(prev_node);
                    }
                    _K_PORT->free(test_work);
                }
                else
                {
                    prev_node = test_node;
                    test_node = _slist_peek_next(prev_node);
                }

            } while (prev_node != end_node);

        } while (0);

        do
        {
            unsigned i;
            for (i = 0; i < sizeof(work_q->ready_list) / sizeof(work_q->ready_list[0]); i++)
            {
                __slist_t *test_list = &work_q->ready_list[i];
                __slist_node_t *test_node = _slist_peek_head(test_list);
                __slist_node_t *end_node = _slist_peek_tail(test_list);
                __slist_node_t *prev_node = NULL;

                do
                {
                    k_work_handle_t *test_work;

                    if (test_node == NULL)
                    {
                        break;
                    }
                    test_work = _K_CONTAINER_OF(test_node, k_work_handle_t, state_node);

                    if ((void *)test_work->work_route == (void *)timer_route)
                    {
                        _slist_remove_next(test_list, prev_node, test_node);
                        if (prev_node == NULL)
                        {
                            test_node = _slist_peek_head(test_list);
                        }
                        else
                        {
                            test_node = _slist_peek_next(prev_node);
                        }
                        _K_PORT->free(test_work);
                    }
                    else
                    {
                        prev_node = test_node;
                        test_node = _slist_peek_next(prev_node);
                    }

                } while (prev_node != end_node);
            }

        } while (0);
    }

    _K_EN_SCHED();
}

/**
 * @brief 获取定时器对象是否有效
 *
 * @param timer_handle 由 k_timer_create() 初始化
 * @retval true 定时器对象有效
 * @retval false 定时器对象无效
 */
bool k_timer_is_valid(k_timer_t *timer_handle)
{
    return k_work_is_valid((k_work_t *)timer_handle);
}

/**
 * @brief 获取定时器是否在就绪或延时的状态
 *
 * @param timer_handle 由 k_timer_create() 初始化
 * @retval true 定时器就绪或延时
 * @retval false 定时器被挂起
 */
bool k_timer_is_pending(k_timer_t *timer_handle)
{
    return k_work_is_pending((k_work_t *)timer_handle);
}

/**
 * @brief 查询当前是否自动重装
 *
 * @param timer_handle 由 k_timer_create() 初始化
 * @retval true 当前状态为自动重装
 * @retval false 当前状态为单次模式
 */
bool k_timer_is_periodic(k_timer_t *timer_handle)
{
    k_work_handle_t *work = timer_handle->hdl;
    return (bool)!!work->obj_flag_periodic;
}

/**
 * @brief 查询当前的自动重装值
 *
 * @param timer_handle
 * @return k_tick_t
 */
k_tick_t k_timer_get_period(k_timer_t *timer_handle)
{
    k_work_handle_t *work = timer_handle->hdl;
    return work->obj_data.period;
}

/**
 * @brief 获取定时器距离下个执行的剩余时间
 *
 * @param timer_handle
 * @return k_tick_t
 */
k_tick_t k_timer_time_remain(k_timer_t *timer_handle)
{
    return k_work_time_remain((k_work_t *)timer_handle);
}

/**
 * @brief 创建一个FIFO对象
 * 对象数据由堆自动分配
 * 对象内存: 20 bytes
 *
 * @param fifo_handle[out] 初始化FIFO对象指针
 * @return k_err_t 0 -- 成功, -1 -- 堆内存不足
 */
k_err_t k_fifo_q_create(k_fifo_t *fifo_handle)
{
    k_fifo_q_handle_t *fifo_q;

    _ASSERT_FALSE(s_kit_init_struct.malloc == NULL, "Never use 'k_init()' to initialize");

    _K_DIS_SCHED();
    fifo_q = _K_PORT->malloc(sizeof(k_fifo_q_handle_t));
    if (fifo_q != NULL)
    {
        fifo_handle->hdl = fifo_q;
        fifo_q->resume_work = NULL;
        fifo_q->resume_delay = 0;
        _slist_init_list(&fifo_q->list);
    }
    _K_EN_SCHED();

    if (fifo_q == NULL)
    {
        _DBG_WRN("Insufficient memory");
        return -1;
    }
    else
    {
        return 0;
    }
}

/**
 * @brief 删除一个FIFO的对象
 * 1. 在该队列中所有数据被立即删除
 * 2. FIFO对象被删除
 * 3. FIFO对象及数据的堆内存被自动回收
 *
 * @param fifo_handle 由 k_fifo_q_create() 初始化
 */
void k_fifo_q_delete(k_fifo_t *fifo_handle)
{
    k_fifo_q_handle_t *fifo_q;

    _K_DIS_SCHED();
    fifo_q = fifo_handle->hdl;
    if (fifo_q != NULL)
    {
        for (;;)
        {
            __slist_node_t *node = _slist_take_head(&fifo_q->list);
            if (node == NULL)
            {
                break;
            }
            else
            {
                k_fifo_handle_t *fifo_data = _K_CONTAINER_OF(node, k_fifo_handle_t, ctrl.state_node);
                _K_PORT->free(fifo_data);
            }
        }
        _K_PORT->free(fifo_q);
        fifo_handle->hdl = NULL;
    }
    _K_EN_SCHED();
}

/**
 * @brief 清除FIFO内的所有数据
 *
 * @param fifo_handle 由 k_fifo_q_create() 初始化
 */
void k_fifo_q_clr(k_fifo_t *fifo_handle)
{
    k_fifo_q_handle_t *fifo_q;

    _ASSERT_FALSE(s_kit_init_struct.malloc == NULL, "Never use 'k_init()' to initialize");

    _K_DIS_SCHED();
    fifo_q = fifo_handle->hdl;
    if (fifo_q != NULL)
    {
        for (;;)
        {
            __slist_node_t *node = _slist_take_head(&fifo_q->list);
            if (node == NULL)
            {
                break;
            }
            else
            {
                k_fifo_handle_t *fifo_data = _K_CONTAINER_OF(node, k_fifo_handle_t, ctrl.state_node);
                _K_PORT->free(fifo_data);
            }
        }
    }
    _K_EN_SCHED();
}

/**
 * @brief 获取 FIFO 对象是否有效
 *
 * @param fifo_handle 由 k_fifo_q_create() 初始化
 * @retval true
 * @retval false
 */
bool k_fifo_q_is_valid(k_fifo_t *fifo_handle)
{
    if (s_kit_init_struct.malloc == NULL || fifo_handle == NULL || fifo_handle->hdl == NULL)
    {
        return _FALSE;
    }
    else
    {
        return _TRUE;
    }
}

/**
 * @brief 注册任务，当队列非空时，任务被唤醒。
 * 注意同时只能注册一个任务
 *
 * @param fifo_handle 由 k_fifo_q_create() 初始化
 * @param work_handle 由 k_work_create() 初始化
 * @param delay_ticks 延迟唤醒时间
 */
void k_fifo_q_regist(k_fifo_t *fifo_handle, k_work_t *work_handle, k_tick_t delay_ticks)
{
    k_fifo_q_handle_t *fifo_q = fifo_handle->hdl;
    if (fifo_q != NULL)
    {
        fifo_q->resume_work = work_handle;
        fifo_q->resume_delay = delay_ticks;
    }
}

/**
 * @brief 取消注册任务
 *
 * @param fifo_handle 由 k_fifo_q_create() 初始化
 */
void k_fifo_q_unregist(k_fifo_t *fifo_handle)
{
    k_fifo_q_handle_t *fifo_q = fifo_handle->hdl;
    fifo_q->resume_work = NULL;
}

/**
 * @brief 申请可用于FIFO的数据结构
 * 管理块内存 8 bytes
 *
 * @param size 数据结构大小，单位为字节
 * @return void* 数据结构地址，为 NULL 时表示内存不足
 */
void *k_fifo_alloc(size_t size)
{
    k_fifo_handle_t *fifo_data;

    _ASSERT_FALSE(s_kit_init_struct.malloc == NULL, "Never use 'k_init()' to initialize");

    _K_DIS_SCHED();
    fifo_data = _K_PORT->malloc(sizeof(*fifo_data) + size);
    _K_EN_SCHED();

    if (fifo_data == NULL)
    {
        _DBG_WRN("Insufficient memory");
        return NULL;
    }
    else
    {
        fifo_data->ctrl.free_flag = _K_FIFO_FREE;
        return (void *)&fifo_data[1];
    }
}

/**
 * @brief 释放由 k_fifo_alloc() 申请的数据结构
 * 注意数据不允许在FIFO中时被释放
 *
 * @param data 数据结构地址
 * @return k_err_t 0 -- 成功，-1 -- 数据未从FIFO中提取或数据无效
 */
k_err_t k_fifo_free(void *data)
{
    k_fifo_handle_t *fifo_data;

    _ASSERT_FALSE(s_kit_init_struct.malloc == NULL, "Never use 'k_init()' to initialize");

    fifo_data = &((k_fifo_handle_t *)data)[-1];

    _K_DIS_SCHED();
    if (fifo_data->ctrl.free_flag == _K_FIFO_FREE)
    {
        fifo_data->ctrl.free_flag = 0;
        _K_PORT->free(fifo_data);
        fifo_data = NULL;
    }
    _K_EN_SCHED();

    if (fifo_data != NULL)
    {
        _DBG_WRN("%p was not in fifo queue, can not to free", data);
        return -1;
    }
    else
    {
        return 0;
    }
}

/**
 * @brief 把数据结构压入到FIFO中
 *
 * @param fifo_handle 由 k_fifo_q_create() 初始化
 * @param data 数据结构
 * @return k_err_t 0 -- 成功，-1 -- FIFO对象无效或数据结不是由 k_fifo_alloc() 所创建
 */
k_err_t k_fifo_put(k_fifo_t *fifo_handle, void *data)
{
    k_fifo_q_handle_t *fifo_q = fifo_handle->hdl;
    k_fifo_handle_t *fifo_data = &((k_fifo_handle_t *)data)[-1];
    bool resume_flag = false;

    if (fifo_q == NULL)
    {
        _DBG_WRN("fifo_handle invalid");
        return -1;
    }

    if (data == NULL || fifo_data->ctrl.free_flag != _K_FIFO_FREE)
    {
        _DBG_WRN("fifo_data invalid");
        return -1;
    }

    _slist_init_node(&fifo_data->ctrl.state_node);
    _K_DIS_INT();
    if (_slist_peek_head(&fifo_q->list) == NULL)
    {
        resume_flag = true;
    }
    _slist_insert_tail(&fifo_q->list, &fifo_data->ctrl.state_node);
    _K_EN_INT();

    if (resume_flag)
    {
        _k_ipc_resume(fifo_q->resume_work, fifo_q->resume_delay, 0);
    }

    return 0;
}

/**
 * @brief 从FIFO中弹出最先压入的数据
 *
 * @param fifo_handle 由 k_fifo_q_create() 初始化
 * @return void* 数据地址，NULL -- FIFO队列为空
 */
void *k_fifo_take(k_fifo_t *fifo_handle)
{
    k_fifo_q_handle_t *fifo_q = fifo_handle->hdl;
    __slist_node_t *node;

    if (fifo_q == NULL)
    {
        _DBG_WRN("fifo_handle invalid");
        return NULL;
    }

    _K_DIS_INT();
    node = _slist_take_head(&fifo_q->list);
    _K_EN_INT();

    if (node == NULL)
    {
        return NULL;
    }
    else
    {
        k_fifo_handle_t *fifo_data = _K_CONTAINER_OF(node, k_fifo_handle_t, ctrl.state_node);
        fifo_data->ctrl.free_flag = _K_FIFO_FREE;
        return (void *)&fifo_data[1];
    }
}

/**
 * @brief 查询FIFO中头部的数据地址
 *
 * @param fifo_handle 由 k_fifo_q_create() 初始化
 * @return void* 数据地址，NULL -- FIFO队列为空
 */
void *k_fifo_peek_head(k_fifo_t *fifo_handle)
{
    k_fifo_q_handle_t *fifo_q;
    __slist_node_t *node;

    if (fifo_handle->hdl == NULL)
    {
        return NULL;
    }

    fifo_q = fifo_handle->hdl;
    node = _slist_peek_head(&fifo_q->list);
    if (node == NULL)
    {
        return NULL;
    }
    else
    {
        k_fifo_handle_t *fifo_data = _K_CONTAINER_OF(node, k_fifo_handle_t, ctrl.state_node);
        return (void *)&fifo_data[1];
    }
}

/**
 * @brief 查询FIFO中尾部的数据地址
 *
 * @param fifo_handle 由 k_fifo_q_create() 初始化
 * @return void* 数据地址，NULL -- FIFO队列为空
 */
void *k_fifo_peek_tail(k_fifo_t *fifo_handle)
{
    k_fifo_q_handle_t *fifo_q;
    __slist_node_t *node;

    if (fifo_handle->hdl == NULL)
    {
        return NULL;
    }

    fifo_q = fifo_handle->hdl;
    node = _slist_peek_tail(&fifo_q->list);
    if (node == NULL)
    {
        return NULL;
    }
    else
    {
        k_fifo_handle_t *fifo_data = _K_CONTAINER_OF(node, k_fifo_handle_t, ctrl.state_node);
        return (void *)&fifo_data[1];
    }
}

/**
 * @brief 创建一个QUEUE对象
 * 对象数据由堆自动分配
 * 对象内存: 32 bytes
 *
 * @param queue_handle[out] 初始化QUEUE对象指针
 * @param queue_length 指定队列有多少顶
 * @param item_size 指定每项的长度（字节）
 * @return k_err_t 0 -- 成功, -1 -- 堆内存不足
 */
k_err_t k_queue_create(k_queue_t *queue_handle, size_t queue_length, size_t item_size)
{
    size_t real_item_size;
    k_queue_handle_t *queue;

    _ASSERT_FALSE(s_kit_init_struct.malloc == NULL, "Never use 'k_init()' to initialize");

    real_item_size = (size_t) & ((k_queue_data_t *)0)[1] + (item_size + sizeof(size_t) - 1) / sizeof(size_t) * sizeof(size_t);

    _K_DIS_SCHED();
    queue = _K_PORT->malloc(sizeof(k_queue_handle_t) + real_item_size * queue_length);
    if (queue != NULL)
    {
        size_t i;
        queue_handle->hdl = queue;
        queue->item_size = item_size;
        queue->resume_work = NULL;
        queue->resume_delay = 0;
        _slist_init_list(&queue->list_valid);
        _slist_init_list(&queue->list_idle);
        for (i = 0; i < queue_length; i++)
        {
            k_queue_data_t *queue_data = (k_queue_data_t *)&((uint8_t *)&queue[1])[real_item_size * i];
            queue_data->queue_handle = queue_handle;
            _slist_init_node(&queue_data->ctrl.state_node);
            _slist_insert_tail(&queue->list_idle, &queue_data->ctrl.state_node);
        }
    }
    _K_EN_SCHED();

    if (queue == NULL)
    {
        _DBG_WRN("Insufficient memory");
        return -1;
    }
    else
    {
        return 0;
    }
}

/**
 * @brief 删除一个QUEUE的对象
 * 1. 在该队列中所有数据被立即删除
 * 2. QUEUE对象被删除
 * 3. QUEUE对象及数据的堆内存被自动回收
 *
 * @param queue_handle 由 k_queue_create() 初始化
 */
void k_queue_delete(k_queue_t *queue_handle)
{
    k_queue_handle_t *queue;

    _K_DIS_SCHED();
    queue = queue_handle->hdl;
    if (queue != NULL)
    {
        _K_PORT->free(queue);
        queue_handle->hdl = NULL;
    }
    _K_EN_SCHED();
}

/**
 * @brief 清除QUEUE内的所有数据
 *
 * @param queue_handle 由 k_queue_create() 初始化
 */
void k_queue_clr(k_queue_t *queue_handle)
{
    k_queue_handle_t *queue;
    __slist_node_t *node;

    _ASSERT_FALSE(s_kit_init_struct.malloc == NULL, "Never use 'k_init()' to initialize");

    queue = queue_handle->hdl;
    if (queue != NULL)
    {
        for (;;)
        {
            _K_DIS_INT();
            node = _slist_take_head(&queue->list_valid);
            if (node == NULL)
            {
                _K_EN_INT();
                break;
            }
            else
            {
                _slist_insert_font(&queue->list_idle, node);
                _K_EN_INT();
            }
        }
    }
}

/**
 * @brief 获取 QUEUE 对象是否有效
 *
 * @param queue_handle 由 k_queue_create() 初始化
 * @retval true
 * @retval false
 */
bool k_queue_is_valid(k_queue_t *queue_handle)
{
    if (s_kit_init_struct.malloc == NULL || queue_handle == NULL || queue_handle->hdl == NULL)
    {
        return _FALSE;
    }
    else
    {
        return _TRUE;
    }
}

/**
 * @brief 注册任务，当队列非空时，任务被唤醒。
 * 注意同时只能注册一个任务
 *
 * @param queue_handle 由 k_queue_create() 初始化
 * @param work_handle 由 k_work_create() 初始化
 * @param delay_ticks 延迟唤醒时间
 */
void k_queue_regist(k_queue_t *queue_handle, k_work_t *work_handle, k_tick_t delay_ticks)
{
    k_queue_handle_t *queue = queue_handle->hdl;
    if (queue != NULL)
    {
        queue->resume_work = work_handle;
        queue->resume_delay = delay_ticks;
    }
}

/**
 * @brief 取消注册任务
 *
 * @param queue_handle 由 k_queue_create() 初始化
 */
void k_queue_unregist(k_queue_t *queue_handle)
{
    k_queue_handle_t *queue = queue_handle->hdl;
    queue->resume_work = NULL;
}

/**
 * @brief 接收并复制数据
 * 等价于 k_queue_take() + memcpy() + k_queue_free()
 *
 * @param queue_handle 由 k_queue_create() 初始化
 * @param dst 保存接收到的数据
 * @return k_err_t 0 接收成功，-1 队列中没有数据
 */
k_err_t k_queue_recv(k_queue_t *queue_handle, void *dst)
{
    k_queue_handle_t *queue;
    void *data = k_queue_take(queue_handle);
    if (data == NULL)
    {
        return -1;
    }

    queue = queue_handle->hdl;
    memcpy(dst, data, queue->item_size);

    k_queue_free(data);

    return 0;
}

/**
 * @brief 复制数据并发送
 * 等价于 k_queue_alloc() + memcpy() + k_queue_put()
 *
 * @param queue_handle 由 k_queue_create() 初始化
 * @param src 发送的数据
 * @return k_err_t 0 发送成功，-1 队列已满
 */
k_err_t k_queue_send(k_queue_t *queue_handle, const void *src)
{
    k_queue_handle_t *queue;
    void *data = k_queue_alloc(queue_handle);
    if (data == NULL)
    {
        return -1;
    }

    queue = queue_handle->hdl;
    memcpy(data, src, queue->item_size);

    k_queue_put(data);

    return 0;
}

/**
 * @brief 申请可用于QUEUE的数据结构
 * 管理块内存 4 bytes
 *
 * @param queue_handle 由 k_queue_create() 初始化
 * @return void* 数据结构地址，为0时表示内存不足
 */
void *k_queue_alloc(k_queue_t *queue_handle)
{
    k_queue_handle_t *queue = queue_handle->hdl;
    __slist_node_t *node;

    _ASSERT_FALSE(s_kit_init_struct.malloc == NULL, "Never use 'k_init()' to initialize");

    if (queue == NULL)
    {
        _DBG_WRN("queue_handle invalid");
        return NULL;
    }

    _K_DIS_INT();
    node = _slist_take_head(&queue->list_idle);
    _K_EN_INT();

    if (node == NULL)
    {
        return NULL;
    }
    else
    {
        k_queue_data_t *queue_data = _K_CONTAINER_OF(node, k_queue_data_t, ctrl.state_node);
        queue_data->ctrl.free_flag = _K_QUEUE_FREE;
        return (void *)&queue_data[1];
    }
}

/**
 * @brief 释放由 k_queue_alloc() 申请的数据结构
 * 注意数据不允许在QUEUE中时被释放
 *
 * @param data 由 k_queue_alloc() 或 k_queue_take() 获取
 * @return k_err_t 0 -- 成功，-1 -- 数据未从QUEUE中弹出或数据无效
 */
k_err_t k_queue_free(void *data)
{
    k_queue_data_t *queue_data;
    k_queue_handle_t *queue;

    _ASSERT_FALSE(s_kit_init_struct.malloc == NULL, "Never use 'k_init()' to initialize");

    if (data == NULL)
    {
        _DBG_WRN("data invalid");
        return -1;
    }

    queue_data = &((k_queue_data_t *)data)[-1];
    queue = (k_queue_handle_t *)queue_data->queue_handle->hdl;

    if (queue == NULL)
    {
        _DBG_WRN("queue_handle invalid");
        return -1;
    }
    if (queue_data->ctrl.free_flag != _K_QUEUE_FREE)
    {
        _DBG_WRN("%p was freed", data);
        return -1;
    }

    _slist_init_node(&queue_data->ctrl.state_node);
    _K_DIS_INT();
    _slist_insert_font(&queue->list_idle, &queue_data->ctrl.state_node);
    _K_EN_INT();

    return 0;
}

/**
 * @brief 把数据结构压入到QUEUE中
 *
 * @param data 由 k_queue_alloc() 或 k_queue_take() 获取
 * @return k_err_t 0 -- 成功，-1 -- QUEUE对象无效或数据结不是由 k_queue_alloc() 所创建
 */
k_err_t k_queue_put(void *data)
{
    k_queue_data_t *queue_data;
    k_queue_handle_t *queue;
    bool resume_flag = false;

    if (data == NULL)
    {
        _DBG_WRN("data invalid");
        return -1;
    }

    queue_data = &((k_queue_data_t *)data)[-1];
    queue = (k_queue_handle_t *)queue_data->queue_handle->hdl;

    if (queue == NULL)
    {
        _DBG_WRN("queue_handle invalid");
        return -1;
    }
    if (queue_data->ctrl.free_flag != _K_QUEUE_FREE)
    {
        _DBG_WRN("%p was freed", data);
        return -1;
    }

    _slist_init_node(&queue_data->ctrl.state_node);
    _K_DIS_INT();
    if (_slist_peek_head(&queue->list_valid) == NULL)
    {
        resume_flag = true;
    }
    _slist_insert_tail(&queue->list_valid, &queue_data->ctrl.state_node);
    _K_EN_INT();

    if (resume_flag)
    {
        _k_ipc_resume(queue->resume_work, queue->resume_delay, 0);
    }

    return 0;
}

/**
 * @brief 从QUEUE中弹出最先压入的数据
 *
 * @param queue_handle 由 k_queue_create() 初始化
 * @return void* 数据地址，NULL -- QUEUE队列为空
 */
void *k_queue_take(k_queue_t *queue_handle)
{
    k_queue_handle_t *queue = queue_handle->hdl;
    __slist_node_t *node;

    if (queue == NULL)
    {
        _DBG_WRN("queue_handle invalid");
        return NULL;
    }

    _K_DIS_INT();
    node = _slist_take_head(&queue->list_valid);
    _K_EN_INT();

    if (node == NULL)
    {
        return NULL;
    }
    else
    {
        k_queue_data_t *queue_data = _K_CONTAINER_OF(node, k_queue_data_t, ctrl.state_node);
        queue_data->ctrl.free_flag = _K_QUEUE_FREE;
        return (void *)&queue_data[1];
    }
}

/**
 * @brief 查询QUEUE中头部的数据地址
 *
 * @param queue_handle 由 k_queue_create() 初始化
 * @return void* 数据地址，NULL -- QUEUE队列为空
 */
void *k_queue_peek_head(k_queue_t *queue_handle)
{
    k_queue_handle_t *queue;
    __slist_node_t *node;

    if (queue_handle == NULL || queue_handle->hdl == NULL)
    {
        return NULL;
    }

    queue = queue_handle->hdl;
    node = _slist_peek_head(&queue->list_valid);
    if (node == NULL)
    {
        return NULL;
    }
    else
    {
        k_queue_data_t *queue_data = _K_CONTAINER_OF(node, k_queue_data_t, ctrl.state_node);
        return (void *)&queue_data[1];
    }
}

/**
 * @brief 查询QUEUE中尾部的数据地址
 *
 * @param queue_handle 由 k_queue_create() 初始化
 * @return void* 数据地址，NULL -- QUEUE队列为空
 */
void *k_queue_peek_tail(k_queue_t *queue_handle)
{
    k_queue_handle_t *queue;
    __slist_node_t *node;

    if (queue_handle == NULL || queue_handle->hdl == NULL)
    {
        return NULL;
    }

    queue = queue_handle->hdl;
    node = _slist_peek_tail(&queue->list_valid);
    if (node == NULL)
    {
        return NULL;
    }
    else
    {
        k_queue_data_t *queue_data = _K_CONTAINER_OF(node, k_queue_data_t, ctrl.state_node);
        return (void *)&queue_data[1];
    }
}

/**
 * @brief 查询已申请数据的所属队列句柄
 *
 * @param data 已申请数据的数据地址
 * @return k_queue_t 所属队列句柄
 */
k_queue_t *k_queue_read_handle(void *data)
{
    k_queue_data_t *queue_data;

    if (data == NULL)
    {
        _DBG_WRN("data invalid");
        return NULL;
    }

    queue_data = &((k_queue_data_t *)data)[-1];
    return queue_data->queue_handle;
}

/**
 * @brief 读回 k_queue_create() 中设置的 item_size 的值
 *
 * @param queue_handle 由 k_queue_create() 初始化
 * @return size_t k_queue_create() 中设置的 item_size 的值
 */
size_t k_queue_get_item_size(k_queue_t *queue_handle)
{
    k_queue_handle_t *queue;

    if (queue_handle == NULL || queue_handle->hdl == NULL)
    {
        return 0;
    }

    queue = queue_handle->hdl;
    return queue->item_size;
}

/**
 * @brief 创建一个管道对象
 * 对象内存: 24 bytes
 *
 * @param pipe_handle[out] 初始化管道对象
 * @param pipe_size 可保存的最大长度
 * @return k_err_t 0 -- 成功, -1 -- 堆内存不足
 */
k_err_t k_pipe_create(k_pipe_t *pipe_handle, size_t pipe_size)
{
    k_pipe_handle_t *pipe;

    _ASSERT_FALSE(s_kit_init_struct.malloc == NULL, "Never use 'k_init()' to initialize");

    pipe_size++;

    _K_DIS_SCHED();
    pipe = _K_PORT->malloc(sizeof(k_pipe_handle_t) + pipe_size);
    if (pipe != NULL)
    {
        pipe_handle->hdl = pipe;
        pipe->size = pipe_size;
        pipe->resume_work = NULL;
        pipe->resume_delay = 0;
    }
    _K_EN_SCHED();

    if (pipe == NULL)
    {
        _DBG_WRN("Insufficient memory");
        return -1;
    }
    else
    {
        k_pipe_clr(pipe_handle);
        return 0;
    }
}

/**
 * @brief 删除一个管道对象
 *
 * @param pipe_handle 由 k_pipe_create() 初始化
 */
void k_pipe_delete(k_pipe_t *pipe_handle)
{
    _ASSERT_FALSE(s_kit_init_struct.malloc == NULL, "Never use 'k_init()' to initialize");

    _K_DIS_SCHED();
    if (pipe_handle->hdl != NULL)
    {
        _K_PORT->free(pipe_handle->hdl);
        pipe_handle->hdl = NULL;
    }
    _K_EN_SCHED();
}

/**
 * @brief 清空管道的数据
 *
 * @param pipe_handle 由 k_pipe_create() 初始化
 */
void k_pipe_clr(k_pipe_t *pipe_handle)
{
    k_pipe_handle_t *pipe = (k_pipe_handle_t *)pipe_handle->hdl;

    if (pipe == NULL)
    {
        _DBG_WRN("fifo_handle invalid");
        return;
    }

    _K_DIS_SCHED();
    pipe->wid = 0;
    pipe->rid = 0;
    _K_EN_SCHED();
}

/**
 * @brief 获取 PIPE 对象是否有效
 *
 * @param pipe_handle 由 k_pipe_create() 初始化
 * @retval true
 * @retval false
 */
bool k_pipe_is_valid(k_pipe_t *pipe_handle)
{
    if (s_kit_init_struct.malloc == NULL || pipe_handle == NULL || pipe_handle->hdl == NULL)
    {
        return _FALSE;
    }
    else
    {
        return _TRUE;
    }
}

/**
 * @brief 注册任务，当队列非空时，任务被唤醒。
 * 注意同时只能注册一个任务
 *
 * @param pipe_handle 由 k_pipe_create() 初始化
 * @param work_handle 由 k_work_create() 初始化
 * @param delay_ticks 延迟唤醒时间
 */
void k_pipe_regist(k_pipe_t *pipe_handle, k_work_t *work_handle, k_tick_t delay_ticks)
{
    k_pipe_handle_t *pipe = pipe_handle->hdl;
    if (pipe != NULL)
    {
        pipe->resume_work = work_handle;
        pipe->resume_delay = delay_ticks;
    }
}

/**
 * @brief 取消注册任务
 *
 * @param pipe_handle 由 k_pipe_create() 初始化
 */
void k_pipe_unregist(k_pipe_t *pipe_handle)
{
    k_pipe_handle_t *pipe = pipe_handle->hdl;
    pipe->resume_work = NULL;
}

/**
 * @brief 写一个字节到缓存中（写入缓存）
 *
 * @param pipe_handle 由 k_pipe_create() 初始化
 * @param data 值（字节）
 * @return size_t 返回实际复制成功的字节数
 */
size_t k_pipe_poll_write(k_pipe_t *pipe_handle, uint8_t data)
{
    k_pipe_handle_t *pipe = (k_pipe_handle_t *)pipe_handle->hdl;
    bool resume_flag;

    if (pipe)
    {
        pipe_id_t wid = pipe->wid;

        if (++wid >= pipe->size)
        {
            wid = 0;
        }

        if (wid != pipe->rid)
        {
            ((uint8_t *)&pipe[1])[pipe->wid] = data;

            _K_DIS_INT();
            resume_flag = (pipe->wid == pipe->rid);
            pipe->wid = wid;
            _K_EN_INT();

            if (resume_flag)
            {
                _k_ipc_resume(pipe->resume_work, pipe->resume_delay, 0);
            }

            return 1;
        }
    }

    return 0;
}

/**
 * @brief 把内存数据复制到缓存中
 *
 * @param pipe_handle 由 k_pipe_create() 初始化
 * @param data 来源数据。如果值为 NULL 时，仅执行填充
 * @param size 来源数据大小（字节）
 * @return size_t 返回实际复制成功的字节数
 */
size_t k_pipe_fifo_fill(k_pipe_t *pipe_handle, const void *data, size_t size)
{
    size_t ret = 0;
    k_pipe_handle_t *pipe = (k_pipe_handle_t *)pipe_handle->hdl;
    bool resume_flag;

    if (pipe)
    {
        pipe_id_t wid = pipe->wid;

        resume_flag = (wid == pipe->rid);

        while (size)
        {
            pipe_id_t rid = *(volatile pipe_id_t *)&pipe->rid;
            pipe_id_t min;
            pipe_id_t remain;

            if (wid >= rid)
            {
                remain = pipe->size - wid - (rid == 0);
            }
            else
            {
                remain = rid - wid - 1;
            }
            if (remain == 0)
            {
                break;
            }
            min = size < remain ? size : remain;

            if (data)
            {
                memcpy(&((uint8_t *)&pipe[1])[wid], &((uint8_t *)data)[ret], min);
            }

            if ((wid += min) >= pipe->size)
            {
                wid = 0;
            }

            size -= min;
            ret += min;
        }
        _K_DIS_INT();
        resume_flag = (pipe->wid == pipe->rid);
        pipe->wid = wid;
        _K_EN_INT();

        if (resume_flag && ret)
        {
            _k_ipc_resume(pipe->resume_work, pipe->resume_delay, 0);
        }
    }

    return ret;
}

/**
 * @brief 从缓存复制一个字节到指定地址中
 *
 * @param pipe_handle 由 k_pipe_create() 初始化
 * @param data 目标内存地址
 * @return size_t 返回 0 表示缓存空
 */
size_t k_pipe_poll_read(k_pipe_t *pipe_handle, uint8_t *data)
{
    k_pipe_handle_t *pipe = (k_pipe_handle_t *)pipe_handle->hdl;

    if (pipe)
    {
        pipe_id_t rid = pipe->rid;
        if (rid != pipe->wid)
        {
            data[0] = ((uint8_t *)&pipe[1])[rid++];
            if (rid < pipe->size)
            {
                pipe->rid = rid;
            }
            else
            {
                pipe->rid = 0;
            }
            return 1;
        }
    }

    return 0;
}

/**
 * @brief 从管道中复制数据到内存（从缓存读取）。注：参数 data 值可以为 NULL，此时不复制数据，只释放相应的数据量
 *
 * @param pipe_handle 由 k_pipe_create() 初始化
 * @param data 目标内存地址。如果值为 NULL 时，仅执行释放
 * @param size 目标内存大小（字节）
 * @return size_t 返回实际复制成功的字节数
 */
size_t k_pipe_fifo_read(k_pipe_t *pipe_handle, void *data, size_t size)
{
    size_t ret = 0;
    k_pipe_handle_t *pipe = (k_pipe_handle_t *)pipe_handle->hdl;

    if (pipe)
    {
        pipe_id_t rid = pipe->rid;
        while (size)
        {
            pipe_id_t wid = *(volatile pipe_id_t *)&pipe->wid;
            pipe_id_t min;
            pipe_id_t remain;

            if (rid > wid)
            {
                remain = pipe->size - rid;
            }
            else
            {
                remain = wid - rid;
            }
            if (remain == 0)
            {
                break;
            }
            min = size < remain ? size : remain;

            if (data)
            {
                memcpy(&((uint8_t *)data)[ret], &((uint8_t *)&pipe[1])[rid], min);
            }

            if ((rid += min) >= pipe->size)
            {
                rid = 0;
            }

            size -= min;
            ret += min;
        }
        pipe->rid = rid;
    }

    return ret;
}

/**
 * @brief 获取管道非空
 *
 * @param pipe_handle 由 k_pipe_create() 初始化
 * @retval true 有数据
 * @retval false 表示空
 */
bool k_pipe_is_ne(k_pipe_t *pipe_handle)
{
    if (pipe_handle->hdl == NULL)
    {
        return _FALSE;
    }
    else
    {
        k_pipe_handle_t *pipe = (k_pipe_handle_t *)pipe_handle->hdl;
        if (pipe->rid == pipe->wid)
        {
            return _FALSE;
        }
        else
        {
            return _TRUE;
        }
    }
}

/**
 * @brief 获取管道的数据大小（字节数）
 *
 * @param pipe_handle 由 k_pipe_create() 初始化
 * @return size_t 数据大小（字节数）
 */
size_t k_pipe_get_valid_size(k_pipe_t *pipe_handle)
{
    if (pipe_handle->hdl == NULL)
    {
        return 0;
    }
    else
    {
        k_pipe_handle_t *pipe = (k_pipe_handle_t *)pipe_handle->hdl;
        if (pipe->wid < pipe->rid)
        {
            return pipe->size + pipe->wid - pipe->rid;
        }
        else
        {
            return pipe->wid - pipe->rid;
        }
    }
}

/**
 * @brief 获取管道的剩余空间（字节数）
 *
 * @param pipe_handle 由 k_pipe_create() 初始化
 * @return size_t 总剩余大小（字节数）
 */
size_t k_pipe_get_empty_size(k_pipe_t *pipe_handle)
{
    if (pipe_handle->hdl == NULL)
    {
        return 0;
    }
    else
    {
        k_pipe_handle_t *pipe = (k_pipe_handle_t *)pipe_handle->hdl;
        return pipe->size - (k_pipe_get_valid_size(pipe_handle) + 1);
    }
}

/**
 * @brief 获取当前已写入的连续的内存信息。
 * pipe 实际是一环形缓存结构，获取的 dst_size 是指在缓存中的连续的长度，因此并不一定等于总长度。
 * 当需要读取已缓存数据时，取得当前已缓存的数据的连续内存信息后，可对缓存的直接访问，以节省多次复制内存的资源开销，
 * 配合使用 k_pipe_fifo_read(pipe_handle, NULL, size) 直接释放对应的长度。
 *
 * @param pipe_handle   由 k_pipe_create() 初始化
 * @param dst_base[out] 已缓存数据的内存地址
 * @param dst_size[out] 已缓存数据的连续长度（字节）
 */
void k_pipe_peek_valid(k_pipe_t *pipe_handle, void **dst_base, size_t *dst_size)
{
    if (pipe_handle->hdl)
    {
        k_pipe_handle_t *pipe = (k_pipe_handle_t *)pipe_handle->hdl;
        if (dst_base)
        {
            *dst_base = (&((uint8_t *)&pipe[1])[pipe->rid]);
        }
        if (dst_size)
        {
            pipe_id_t rid = pipe->rid;
            pipe_id_t wid = pipe->wid;
            if (rid > wid)
            {
                *dst_size = pipe->size - rid;
            }
            else
            {
                *dst_size = wid - rid;
            }
        }
    }
}

/**
 * @brief 获取当前空闲的连续的内存信息。
 * pipe 实际是一环形缓存结构，获取的 dst_size 是指在缓存中的连续的长度，因此并不一定等于总长度。
 * 当需要写入缓存数据时，此函数用于取得当前空闲的连续内存信息后，可对缓存的直接访问，以节省多次复制内存的资源开销，
 * 配合使用 k_pipe_fifo_fill(pipe_handle, NULL, size) 直接填充对应的长度。
 *
 * @param pipe_handle   由 k_pipe_create() 初始化
 * @param dst_base[out] 当前空闲的内存地址
 * @param dst_size[out] 当前空闲的连续长度（字节）
 */
void k_pipe_peek_empty(k_pipe_t *pipe_handle, void **dst_base, size_t *dst_size)
{
    if (pipe_handle->hdl)
    {
        k_pipe_handle_t *pipe = (k_pipe_handle_t *)pipe_handle->hdl;
        if (dst_base)
        {
            *dst_base = (&((uint8_t *)&pipe[1])[pipe->wid]);
        }
        if (dst_size)
        {
            pipe_id_t rid = pipe->rid;
            pipe_id_t wid = pipe->wid;
            if (wid >= rid)
            {
                *dst_size = pipe->size - wid - (rid == 0);
            }
            else
            {
                *dst_size = rid - wid - 1;
            }
        }
    }
}

/**
 * @brief 获取当前系统时间
 *
 * @return k_tick_t 当前系统时间
 */
k_tick_t k_get_sys_ticks(void)
{
    return _K_PORT->get_sys_ticks();
}

/**
 * @brief 禁止中断
 *
 */
void k_disable_interrupt(void)
{
    _K_DIS_INT();
}

/**
 * @brief 恢复中断
 *
 */
void k_enable_interrupt(void)
{
    _K_EN_INT();
}

/**
 * @brief 打印当前调度日志。
 * 在调度的过程中会实时记录调度和嵌套顺序，
 * 通过本函数打印出具日志。
 * 需要配置 CONFIG_K_KIT_LOG_ON 为非 0 时生效
 * 注意若当前执行的任务未发生变化时不会打印
 */
void k_log_sched(void)
{
#if (CONFIG_K_KIT_LOG_ON)
#define _WORK_NAME(WORK) ((k_work_handle_t *)WORK->hdl)->name
    k_work_t *work = s_cm_kit.curr_hook;
    unsigned flag;

    if (s_cm_kit.curr_log == work)
    {
        return;
    }
    else
    {
        s_cm_kit.curr_log = work;
    }

    _K_DIS_SCHED();

    CONFIG_K_KIT_PRINT("\r\n");

    do
    {
        unsigned index;
        const char *arg;
        unsigned max_index;

        flag = 0;

        max_index = sizeof(s_cm_kit.work_log) / sizeof(s_cm_kit.work_log[0]);
        if (max_index > s_cm_kit.log_index)
        {
            max_index = s_cm_kit.log_index;
        }

        CONFIG_K_KIT_PRINT("[work_q]");

        for (index = 0; index < max_index; index++)
        {
            if (s_cm_kit.work_log[index] == work)
            {
                index++;
                break;
            }
            arg = _WORK_NAME(s_cm_kit.work_log[index]);
            if (arg == NULL)
                arg = "Unknow";
            CONFIG_K_KIT_PRINT(" ==> '" _COLOR_C "%s" _COLOR_END "'", arg);
        }

        arg = _WORK_NAME(work);
        if (arg == NULL)
            arg = "Unknow";
        if (index < s_cm_kit.log_max)
        {
            if (s_cm_kit.log_index > sizeof(s_cm_kit.work_log) / sizeof(s_cm_kit.work_log[0]))
            {
                CONFIG_K_KIT_PRINT(" ==> ... ==> '" _COLOR_C "%s" _COLOR_END "' >>>\r\n", arg);
            }
            else
            {
                unsigned i;

                CONFIG_K_KIT_PRINT(" ==> '" _COLOR_C "%s" _COLOR_END "'", arg);

                max_index = sizeof(s_cm_kit.work_log) / sizeof(s_cm_kit.work_log[0]);
                if (max_index > s_cm_kit.log_max)
                {
                    max_index = s_cm_kit.log_max;
                }
                for (i = index; i < max_index; i++)
                {
                    arg = _WORK_NAME(s_cm_kit.work_log[i]);
                    if (arg == NULL)
                        arg = "Unknow";
                    CONFIG_K_KIT_PRINT(" <<< '" _COLOR_C "%s" _COLOR_END "'", arg);
                    s_cm_kit.work_log[i] = NULL;
                }

                if (s_cm_kit.log_max > sizeof(s_cm_kit.work_log) / sizeof(s_cm_kit.work_log[0]))
                {
                    CONFIG_K_KIT_PRINT(" <<< ...");
                }

                CONFIG_K_KIT_PRINT("\r\n");

                s_cm_kit.log_max = index;
                flag = 1;
            }
        }
        else
        {
            CONFIG_K_KIT_PRINT(" ==> '" _COLOR_C "%s" _COLOR_END "' >>>\r\n", arg);
        }

    } while (flag);

    _K_EN_SCHED();
#undef _WORK_NAME
#endif
}
