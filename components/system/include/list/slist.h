/**
 * @file slist.h
 * @author LokLiang (lokliang@163.com)
 * @brief
 * @version 0.1
 * @date 2023-05-01
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef __SINGLELIST_H__
#define __SINGLELIST_H__

#include "sys_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef LIST_NODE_TYPE
#define LIST_NODE_TYPE         0        /* 0 -- 使用指针连结（推荐）；1 -- 使用偏移量连结，内存的地址允许被迁移并继续管理（推荐） */
#endif

#if LIST_NODE_TYPE == 1
typedef int list_node_t;
#else
typedef void * list_node_t;
#endif

typedef struct
{
    list_node_t next;
} slist_node_t;
typedef struct
{
    list_node_t head;
    list_node_t tail;
} slist_t;

__static_inline void slist_init_list (slist_t *list);
__static_inline void slist_init_node (slist_node_t *node);

__static_inline int  slist_insert_font (slist_t *list, slist_node_t *node);
__static_inline int  slist_insert_tail (slist_t *list, slist_node_t *node);
__static_inline int  slist_remove      (slist_t *list, slist_node_t *node);
__static_inline int  slist_remove_next (slist_t *list, slist_node_t *prev_node, slist_node_t *remove_node);

__static_inline int  slist_insert_next (slist_t *list, slist_node_t *tar_node, slist_node_t *new_node);
__static_inline int  slist_insert_prev (slist_t *list, slist_node_t *tar_node, slist_node_t *new_node);

__static_inline slist_node_t *slist_take_head (slist_t *list);
__static_inline slist_node_t *slist_take_tail (slist_t *list);

__static_inline slist_node_t *slist_peek_head (slist_t *list);
__static_inline slist_node_t *slist_peek_tail (slist_t *list);

__static_inline slist_node_t *slist_peek_next (slist_node_t *node);
__static_inline slist_node_t *slist_peek_prev (slist_t *list, slist_node_t *node);

__static_inline slist_node_t *slist_peek_next_loop (slist_t *list, slist_node_t *node);
__static_inline slist_node_t *slist_peek_prev_loop (slist_t *list, slist_node_t *node);

__static_inline int slist_is_pending (slist_node_t *node);

#define SLIST_CONTAINER_OF(PTR, TYPE, MEMBER) ((TYPE *)&((uint8_t *)PTR)[-(int)&((TYPE *)0)->MEMBER])

#define SLIST_FOR_EACH_NODE(__sl, __sn)      \
    for (__sn = slist_peek_head(__sl); __sn; \
         __sn = slist_peek_next(__sn))

#define SLIST_ITERATE_FROM_NODE(__sl, __sn)           \
    for (__sn = __sn ? slist_peek_next_no_check(__sn) \
                     : slist_peek_head(__sl);         \
         __sn;                                        \
         __sn = slist_peek_next(__sn))

#define SLIST_FOR_EACH_NODE_SAFE(__sl, __sn, __sns) \
    for (__sn = slist_peek_head(__sl),              \
        __sns = slist_peek_next(__sn);              \
         __sn;                                      \
         __sn = __sns, __sns = slist_peek_next(__sn))

#define SLIST_CONTAINER(__ln, __cn, __n) \
    ((__ln) ? SLIST_CONTAINER_OF((__ln), __typeof__(*(__cn)), __n) : NULL)

#define SLIST_PEEK_HEAD_CONTAINER(__sl, __cn, __n) \
    SLIST_CONTAINER(slist_peek_head(__sl), __cn, __n)

#define SLIST_PEEK_TAIL_CONTAINER(__sl, __cn, __n) \
    SLIST_CONTAINER(slist_peek_tail(__sl), __cn, __n)

#define SLIST_PEEK_NEXT_CONTAINER(__cn, __n) \
    ((__cn) ? SLIST_CONTAINER(slist_peek_next(&((__cn)->__n)), __cn, __n) : NULL)

#define SLIST_FOR_EACH_CONTAINER(__sl, __cn, __n)           \
    for (__cn = SLIST_PEEK_HEAD_CONTAINER(__sl, __cn, __n); \
         __cn;                                              \
         __cn = SLIST_PEEK_NEXT_CONTAINER(__cn, __n))

