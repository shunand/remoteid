/**
 * @file pslist.h
 * @author LokLiang (lokliang@163.com)
 * @brief
 * @version 0.1
 * @date 2023-05-01
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef __PSLIST_H__
#define __PSLIST_H__

#include "sys_types.h"

typedef struct _psnode sys_psnode_t;

typedef struct _pslist sys_pslist_t;

struct _psnode
{
    sys_psnode_t *const *next;
};

struct _pslist
{
    sys_psnode_t *const *head;
    sys_psnode_t *const *tail;
};

#define _CONTAINER_OF(PTR, TYPE, MEMBER) ((TYPE *)&((uint8_t *)PTR)[-(int)&((TYPE *)0)->MEMBER])

#define SYS_PSLIST_FOR_EACH_NODE(__sl, __sn)      \
    for (__sn = sys_pslist_peek_head(__sl); __sn; \
         __sn = sys_pslist_peek_next(__sn))

#define SYS_PSLIST_ITERATE_FROM_NODE(__sl, __sn)           \
    for (__sn = __sn ? sys_pslist_peek_next_no_check(__sn) \
                     : sys_pslist_peek_head(__sl);         \
         __sn;                                             \
         __sn = sys_pslist_peek_next(__sn))

#define SYS_PSLIST_FOR_EACH_NODE_SAFE(__sl, __sn, __sns) \
    for (__sn = sys_pslist_peek_head(__sl),              \
        __sns = sys_pslist_peek_next(__sn);              \
         __sn;                                           \
         __sn = __sns, __sns = sys_pslist_peek_next(__sn))

#define SYS_PSLIST_CONTAINER(__ln, __cn, __n) \
    ((__ln) ? _CONTAINER_OF((__ln), __typeof__(*(__cn)), __n) : NULL)

#define SYS_PSLIST_PEEK_HEAD_CONTAINER(__sl, __cn, __n) \
    SYS_PSLIST_CONTAINER(sys_pslist_peek_head(__sl), __cn, __n)

#define SYS_PSLIST_PEEK_TAIL_CONTAINER(__sl, __cn, __n) \
    SYS_PSLIST_CONTAINER(sys_pslist_peek_tail(__sl), __cn, __n)

#define SYS_PSLIST_PEEK_NEXT_CONTAINER(__cn, __n)                        \
    ((__cn) ? SYS_PSLIST_CONTAINER(sys_pslist_peek_next(&((__cn)->__n)), \
                                   __cn, __n)                            \
            : NULL)

#define SYS_PSLIST_FOR_EACH_CONTAINER(__sl, __cn, __n)           \
    for (__cn = SYS_PSLIST_PEEK_HEAD_CONTAINER(__sl, __cn, __n); \
         __cn;                                                   \
         __cn = SYS_PSLIST_PEEK_NEXT_CONTAINER(__cn, __n))

#define SYS_PSLIST_FOR_EACH_CONTAINER_SAFE(__sl, __cn, __cns, __n) \
    for (__cn = SYS_PSLIST_PEEK_HEAD_CONTAINER(__sl, __cn, __n),   \
        __cns = SYS_PSLIST_PEEK_NEXT_CONTAINER(__cn, __n);         \
         __cn;                                                     \
         __cn = __cns, __cns = SYS_PSLIST_PEEK_NEXT_CONTAINER(__cn, __n))

__static_inline void sys_pslist_init(sys_pslist_t *list)
{
    list->head = NULL;
    list->tail = NULL;
}

#define SYS_PSLIST_STATIC_INIT(ptr_to_list) \
    {                                       \
        NULL, NULL                          \
    }

__static_inline bool sys_pslist_is_empty(sys_pslist_t *list)
{
    return (!list->head);
}

__static_inline sys_psnode_t *const *sys_pslist_peek_head(sys_pslist_t *list)
{
    return list->head;
}

__static_inline sys_psnode_t *const *sys_pslist_peek_tail(sys_pslist_t *list)
{
    return list->tail;
}

__static_inline sys_psnode_t *const *sys_pslist_peek_next_no_check(sys_psnode_t *const *node)
{
    return (*node)->next;
}

__static_inline sys_psnode_t *const *sys_pslist_peek_next(sys_psnode_t *const *node)
{
    return node ? sys_pslist_peek_next_no_check(node) : NULL;
}

__static_inline void sys_pslist_prepend(sys_pslist_t *list,
                                        sys_psnode_t *const *node)
{
    (*node)->next = list->head;
    list->head = node;

    if (!list->tail)
    {
        list->tail = list->head;
    }
}

__static_inline void sys_pslist_append(sys_pslist_t *list,
                                       sys_psnode_t *const *node)
{
    (*node)->next = NULL;

    if (!list->tail)
    {
        list->tail = node;
        list->head = node;
    }
    else
    {
        (*(list->tail))->next = node;
        list->tail = node;
    }
}

__static_inline void sys_pslist_append_list(sys_pslist_t *list,
                                            sys_psnode_t *const *head, sys_psnode_t *const *tail)
{
    if (!list->tail)
    {
        list->head = head;
        list->tail = tail;
    }
    else
    {
        (*(list->tail))->next = head;
        list->tail = tail;
    }
}

__static_inline void sys_pslist_merge_pslist(sys_pslist_t *list,
                                             sys_pslist_t *list_to_append)
{
    sys_pslist_append_list(list, list_to_append->head,
                           list_to_append->tail);
    sys_pslist_init(list_to_append);
}

__static_inline void sys_pslist_insert(sys_pslist_t *list,
                                       sys_psnode_t *const *prev,
                                       sys_psnode_t *const *node)
{
    if (!prev)
    {
        sys_pslist_prepend(list, node);
    }
    else if (!(*prev)->next)
    {
        sys_pslist_append(list, node);
    }
    else
    {
        (*node)->next = (*prev)->next;
        (*prev)->next = node;
    }
}

__static_inline sys_psnode_t *const *sys_pslist_get_not_empty(sys_pslist_t *list)
{
    sys_psnode_t *const *node = list->head;

    list->head = (*node)->next;
    if (list->tail == node)
    {
        list->tail = list->head;
    }

    return node;
}

__static_inline sys_psnode_t *const *sys_pslist_get(sys_pslist_t *list)
{
    return sys_pslist_is_empty(list) ? NULL : sys_pslist_get_not_empty(list);
}

__static_inline void sys_pslist_remove(sys_pslist_t *list,
                                       sys_psnode_t *const *prev_node,
                                       sys_psnode_t *const *node)
{
    if (!prev_node)
    {
        list->head = (*node)->next;

        /* Was node also the tail? */
        if (list->tail == node)
        {
            list->tail = list->head;
        }
    }
    else
    {
        (*prev_node)->next = (*node)->next;

        /* Was node the tail? */
        if (list->tail == node)
        {
            list->tail = prev_node;
        }
    }

    (*node)->next = NULL;
}

__static_inline bool sys_pslist_find_and_remove(sys_pslist_t *list,
                                                sys_psnode_t *const *node)
{
    sys_psnode_t *const *prev = NULL;
    sys_psnode_t *const *test;

    SYS_PSLIST_FOR_EACH_NODE(list, test)
    {
        if (test == node)
        {
            sys_pslist_remove(list, prev, node);
            return true;
        }

        prev = test;
    }

    return false;
}

#endif
