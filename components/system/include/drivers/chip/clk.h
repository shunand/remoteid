#ifndef __CLK_H__
#define __CLK_H__

#ifdef __cplusplus
extern "C"
{
#endif

unsigned drv_clk_get_cpu_clk(void);

int drv_clk_calibration(unsigned tar_clk, unsigned cur_clk);

#ifdef __cplusplus
}
#endif

#endif
