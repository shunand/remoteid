/**
 * @file dlist.h
 * @author LokLiang (lokliang@163.com)
 * @brief
 * @version 0.1
 * @date 2023-05-01
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef __DUPLEXLIST_H__
#define __DUPLEXLIST_H__

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
    list_node_t prev;
} dlist_node_t;
typedef struct
{
    list_node_t head;
} dlist_t;

__static_inline void dlist_init_list (dlist_t *list);
__static_inline void dlist_init_node (dlist_node_t *node);

__static_inline int  dlist_insert_font (dlist_t *list, dlist_node_t *node);
__static_inline int  dlist_insert_tail (dlist_t *list, dlist_node_t *node);
__static_inline int  dlist_remove      (dlist_t *list, dlist_node_t *node);

__static_inline int  dlist_insert_next (dlist_t *list, dlist_node_t *tar_node, dlist_node_t *new_node);
__static_inline int  dlist_insert_prev (dlist_t *list, dlist_node_t *tar_node, dlist_node_t *new_node);

__static_inline void dlist_set_next (dlist_t *list);

__static_inline dlist_node_t *dlist_take_head (dlist_t *list);
__static_inline dlist_node_t *dlist_take_tail (dlist_t *list);

__static_inline dlist_node_t *dlist_peek_head (dlist_t *list);
__static_inline dlist_node_t *dlist_peek_tail (dlist_t *list);

__static_inline dlist_node_t *dlist_peek_next (dlist_t *list, dlist_node_t *node);
__static_inline dlist_node_t *dlist_peek_prev (dlist_t *list, dlist_node_t *node);

__static_inline dlist_node_t *dlist_peek_next_loop (dlist_t *list, dlist_node_t *node);
__static_inline dlist_node_t *dlist_peek_prev_loop (dlist_t *list, dlist_node_t *node);

__static_inline int dlist_is_pending (dlist_node_t *node);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if LIST_NODE_TYPE == 1
#define DLIST_SET(Value1, Value2)       (Value1 = Value2 == 0 ? 0 : ((int)Value2 - (int)&Value1) | 1)
#define DLIST_GET(Value)                ((dlist_node_t*)(Value == 0 ? 0 : ((int)&Value + Value) & ~1))
#else
#define DLIST_SET(Value1, Value2)       (Value1 = (list_node_t)Value2)
#define DLIST_GET(Value)                ((dlist_node_t*)(Value))
#endif

__static_inline void dlist_init_list(dlist_t *list)
{
    DLIST_SET(list->head, 0);
}

__static_inline void dlist_init_node(dlist_node_t *node)
{
    DLIST_SET(node->next, 0);
    DLIST_SET(node->prev, 0);
}

__static_inline int dlist_insert_font(dlist_t *list, dlist_node_t *node)
{
    if (dlist_insert_tail(list, node) == 0)
    {
        DLIST_SET(list->head, node);
        return 0;
    }
    else
    {
        return -1;
    }
}

__static_inline int dlist_insert_tail(dlist_t *list, dlist_node_t *node)
{
    if (DLIST_GET(node->next) != NULL || DLIST_GET(node->prev) != NULL)
    {
        return -1;
    }

    dlist_node_t *first_node = DLIST_GET(list->head);
    if (first_node == NULL)
    {
        /* 直接设置链头 */
        DLIST_SET(node->next, node);
        DLIST_SET(node->prev, node);
        DLIST_SET(list->head, node);
    }
    else
    {
        dlist_node_t *first_node_prev = DLIST_GET(first_node->prev);

        DLIST_SET(node->next, first_node);
        DLIST_SET(node->prev, first_node_prev);
        DLIST_SET(first_node_prev->next, node);
        DLIST_SET(first_node->prev, node);
    }

    return 0;
}

