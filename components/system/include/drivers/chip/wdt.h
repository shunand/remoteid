#ifndef __WDT_H__
#define __WDT_H__

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum
{
	_WDT_MODE_RESET,
	_WDT_MODE_INTERRUPT_RESET
} wdt_mode_t;

void wdt_enable(void);
void wdt_disable(void);

void wdt_irq_enable(int priority);
void wdt_irq_disable(void);

void wdt_set_config(wdt_mode_t mode, int int_ms, int reset_ms);

void wdt_reload(void);
unsigned wdt_get_time_cost(void);

void wdt_clear_int_flag(void);

#ifdef __cplusplus
}
#endif

#endif
