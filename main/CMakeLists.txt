
set(srcs-app
)

# 基础程序
set(srcs-common
    "ota.c"
    "../app/app_main.c"
    "../app/console.c"
    "../app/drivers/data_port/sb_data_port.c"
    "../app/drivers/data_port/uart/uart_port.c"
    "../app/button/button_event.c"
)

# 自定义框架抽象层
set(srcs-components
    "../components/system/source/k_kit/k_kit.c"
    "../components/system/source/shell/sh_vt100.c"
    "../components/system/source/shell/sh_vset.c"
    "../components/system/source/shell/sh.c"
    "../sal/esp32s3/kernel/os_heap.c"
    "../sal/esp32s3/kernel/os_mutex.c"
    "../sal/esp32s3/kernel/os_hook.c"
    "../sal/esp32s3/kernel/os_timer.c"
    "../sal/esp32s3/kernel/os_semaphore.c"
    "../sal/esp32s3/kernel/os_thread.c"
    "../sal/esp32s3/kernel/os_kit.c"
    "../sal/esp32s3/kernel/os_service.c"
    "../sal/esp32s3/chip/uart_esp32.c"
    "../sal/esp32s3/soc_shell.c"
    "../app/config/board_config.c"
)
set(incs
    "../app"
    "../components/system/include"
    "../components/system/source"
    "../components/system/source/k_kit"
    "../components/system/source/shell"
    "../sal/esp32s3"
    "../sal/esp32s3/kernel"
    "${IDF_PATH}/components/freertos/FreeRTOS-Kernel/include/freertos"
)

set(requires
       arduino-esp32-3.0.7
      DroneCAN_generated
      libcanard
    libopendroneid
   mavlink2
   esp_hw_support
   esp_partition
   nvs_flash 
   app_update   
   efuse
   )


idf_component_register(SRCS "main.c"
    ${srcs-components}
    ${srcs-common}
    INCLUDE_DIRS "." ${incs}
    REQUIRES ${requires})
