#pragma once

#include "nvs.h"
#include "os/os.h"

extern nvs_handle g_nvs_hdl;                // nvs 句柄
extern os_work_q_t g_work_q_hdl_low;        // 低于 default_os_work_q_hdl 优先级的工作队列
