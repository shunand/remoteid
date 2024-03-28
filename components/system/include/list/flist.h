/**
 * @file flist.h
 * @author LokLiang (lokliang@163.com)
 * @brief
 * @version 0.1
 * @date 2023-05-01
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef __FULLLIST_H__
#define __FULLLIST_H__

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
    list_node_t list;
} flist_node_t;
typedef struct
{
    list_node_t head;
} flist_t;

__static_inline void flist_init_list (flist_t *list);
__static_inline void flist_init_node (flist_node_t *node);

__static_inline int  flist_insert_font (flist_t *list, flist_node_t *node);
__static_inline int  flist_insert_tail (flist_t *list, flist_node_t *node);
__static_inline int  flist_remove      (flist_t *list, flist_node_t *node);

__static_inline int  flist_insert_next (flist_node_t *tar_node, flist_node_t *new_node);
__static_inline int  flist_insert_prev (flist_node_t *tar_node, flist_node_t *new_node);
__static_inline int  flist_node_free   (flist_node_t *node);

__static_inline void flist_set_next (flist_t *list);

__static_inline flist_node_t *flist_take_head (flist_t *list);
__static_inline flist_node_t *flist_take_tail (flist_t *list);

__static_inline flist_node_t *flist_peek_head (flist_t *list);
__static_inline flist_node_t *flist_peek_tail (flist_t *list);

__static_inline flist_t      *flist_peek_list  (flist_node_t *node);
__static_inline flist_node_t *flist_peek_first (flist_node_t *node);
__static_inline flist_node_t *flist_peek_next  (flist_node_t *node);
__static_inline flist_node_t *flist_peek_prev  (flist_node_t *node);

__static_inline flist_node_t *flist_peek_next_loop (flist_node_t *node);
__static_inline flist_node_t *flist_peek_prev_loop (flist_node_t *node);

__static_inline int flist_is_pending (flist_node_t *node);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if LIST_NODE_TYPE == 1
#define FLIST_SET(Value1, Value2)       (Value1 = Value2 == 0 ? 0 : ((int)Value2 - (int)&Value1) | 1)
#define FLIST_GET(Value)                ((flist_node_t*)(Value == 0 ? 0 : ((int)&Value + Value) & ~1))
#else
#define FLIST_SET(Value1, Value2)       (Value1 = (list_node_t)Value2)
#define FLIST_GET(Value)                ((flist_node_t*)(Value))
#endif

__static_inline void flist_init_list(flist_t *list)
{
    FLIST_SET(list->head, 0);
}

__static_inline void flist_init_node(flist_node_t *node)
{
    FLIST_SET(node->list, 0);
    FLIST_SET(node->next, 0);
    FLIST_SET(node->prev, 0);
}

__static_inline int flist_insert_font(flist_t *list, flist_node_t *node)
{
    flist_insert_tail(list, node);
    FLIST_SET(list->head, node);
    return 0;
}

__static_inline int flist_insert_tail(flist_t *list, flist_node_t *node)
{
    if (FLIST_GET(node->list) != NULL)
    {
        flist_node_free(node);
    }

    FLIST_SET(node->list, list);

    flist_node_t *first_node = FLIST_GET(list->head);
    if (first_node == NULL)
    {
        /* 直接设置链头 */
        FLIST_SET(node->next, node);
        FLIST_SET(node->prev, node);
        FLIST_SET(list->head, node);
    }
    else
    {
        flist_node_t *first_node_prev = FLIST_GET(first_node->prev);
        FLIST_SET(node->next, first_node);
        FLIST_SET(node->prev, first_node_prev);
        FLIST_SET(first_node_prev->next, node);
        FLIST_SET(first_node->prev, node);
    }

    return 0;
}

__static_inline int flist_remove(flist_t *list, flist_node_t *node)
{
    if (node == NULL || list == NULL)
    {
        return -1;
    }

    flist_node_t *first_node = FLIST_GET(list->head);
    flist_node_t *node_next = FLIST_GET(node->next);
    if (node_next == NULL || first_node == NULL)
    {
        return -1;
    }

    if (node_next == node) // 是最后一节点
    {
        FLIST_SET(list->head, 0);
    }
    else // 不是最后一节点
    {
        if (first_node == node)
        {
            FLIST_SET(list->head, node_next); // 链头指向下一节点
        }
        flist_node_t *node_prev = FLIST_GET(node->prev);
        FLIST_SET(node_prev->next, node_next);
        FLIST_SET(node_next->prev, node_prev);
    }

    FLIST_SET(node->list, 0);
    FLIST_SET(node->next, 0);
    FLIST_SET(node->prev, 0);

    return 0;
}

