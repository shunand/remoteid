#include "os/os.h"
#include "os_util.h"
#include "portable.h"
#include <string.h>

void *os_malloc(size_t size)
{
    return pvPortMalloc(size);
}

void *os_calloc(size_t size)
{
    void *ret = os_malloc(size);
    if (ret)
    {
        memset(ret, 0, size);
    }
    return ret;
}

void *os_realloc(void *p, size_t size)
{
#if (configSTACK_ALLOCATION_FROM_SEPARATE_HEAP == 1)
    return realloc(p, size);
#else
    OS_DBG("%s() fail @ %d function not support!\r\n", __func__, __LINE__);
    return NULL;
#endif
}

void os_free(void *p)
{
    vPortFree(p);
}

void os_heap_info(size_t *used_size, size_t *free_size, size_t *max_block_size)
{
    if (max_block_size)
    {
        *max_block_size = 0;
    }
    if (used_size)
        *used_size = 0;
    if (free_size)
        *free_size = xPortGetFreeHeapSize();
}
