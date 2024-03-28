/**
 * @file os_heap.h
 * @author LokLiang (lokliang@163.com)
 * @brief
 * @version 0.1
 * @date 2023-05-01
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef __OS_HEAP_H__
#define __OS_HEAP_H__

#include "os/os_common.h"

#ifdef __cplusplus
extern "C"
{
#endif

void *os_malloc(size_t size);
void *os_calloc(size_t size);
void *os_realloc(void *p, size_t size);
void  os_free(void *p);

void os_heap_info(size_t *used_size, size_t *free_size, size_t *max_block_size);

#ifdef __cplusplus
}
#endif

#endif /* __OS_HEAP_H__ */
