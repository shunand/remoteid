#ifndef __CONSOLE_H__
#define __CONSOLE_H__

#include <stdbool.h>
#include "shell/sh.h"

typedef void (*shell_input_cb)(sh_t *sh_hdl, char c);

void shell_start();

void shell_input_set(shell_input_cb cb);
void shell_input_restore(void);

bool shell_is_aobrt(void);

#endif