__static_inline int dlist_remove(dlist_t *list, dlist_node_t *node)
{
    if (node == NULL || list == NULL)
    {
        return -1;
    }
    dlist_node_t *first_node = DLIST_GET(list->head);
    dlist_node_t *node_next = DLIST_GET(node->next);
    if (node_next == NULL || first_node == NULL)
    {
        return -1;
    }

    if (node_next == node) // 是最后一节点
    {
        DLIST_SET(list->head, 0);
    }
    else // 不是最后一节点
    {
        dlist_node_t *node_prev = DLIST_GET(node->prev);
        if (first_node == node)
        {
            DLIST_SET(list->head, node_next); // 链头指向下一节点
        }
        DLIST_SET(node_prev->next, node_next);
        DLIST_SET(node_next->prev, node_prev);
    }

    DLIST_SET(node->next, 0);
    DLIST_SET(node->prev, 0);

    return 0;
}

__static_inline int dlist_insert_next(dlist_t *list, dlist_node_t *tar_node, dlist_node_t *new_node)
{
    dlist_node_t *tar_node_next = DLIST_GET(tar_node->next);
    if (DLIST_GET(list->head) == NULL || tar_node_next == NULL || DLIST_GET(new_node->next) != NULL)
    {
        return -1;
    }

    DLIST_SET(new_node->next, tar_node_next);
    DLIST_SET(new_node->prev, tar_node);
    DLIST_SET(tar_node_next->prev, new_node);
    DLIST_SET(tar_node->next, new_node);

    return 0;
}

__static_inline int dlist_insert_prev(dlist_t *list, dlist_node_t *tar_node, dlist_node_t *new_node)
{
    dlist_node_t *list_head = DLIST_GET(list->head);
    if (list_head == NULL || DLIST_GET(tar_node->next) == NULL || DLIST_GET(new_node->next) != NULL)
    {
        return -1;
    }

    DLIST_SET(new_node->next, tar_node);
    DLIST_SET(new_node->prev, DLIST_GET(tar_node->prev));
    DLIST_SET(DLIST_GET(tar_node->prev)->next, new_node);
    DLIST_SET(tar_node->prev, new_node);
    if (tar_node == list_head) // tar_node 是首个节点
    {
        DLIST_SET(list->head, new_node);
    }

    return 0;
}

__static_inline void dlist_set_next(dlist_t *list)
{
    if (list != NULL)
    {
        dlist_node_t *node = DLIST_GET(list->head);
        if (node != NULL)
        {
            DLIST_SET(list->head, DLIST_GET(node->next)); // 链头指向下一节点
        }
    }
}

__static_inline dlist_node_t *dlist_take_head(dlist_t *list)
{
    dlist_node_t *ret = DLIST_GET(list->head);
    dlist_remove(list, ret);
    return ret;
}

__static_inline dlist_node_t *dlist_take_tail(dlist_t *list)
{
    dlist_node_t *ret = dlist_peek_tail(list);
    dlist_remove(list, ret);
    return ret;
}

__static_inline dlist_node_t *dlist_peek_head(dlist_t *list)
{
    return DLIST_GET(list->head);
}

__static_inline dlist_node_t *dlist_peek_tail(dlist_t *list)
{
    dlist_node_t *list_head = DLIST_GET(list->head);
    if (list_head == NULL)
    {
        return NULL;
    }
    else
    {
        return DLIST_GET(list_head->prev);
    }
}

__static_inline dlist_node_t *dlist_peek_next(dlist_t *list, dlist_node_t *node)
{
    dlist_node_t *ret = DLIST_GET(node->next);
    if (dlist_peek_head(list) == ret)
    {
        ret = NULL;
    }
    return ret;
}

__static_inline dlist_node_t *dlist_peek_prev(dlist_t *list, dlist_node_t *node)
{
    dlist_node_t *ret = DLIST_GET(node->prev);
    if (dlist_peek_tail(list) == ret)
    {
        ret = NULL;
    }
    return ret;
}

__static_inline dlist_node_t *dlist_peek_next_loop(dlist_t *list, dlist_node_t *node)
{
    (void)list;
    return DLIST_GET(node->next);
}

__static_inline dlist_node_t *dlist_peek_prev_loop(dlist_t *list, dlist_node_t *node)
{
    (void)list;
    return DLIST_GET(node->prev);
}

__static_inline int dlist_is_pending(dlist_node_t *node)
{
    return (DLIST_GET(node->next) != NULL);
}

#ifdef __cplusplus
}
#endif

#endif