#define SLIST_FOR_EACH_CONTAINER_SAFE(__sl, __cn, __cns, __n) \
    for (__cn = SLIST_PEEK_HEAD_CONTAINER(__sl, __cn, __n),   \
        __cns = SLIST_PEEK_NEXT_CONTAINER(__cn, __n);         \
         __cn;                                                \
         __cn = __cns, __cns = SLIST_PEEK_NEXT_CONTAINER(__cn, __n))

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if LIST_NODE_TYPE == 1
#define SLIST_SET(Value1, Value2)       (Value1 = Value2 == 0 ? 0 : ((int)Value2 - (int)&Value1) | 1)
#define SLIST_GET(Value)                ((slist_node_t*)(Value == 0 ? 0 : ((int)&Value + Value) & ~1))
#else
#define SLIST_SET(Value1, Value2)       (Value1 = (list_node_t)Value2)
#define SLIST_GET(Value)                ((slist_node_t*)(Value))
#endif

__static_inline void slist_init_list(slist_t *list)
{
    SLIST_SET(list->head, 0);
    SLIST_SET(list->tail, 0);
}
__static_inline void slist_init_node(slist_node_t *node)
{
    SLIST_SET(node->next, 0);
}
__static_inline int slist_insert_font(slist_t *list, slist_node_t *node)
{
    if (SLIST_GET(node->next) != NULL)
    {
        // SYS_LOG_WRN("Node is pending");
        return -1;
    }

    slist_node_t *head_node = SLIST_GET(list->head);
    if (head_node)
    {
        SLIST_SET(node->next, head_node);
    }
    else
    {
        SLIST_SET(node->next, node);
        SLIST_SET(list->tail, node);
    }
    SLIST_SET(list->head, node);

    return 0;
}
__static_inline int slist_insert_tail(slist_t *list, slist_node_t *node)
{
    if (SLIST_GET(node->next) != NULL)
    {
        // SYS_LOG_WRN("Node is pending");
        return -1;
    }

    SLIST_SET(node->next, node);
    if (SLIST_GET(list->head) == NULL)
    {
        SLIST_SET(list->head, node);
    }
    else
    {
        slist_node_t *tail_node = (slist_node_t *)SLIST_GET(list->tail);

        SLIST_SET(tail_node->next, node);
    }
    SLIST_SET(list->tail, node);

    return 0;
}
__static_inline int slist_remove(slist_t *list, slist_node_t *node)
{
    if (node == NULL || list == NULL)
    {
        return -1;
    }

    slist_node_t *list_head = SLIST_GET(list->head);
    slist_node_t *node_next = SLIST_GET(node->next);
    if (node_next == NULL || list_head == NULL)
    {
        return -1;
    }

    if (node == list_head) // node 是首个节点
    {
        if (node == SLIST_GET(list->tail)) // node 是最后一个节点
        {
            SLIST_SET(list->head, 0);
            SLIST_SET(list->tail, 0);
        }
        else
        {
            SLIST_SET(list->head, node_next);
        }
    }
    else // node 不是首个节点
    {
        slist_node_t *prev_node = slist_peek_prev(list, node);
        if (prev_node == NULL)
        {
            return -1;
        }

        if (node == SLIST_GET(list->tail)) // node 是最后一个节点
        {
            SLIST_SET(list->tail, prev_node);
            SLIST_SET(prev_node->next, prev_node);
        }
        else
        {
            SLIST_SET(prev_node->next, node_next);
        }
    }

    SLIST_SET(node->next, 0);

    return 0;
}
__static_inline int slist_remove_next(slist_t *list, slist_node_t *prev_node, slist_node_t *remove_node)
{
    if (remove_node == NULL || list == NULL)
    {
        return -1;
    }

    slist_node_t *list_head = SLIST_GET(list->head);
    slist_node_t *node_next = SLIST_GET(remove_node->next);
    if (node_next == NULL || list_head == NULL)
    {
        return -1;
    }

    if (remove_node == list_head) // remove_node 是首个节点
    {
        if (remove_node == SLIST_GET(list->tail)) // remove_node 是最后一个节点
        {
            SLIST_SET(list->head, 0);
            SLIST_SET(list->tail, 0);
        }
        else
        {
            SLIST_SET(list->head, node_next);
        }
    }
    else // remove_node 不是首个节点
    {
        if (prev_node == NULL)
        {
            prev_node = slist_peek_prev(list, remove_node);
            if (prev_node == NULL)
            {
                return -1;
            }
        }

        if (remove_node == SLIST_GET(list->tail)) // remove_node 是最后一个节点
        {
            SLIST_SET(list->tail, prev_node);
            SLIST_SET(prev_node->next, prev_node);
        }
        else
        {
            SLIST_SET(prev_node->next, node_next);
        }
    }

    SLIST_SET(remove_node->next, 0);

    return 0;
}
__static_inline int slist_insert_next(slist_t *list, slist_node_t *tar_node, slist_node_t *new_node)
{
    slist_node_t *tar_node_next = SLIST_GET(tar_node->next);
    if (SLIST_GET(list->head) == NULL || tar_node_next == NULL || SLIST_GET(new_node->next) != NULL)
    {
        return -1;
    }

    if (tar_node == SLIST_GET(list->tail)) // tar_node 是最后一个节点
    {
        SLIST_SET(new_node->next, new_node);
        SLIST_SET(tar_node->next, new_node);
        SLIST_SET(list->tail, new_node);
    }
    else
    {
        SLIST_SET(new_node->next, tar_node_next);
        SLIST_SET(tar_node->next, new_node);
    }

    return 0;
}
__static_inline int slist_insert_prev(slist_t *list, slist_node_t *tar_node, slist_node_t *new_node)
{
    slist_node_t *list_head = SLIST_GET(list->head);
    if (list_head == NULL || SLIST_GET(tar_node->next) == NULL || SLIST_GET(new_node->next) != NULL)
    {
        return -1;
    }

    if (tar_node == list_head) // tar_node 是首个节点
    {
        SLIST_SET(new_node->next, list_head);
        SLIST_SET(list->head, new_node);
    }
    else
    {
        slist_node_t *prev_node = slist_peek_prev(list, tar_node);
        if (prev_node == NULL)
        {
            return -1;
        }

        SLIST_SET(new_node->next, tar_node);
        SLIST_SET(prev_node->next, new_node);
    }

    return 0;
}

