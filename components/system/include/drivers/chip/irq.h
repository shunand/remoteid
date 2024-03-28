#ifndef __IRQ_H__
#define __IRQ_H__

#ifdef __cplusplus
extern "C"
{
#endif

unsigned drv_irq_disable(void);
void drv_irq_enable(unsigned nest);

unsigned drv_irq_get_nest(void);

void drv_fiq_disable(void);
void drv_fiq_enable(void);

void drv_irq_hardfault_callback(void ((*cb)(void)));

#ifdef __cplusplus
}
#endif

#endif
