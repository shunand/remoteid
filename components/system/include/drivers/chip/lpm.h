#ifndef __LPM_H__
#define __LPM_H__

#ifdef __cplusplus
extern "C"
{
#endif

void drv_lpm_wkup_enable(unsigned id);
void drv_lpm_wkup_disable(unsigned id);

void drv_lpm_sleep(void);
void drv_lpm_deep_sleep(void);

#ifdef __cplusplus
}
#endif

#endif
