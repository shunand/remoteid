#ifndef __MAIN_H__
#define __MAIN_H__

#include "kk.h"
#include "os/os.h"

void main_k_init(void);
void main_timer_reset(void);
void main_timer_hook(void (*fn)(void));

#endif
