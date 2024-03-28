/**
 * @file soc_shell_base.c
 * @author LokLiang
 * @brief 仅被 soc_shell.c 所包含，完成子命令列表及功能
 * @version 0.1
 * @date 2023-09-15
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef __SOC_SHELL_BASE_C__
#define __SOC_SHELL_BASE_C__

#include "shell/sh.h"
#include "esp_heap_caps.h"
#include "spi_flash_mmap.h"
#include "esp_flash.h"
#include "esp_chip_info.h"
#include "task.h"

/* Unsigned integers.  */
#define PRIu8 "u"
#define PRIu16 "u"
#define PRIu32 "u"
#define PRIu64 __PRI64_PREFIX "u"

SH_CMD_FN(_base_free);
SH_CMD_FN(_base_heap);
SH_CMD_FN(_base_version);
SH_CMD_FN(_base_tasks);
SH_CMD_FN(_base_restart);

SH_DEF_SUB_CMD(
    sub_soc_base,
#ifdef CONFIG_FREERTOS_USE_STATS_FORMATTING_FUNCTIONS
    SH_SETUP_CMD("tasks", "Get information about running tasks", _base_tasks, NULL), //
#endif
    SH_SETUP_CMD("free", "Get the current size of free heap memory", _base_free, NULL),                                         //
    SH_SETUP_CMD("heap", "Get minimum size of free heap memory that was available during program execution", _base_heap, NULL), //
    SH_SETUP_CMD("version", "Get version of chip and SDK", _base_version, NULL),                                                //
    SH_SETUP_CMD("restart", "Software reset of the chip", _base_restart, NULL),                                                 //
);

SH_CMD_FN(_base_free)
{
    sh_echo(sh_hdl, "当前堆内存空闲大小: %d bytes\r\n", esp_get_free_heap_size());
    return 0;
}

SH_CMD_FN(_base_heap)
{
    sh_echo(sh_hdl, "当前堆内存最少剩余: %d bytes\r\n", heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT));
    return 0;
}

SH_CMD_FN(_base_version)
{
    const char *model;
    esp_chip_info_t info;
    uint32_t flash_size;
    esp_chip_info(&info);

    switch (info.model)
    {
    case CHIP_ESP32:
        model = "ESP32";
        break;
    case CHIP_ESP32S2:
        model = "ESP32-S2";
        break;
    case CHIP_ESP32S3:
        model = "ESP32-S3";
        break;
    case CHIP_ESP32C3:
        model = "ESP32-C3";
        break;
    case CHIP_ESP32H2:
        model = "ESP32-H2";
        break;
    case CHIP_ESP32C2:
        model = "ESP32-C2";
        break;
    default:
        model = "Unknown";
        break;
    }

    if (esp_flash_get_size(NULL, &flash_size) != ESP_OK)
    {
        sh_echo(sh_hdl, "Get flash size failed");
        return 1;
    }
    sh_echo(sh_hdl, "IDF Version:%s\r\n", esp_get_idf_version());
    sh_echo(sh_hdl, "Chip info:\r\n");
    sh_echo(sh_hdl, "\tmodel:%s\r\n", model);
    sh_echo(sh_hdl, "\tcores:%d\r\n", info.cores);
    sh_echo(sh_hdl, "\tfeature:%s%s%s%s%" PRIu32 "%s\r\n",
            info.features & CHIP_FEATURE_WIFI_BGN ? "/802.11bgn" : "",
            info.features & CHIP_FEATURE_BLE ? "/BLE" : "",
            info.features & CHIP_FEATURE_BT ? "/BT" : "",
            info.features & CHIP_FEATURE_EMB_FLASH ? "/Embedded-Flash:" : "/External-Flash:",
            flash_size / (1024 * 1024), " MB");
    sh_echo(sh_hdl, "\trevision number:%d\r\n", info.revision);
    return 0;
}

SH_CMD_FN(_base_tasks)
{
#ifdef CONFIG_FREERTOS_USE_STATS_FORMATTING_FUNCTIONS
    const size_t bytes_per_task = 40; /* see vTaskList description */
    char *task_list_buffer = malloc(uxTaskGetNumberOfTasks() * bytes_per_task);
    if (task_list_buffer == NULL)
    {
        sh_echo(sh_hdl, "failed to allocate buffer for vTaskList output\r\n");
        return -1;
    }
    sh_echo(sh_hdl, "Task Name\tStatus\tPrio\tHWM\tTask#", stdout);
#ifdef CONFIG_FREERTOS_VTASKLIST_INCLUDE_COREID
    sh_echo(sh_hdl, "\tAffinity", stdout);
#endif
    sh_echo(sh_hdl, "\r\n", stdout);
    vTaskList(task_list_buffer);
    sh_echo(sh_hdl, task_list_buffer, stdout);

    sh_echo(sh_hdl, "Task_Name\tRun_Cnt\t\tUsage_Rate\r\n", stdout);
    vTaskGetRunTimeStats(task_list_buffer);
    sh_echo(sh_hdl, task_list_buffer, stdout);

    free(task_list_buffer);
#endif
    return 0;
}

SH_CMD_FN(_base_restart)
{
    esp_restart();
    return 0;
}

#endif