__static_inline slist_node_t *slist_take_head(slist_t *list)
{
    slist_node_t *ret = SLIST_GET(list->head);
    slist_remove(list, ret);
    return ret;
}
__static_inline slist_node_t *slist_take_tail(slist_t *list)
{
    slist_node_t *ret = SLIST_GET(list->tail);
    slist_remove(list, ret);
    return ret;
}

__static_inline slist_node_t *slist_peek_head(slist_t *list)
{
    return SLIST_GET(list->head);
}
__static_inline slist_node_t *slist_peek_tail(slist_t *list)
{
    return SLIST_GET(list->tail);
}

__static_inline slist_node_t *slist_peek_next(slist_node_t *node)
{
    slist_node_t *next_node = SLIST_GET(node->next);
    if (next_node == node)
    {
        return NULL;
    }
    else
    {
        return next_node;
    }
}
__static_inline slist_node_t *slist_peek_prev(slist_t *list, slist_node_t *node)
{
    slist_node_t *prev_node = NULL;
    slist_node_t *test_node = SLIST_GET(list->head);
    while (test_node && test_node != node && prev_node != test_node)
    {
        prev_node = test_node;
        test_node = SLIST_GET(test_node->next);
    }
    if (test_node == node)
    {
        return prev_node;
    }

    return NULL;
}

__static_inline slist_node_t *slist_peek_next_loop(slist_t *list, slist_node_t *node)
{
    slist_node_t *next_node = SLIST_GET(node->next);
    if (next_node == node)
    {
        return SLIST_GET(list->head);
    }
    else
    {
        return next_node;
    }
}
__static_inline slist_node_t *slist_peek_prev_loop(slist_t *list, slist_node_t *node)
{
    slist_node_t *prev_node = SLIST_GET(list->tail);
    slist_node_t *test_node = SLIST_GET(list->head);
    while (test_node && test_node != node && prev_node != test_node)
    {
        prev_node = test_node;
        test_node = SLIST_GET(test_node->next);
    }
    if (test_node == node)
    {
        return prev_node;
    }

    return NULL;
}

__static_inline int slist_is_pending(slist_node_t *node)
{
    return (SLIST_GET(node->next) != NULL);
}

#ifdef __cplusplus
}
#endif

#endif
