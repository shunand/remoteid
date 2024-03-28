#ifndef __TICK_H__
#define __TICK_H__

#ifdef __cplusplus
extern "C"
{
#endif

void drv_tick_enable(unsigned freq);
void drv_tick_disable(void);

unsigned drv_tick_get_counter(void);

#ifdef __cplusplus
}
#endif

#endif
