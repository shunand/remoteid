#ifndef __DMA_H__
#define __DMA_H__

#include "drivers/chip/_hal.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum __packed
{
    _DMA_DIR_MEM_TO_MEM,
    _DMA_DIR_PER_TO_MEM,
    _DMA_DIR_MEM_TO_PER,
} dma_dir_t;

typedef void (*dma_isr_cb_fn)(void);

void drv_dma_enable(hal_id_t id);
void drv_dma_disable(hal_id_t id);

int drv_dma_init(hal_id_t id, unsigned channel, unsigned prio_level);
int drv_dma_deinit(hal_id_t id, unsigned channel);

int drv_dma_irq_callback_enable(hal_id_t id, unsigned channel, dma_isr_cb_fn cb);
int drv_dma_irq_callback_disable(hal_id_t id, unsigned channel);

void drv_dma_irq_enable(hal_id_t id, unsigned channel, int priority);
void drv_dma_irq_disable(hal_id_t id, unsigned channel);

void drv_dma_set(hal_id_t id, unsigned channel, void *dest, unsigned dest_bit_width, bool dest_inc_mode, const void *src, unsigned src_bit_width, bool src_inc_mode, unsigned count, dma_dir_t dir);
void drv_dma_start(hal_id_t id, unsigned channel);
void drv_dma_stop(hal_id_t id, unsigned channel);

unsigned drv_dma_get_remain(hal_id_t id, unsigned channel);

#ifdef __cplusplus
}
#endif

#endif
