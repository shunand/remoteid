/**
 * @file soc_shell_nvs.c
 * @author LokLiang
 * @brief 仅被 soc_shell.c 所包含，完成子命令列表及功能
 * @version 0.1
 * @date 2023-09-15
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef __SOC_SHELL_NVS_C__
#define __SOC_SHELL_NVS_C__

#include "shell/sh.h"
#include "shell/sh_vset.h"
#include "nvs_flash.h"
#include "os/os.h"

static char s_nvs_str_buf[0x40];

static int _value_set_str(const char *argv[]) { SET_VAR(&s_nvs_str_buf, 0, 0); }

SH_CMD_FN(_nvs_dump);
SH_CMD_FN(_nvs_erase);

SH_DEF_SUB_CMD(
    sub_soc_nvs,
    SH_SETUP_CMD("dump", "Dump blob value for given key <namespace> <key>", _nvs_dump, NULL), //
    SH_SETUP_CMD("erase", "Erase the default NVS partition", _nvs_erase, NULL),               //
);

SH_CMD_FN(_nvs_dump)
{
    const char *argv_r[2] = {"", ""};
    size_t required_size;
    char namespace_buf[sizeof(s_nvs_str_buf)];
    char key_buf[sizeof(s_nvs_str_buf)];
    int err = 0;

    argv_r[0] = argv[0];
    err = err ? err : _value_set_str(argv_r);
    if (err == 0)
        strcpy(namespace_buf, s_nvs_str_buf);

    argv_r[0] = argv[1];
    err = err ? err : _value_set_str(argv_r);
    if (err == 0)
        strcpy(key_buf, s_nvs_str_buf);

    if (err == 0)
    {
        nvs_handle nvs_hdl;
        if (nvs_open(namespace_buf, NVS_READONLY, &nvs_hdl) == ESP_OK)
        {
            err = nvs_get_blob(nvs_hdl, key_buf, NULL, &required_size);
            if (err == 0)
            {
                if (required_size)
                {
                    void *data = os_malloc(required_size);
                    if (data)
                    {
                        err = nvs_get_blob(nvs_hdl, key_buf, data, &required_size);
                        if (err == 0)
                        {
                            SYS_LOG_DUMP(data, required_size, 0, 0);
                        }
                        else
                        {
                            sh_echo(sh_hdl, "Can not get blob key: '%s'\r\n", key_buf);
                        }
                        os_free(data);
                    }
                    else
                    {
                        sh_echo(sh_hdl, "os_malloc() fail\r\n");
                    }
                }
                else
                {
                    sh_echo(sh_hdl, "Blob data empty\r\n");
                }
            }
            else
            {
                sh_echo(sh_hdl, "Can not get blob key: '%s'\r\n", key_buf);
            }
            nvs_close(nvs_hdl);
        }
        else
        {
            sh_echo(sh_hdl, "Can not open name space: '%s'\r\n", namespace_buf);
        }
    }

    return err;
}

SH_CMD_FN(_nvs_erase)
{
    sh_echo(sh_hdl, "正在擦除 NVS 分区 ...\r\n");
    esp_err_t err = nvs_flash_erase();
    if (err == ESP_OK)
    {
        sh_echo(sh_hdl, "操作成功，重启系统 ...\r\n");
        esp_restart();
        return 0;
    }
    else
    {
        sh_echo(sh_hdl, "操作异常: err = %d\r\n", err);
        return -1;
    }
}

#endif
