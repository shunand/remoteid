#if (CONFIG_BOARD_NAME_DEVKIT_ESP32C2)

#define CONFIG_IDF_TARGET "esp32c2" /* 警告：请使用命令 idf.py set-target <参数> 选择对应的平台 */

static cfg_board_t const s_cfg_board_default = {
    .firmware_str = PRODUCT_TYPE,
    .platform_str = CONFIG_IDF_TARGET,
    .board_name = "devkit_esp32c2",

    /* 控制台串口 */
    .uart_console = {
        .pin_txd = {43, _GPIO_DIR_OUT, _GPIO_PUD_PULL_UP},
        .pin_rxd = {44, _GPIO_DIR_IN, _GPIO_PUD_PULL_UP},
        .id = UART_NUM_0,
        .irq_prior = 0,
        .br = 115200,
    },

    /* 数据透传串口 */
    .uart_fc = {
        .pin_txd = {7, _GPIO_DIR_OUT, _GPIO_PUD_PULL_UP},
        .pin_rxd = {6, _GPIO_DIR_IN, _GPIO_PUD_PULL_UP},
        .id = UART_NUM_1,
        .irq_prior = 24,
        .br = 115200,
    },

    .led_spi = {
        .spi_id = SPI2_HOST, // 模拟 PWM 用的 SPI
        .pin = 1,            // 模拟 PWM 输出引脚
    },

    /* 启动按键 */
    .key_boot = {
        .pin = 9,    // 用于切换灯效
        .en_lev = 0, // 用于切换灯效
    },
};

#endif
