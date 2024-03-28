#ifndef __FMC_H__
#define __FMC_H__

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

void drv_fmc_enable(void);
void drv_fmc_disable(void);

void drv_fmc_lock(void);
void drv_fmc_unlock(void);

unsigned drv_fmc_get_rom_base(void);
unsigned drv_fmc_get_rom_size(void);
unsigned drv_fmc_get_sector_base(unsigned addr);
unsigned drv_fmc_get_sector_size(unsigned addr);

int drv_fmc_sector_erase(unsigned addr);
int drv_fmc_data_write(unsigned addr, const void *src, unsigned size);
int drv_fmc_data_read(void *dest, unsigned addr, unsigned size);

bool drv_fmc_is_busy(void);

int drv_fmc_set_read_protection(bool enable);
int drv_fmc_set_write_protection(bool enable, unsigned start_offset, unsigned size);

bool drv_fmc_is_read_protection(void);
bool drv_fmc_is_write_protection(unsigned addr);

int drv_fmc_set_wdt(unsigned time_ms);

#ifdef __cplusplus
}
#endif

#endif