__static_inline int flist_insert_next(flist_node_t *tar_node, flist_node_t *new_node)
{
    flist_node_t *tar_node_list = FLIST_GET(tar_node->list);
    if (tar_node_list == NULL)
    {
        return -1;
    }

    if (tar_node == new_node)
    {
        flist_node_t *src_node_next = FLIST_GET(new_node->next);
        if (src_node_next != new_node) // 不是最后一节点
        {
            flist_t *list = (flist_t *)FLIST_GET(new_node->list);
            flist_node_t *first_node = FLIST_GET(list->head);
            if (first_node == new_node)
            {
                FLIST_SET(list->head, src_node_next); // 链头指向下一节点
            }
        }
    }
    else
    {
        if (FLIST_GET(new_node->list) != NULL)
        {
            flist_node_free(new_node);
        }
        flist_node_t *tar_node_next = FLIST_GET(tar_node->next);
        FLIST_SET(new_node->list, tar_node_list);
        FLIST_SET(new_node->next, tar_node_next);
        FLIST_SET(new_node->prev, tar_node);
        FLIST_SET(tar_node_next->prev, new_node);
        FLIST_SET(tar_node->next, new_node);
    }

    return 0;
}

__static_inline int flist_insert_prev(flist_node_t *tar_node, flist_node_t *new_node)
{
    flist_node_t *tar_node_list = FLIST_GET(tar_node->list);
    if (tar_node_list == NULL)
    {
        return -1;
    }

    if (tar_node == new_node)
    {
        flist_node_t *src_node_next = FLIST_GET(new_node->next);
        if (src_node_next != new_node) // 不是最后一节点
        {
            flist_t *list = (flist_t *)FLIST_GET(new_node->list);
            flist_node_t *first_node = (flist_node_t *)FLIST_GET(list->head);
            if (first_node == new_node)
            {
                FLIST_SET(list->head, src_node_next); // 链头指向下一节点
            }
        }
    }
    else
    {
        if (FLIST_GET(new_node->list) != NULL)
        {
            flist_node_free(new_node);
        }
        flist_node_t *tar_node_prev = FLIST_GET(tar_node->prev);
        FLIST_SET(new_node->list, tar_node_list);
        FLIST_SET(new_node->next, tar_node);
        FLIST_SET(new_node->prev, tar_node_prev);
        FLIST_SET(tar_node_prev->next, new_node);
        FLIST_SET(tar_node->prev, new_node);
        flist_t *list = (flist_t *)tar_node_list;
        if (tar_node == FLIST_GET(list->head)) // tar_node 是首个节点
        {
            FLIST_SET(list->head, new_node);
        }
    }

    return 0;
}

__static_inline int flist_node_free(flist_node_t *node)
{
    if (node == NULL)
    {
        return -1;
    }
    return flist_remove((flist_t *)FLIST_GET(node->list), node);
}

__static_inline void flist_set_next(flist_t *list)
{
    if (list != NULL)
    {
        flist_node_t *node = FLIST_GET(list->head);
        if (node != NULL)
        {
            FLIST_SET(list->head, FLIST_GET(node->next)); // 链头指向下一节点
        }
    }
}

__static_inline flist_node_t *flist_take_head(flist_t *list)
{
    flist_node_t *ret = FLIST_GET(list->head);
    flist_remove(list, ret);
    return ret;
}

__static_inline flist_node_t *flist_take_tail(flist_t *list)
{
    flist_node_t *ret = flist_peek_tail(list);
    flist_remove(list, ret);
    return ret;
}

__static_inline flist_node_t *flist_peek_head(flist_t *list)
{
    return FLIST_GET(list->head);
}

__static_inline flist_node_t *flist_peek_tail(flist_t *list)
{
    flist_node_t *list_head = FLIST_GET(list->head);
    if (list_head == NULL)
    {
        return NULL;
    }
    else
    {
        return FLIST_GET(list_head->prev);
    }
}

__static_inline flist_t *flist_peek_list(flist_node_t *node)
{
    return (flist_t *)FLIST_GET(node->list);
}

__static_inline flist_node_t *flist_peek_first(flist_node_t *node)
{
    flist_node_t *node_list = FLIST_GET(node->list);
    if (node_list == NULL)
    {
        return NULL;
    }
    else
    {
        return FLIST_GET(((flist_t *)node_list)->head);
    }
}

__static_inline flist_node_t *flist_peek_next(flist_node_t *node)
{
    flist_t *pFlist = (flist_t *)FLIST_GET(node->list);
    if (pFlist == NULL)
    {
        return NULL;
    }
    else
    {
        flist_node_t *ret = FLIST_GET(node->next);
        if (flist_peek_head(pFlist) == ret)
        {
            ret = NULL;
        }
        return ret;
    }
}

__static_inline flist_node_t *flist_peek_prev(flist_node_t *node)
{
    flist_t *pFlist = (flist_t *)FLIST_GET(node->list);
    if (pFlist == NULL)
    {
        return NULL;
    }
    else
    {
        flist_node_t *ret = FLIST_GET(node->prev);
        if (flist_peek_tail(pFlist) == ret)
        {
            ret = NULL;
        }
        return ret;
    }
}

__static_inline flist_node_t *flist_peek_next_loop(flist_node_t *node)
{
    return FLIST_GET(node->next);
}

__static_inline flist_node_t *flist_peek_prev_loop(flist_node_t *node)
{
    return FLIST_GET(node->prev);
}

__static_inline int flist_is_pending(flist_node_t *node)
{
    return (FLIST_GET(node->next) != NULL);
}

#ifdef __cplusplus
}
#endif

#endif
