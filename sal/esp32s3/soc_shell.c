#include "shell/sh.h"

#include "esp_system.h"
#include "freertos/FreeRTOS.h"

#define CONFIG_SYS_LOG_DUMP_ON 1
#define CONFIG_SYS_LOG_LEVEL SYS_LOG_LEVEL_INF
#define SYS_LOG_DOMAIN "SOC"
#include "sys_log.h"

#include "soc_shell_base.c"
#include "soc_shell_nvs.c"

SH_DEF_SUB_CMD(
    sub_soc,
    SH_SETUP_CMD("base", "Basic information of the system", NULL, sub_soc_base), //
    SH_SETUP_CMD("nvs", "Partition for operating nvs", NULL, sub_soc_nvs),       //
);

SH_DEF_CMD(
    _register_cmd_soc,
    SH_SETUP_CMD("soc", "System on chip", NULL, sub_soc), );

void soc_shell_register(void)
{
    sh_register_cmd(&_register_cmd_soc);
}
