#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include "driver/gpio.h"

#include "os/os.h"

#include "button_event.h"

#define CONFIG_SYS_LOG_LEVEL SYS_LOG_LEVEL_WRN
#define SYS_LOG_DOMAIN "BUTTON"
#include "sys_log.h"

#define VOLTAGE_HIGH 1 // 高电平有效
#define VOLTAGE_LOW 0  // 高电平有效

#define DEBOUNCE_CHECK_VOLTAGE_CHANGED 1
#define DEBOUNCE_CHECK_VOLTAGE_DID_NOT_CHANGED 0

#define DEBOUCE_CHECK_MAX_TIMES 5

typedef struct
{
    uint8_t pin;
    uint8_t is_inverted; // 是否反向，如果为真的话，则当按钮按下后，引脚为低电平
    uint16_t history;
    uint32_t down_time;
    uint32_t next_long_time;
    int current_voltage; // 当前引脚电平
    int debounce_check_counter;
    uint8_t allow_multiple_long_press_event;
    uint8_t long_press_event_sent;
} debounce_t;

int pin_count = -1;
debounce_t *debounce;
static os_queue_t s_event_queue; // 当出现按键变化时被传送到的队列对象
static button_callback_t *g_callback_list_head = NULL;
static button_callback_t *g_callback_list_tail = NULL;
static os_work_t s_work_hdl;

static os_queue_t *_pulled_button_init(unsigned long long pin_select, gpio_pull_mode_t pull_mode);

static uint32_t millis()
{
    return os_get_sys_time();
}

static int send_event(debounce_t *db, button_event_e ev)
{
    static char *const ev_tbl[] = {
        [BUTTON_UP] = "release",
        [BUTTON_DOWN] = "press",
        [BUTTON_HELD] = "held"};
    SYS_ASSERT(ev < sizeof(ev_tbl), "ev: %d", ev);
    SYS_LOG_DBG("event happend: pinnum: %d, ev: %s", db->pin, ev_tbl[ev]);

    button_event_t event = {
        .pin = db->pin,
        .event = ev,
    };

    os_queue_send(&s_event_queue, &event, 0);

    button_callback_t *tmp = g_callback_list_head;
    button_callback_t *callback_list = NULL;
    button_callback_t *callback_list_it = NULL;
    while (tmp != NULL)
    {
        if (tmp->pin == db->pin)
        {
            button_callback_t *new_callback = NULL;
            if (callback_list == NULL)
            {
                callback_list = os_malloc(sizeof(button_callback_t));
                callback_list_it = callback_list;
                new_callback = callback_list_it;
            }
            else
            {
                new_callback = os_malloc(sizeof(button_callback_t));
            }
            new_callback->pin = db->pin;
            new_callback->event_callback = tmp->event_callback;
            new_callback->next = NULL;

            if (new_callback != callback_list_it)
            {
                callback_list_it->next = new_callback;
                callback_list_it = new_callback;
            }
        }
        tmp = tmp->next;
    }

    tmp = callback_list;
    int stop_forward_event = 0;
    while (tmp != NULL)
    {
        if (tmp->pin == db->pin)
        {
            SYS_LOG_DBG("do call back: pinnum: %d, ev: %s", db->pin, ev_tbl[ev]);
            stop_forward_event = tmp->event_callback(&event);
            if (stop_forward_event)
            {
                break;
            }
        }
        tmp = tmp->next;
    }

    // 释放临时遍历对象
    tmp = callback_list;
    while (tmp)
    {
        button_callback_t *next_obj = tmp->next;

        os_free(tmp);
        tmp = next_obj;
    }

    return stop_forward_event;
}

static int update_debounce_counter(int pin_index)
{
    debounce_t *button_event_info = &debounce[pin_index];
    int level = gpio_get_level(button_event_info->pin);
    if (button_event_info->current_voltage != level)
    {
        button_event_info->debounce_check_counter += 1;
    }
    else if (button_event_info->current_voltage == level && button_event_info->debounce_check_counter > 0)
    {
        button_event_info->debounce_check_counter -= 1;
    }

    // 防抖检测已经连续多次检测到相同的电平，则改变按钮当前的电平
    if (button_event_info->debounce_check_counter == DEBOUCE_CHECK_MAX_TIMES)
    {
        button_event_info->current_voltage = level;
        button_event_info->debounce_check_counter = 0;
        SYS_LOG_DBG("button voltage changed, pin:%d, voltage:%d", button_event_info->pin, level);
        return DEBOUNCE_CHECK_VOLTAGE_CHANGED;
    }

    return DEBOUNCE_CHECK_VOLTAGE_DID_NOT_CHANGED;
}

static bool is_button_down(int pin_index)
{
    debounce_t *button_event_info = &debounce[pin_index];

    if (button_event_info->is_inverted && button_event_info->current_voltage == VOLTAGE_LOW)
    {
        // 如果电平是反向的，并且当前是低电平，则认为按钮按下了
        return true;
    }
    else if (!button_event_info->is_inverted && button_event_info->current_voltage == VOLTAGE_HIGH)
    {
        // 如果电平不是反向的，并且当前是高电平，则认为按钮按下了
        return true;
    }

    return false;
}

static bool is_button_up(int pin_index)
{
    debounce_t *button_event_info = &debounce[pin_index];

    if (button_event_info->is_inverted && button_event_info->current_voltage == VOLTAGE_HIGH)
    {
        // 如果电平是反向的，并且当前是高电平，则认为按钮释放了
        return true;
    }
    else if (!button_event_info->is_inverted && button_event_info->current_voltage == VOLTAGE_LOW)
    {
        // 如果电平不是反向的，并且当前是低电平，则认为按钮释放了
        return true;
    }

    return false;
}

static void _work_handler_button(void *pvParameter)
{
    static uint8_t cancel_laster_event = 1;

    for (int idx = 0; idx < pin_count; idx++)
    {
        debounce_t *button_event_info = &debounce[idx];

        int check_result = update_debounce_counter(idx); // 防抖处理
        if (check_result == DEBOUNCE_CHECK_VOLTAGE_DID_NOT_CHANGED)
        {
            // 如果allow_multiple_long_press_event为false，则一次长按无论长按多久，只会触发一次长按事件
            if (!button_event_info->allow_multiple_long_press_event && button_event_info->long_press_event_sent != 0)
            {
                continue;
            }

            // 电平没有变化，则检查按钮是否已经按下，以开始检查长按操作
            if (button_event_info->down_time && millis() >= button_event_info->next_long_time)
            {
                button_event_info->next_long_time = millis() + LONG_PRESS_REPEAT;
                button_event_info->long_press_event_sent = 1;
                cancel_laster_event = send_event(&debounce[idx], BUTTON_HELD);
                if (cancel_laster_event)
                {
                    button_event_info->down_time = 0;
                }
            }
        }
        else
        {
            // 电平有变化，检查是按下还是松开
            if (button_event_info->down_time == 0 && is_button_down(idx))
            {
                cancel_laster_event = 0;
                SYS_LOG_DBG("update downtime for pin:%d", button_event_info->pin);
                button_event_info->down_time = millis();
                button_event_info->next_long_time = button_event_info->down_time + LONG_PRESS_DURATION;
                cancel_laster_event = send_event(&debounce[idx], BUTTON_DOWN);
                if (cancel_laster_event)
                {
                    button_event_info->down_time = 0;
                }
            }
            else if (button_event_info->down_time && is_button_up(idx))
            {
                button_event_info->down_time = 0;
                button_event_info->long_press_event_sent = 0;
                cancel_laster_event = send_event(&debounce[idx], BUTTON_UP);
                if (cancel_laster_event)
                {
                    button_event_info->down_time = 0;
                }
            }
        }
    }

    os_work_later(10);
}

/**
 * @brief 初始化按电平式的按键检测。
 * 这将创建一个用于扫描按键的线程，当检测到按键变化时，将在线程中产生两种通知：
 * 1. 发送到内部创建的队列中（提示：如果使用了 os_queue_regist() 注册工作项，该工作项将被自动唤醒）
 * 2. 执行由 button_event_add_callback() 设置的回调函数（提示：同一引脚可设置多个回调）
 *
 * @param pin_select    按位表示 MCU 的引脚呈
 * @param en_lev        触发电平
 * @return os_queue_t* 由内部创建的消息队列
 */
os_queue_t *button_init(unsigned long long pin_select, uint8_t en_lev)
{
    return _pulled_button_init(pin_select, en_lev == 0 ? GPIO_PULLUP_ONLY : GPIO_PULLDOWN_ONLY);
}

static os_queue_t *_pulled_button_init(unsigned long long pin_select, gpio_pull_mode_t pull_mode)
{
    if (pin_count != -1)
    {
        SYS_LOG_WRN("Already initialized");
        return NULL;
    }

    // Configure the pins
    gpio_config_t io_conf;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = (pull_mode == GPIO_PULLUP_ONLY || pull_mode == GPIO_PULLUP_PULLDOWN);
    io_conf.pull_down_en = (pull_mode == GPIO_PULLDOWN_ONLY || pull_mode == GPIO_PULLUP_PULLDOWN);
    io_conf.pin_bit_mask = pin_select;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);

    // Scan the pin map to determine number of pins
    pin_count = 0;
    int pin = 0;
    for (pin = 0; pin < sizeof(pin_select) * 8; pin++)  //按位与检查结果
    {
        if ((1ULL << pin) & pin_select)
        {
            pin_count++;
        }
    }

    // Initialize global state and s_event_queue
    debounce = os_calloc(sizeof(debounce_t) * pin_count);
    // s_event_queue = xQueueCreate(4, sizeof(button_event_t));
    os_queue_create(&s_event_queue, 4, sizeof(button_event_t));

    // Scan the pin map to determine each pin number, populate the state
    uint32_t idx = 0;
    for (pin = 0; pin < sizeof(pin_select) * 8; pin++)
    {
        if ((1ULL << pin) & pin_select)
        {
            SYS_LOG_DBG("Registering button input: %d", pin);
            debounce_t *button_event_info = &debounce[idx];

            button_event_info->pin = pin;
            button_event_info->down_time = 0;
            button_event_info->allow_multiple_long_press_event = 1;
            button_event_info->is_inverted = (pull_mode == GPIO_PULLUP_ONLY);
            idx++;
        }
    }

    os_work_create(&s_work_hdl, "", _work_handler_button, NULL, OS_PRIORITY_HIGHEST);
    os_work_submit(default_os_work_q_hdl, &s_work_hdl, 1000);

    SYS_LOG_INF("Initialized");

    return &s_event_queue;
}

/**
 * @brief 添加回调函数：当由 button_init() 设置的对应的引脚号产生按键状态变化时，由内部线程创建执行的回调函数。
 *
 * @param pin 0..n
 * @param callback int (*)(const button_event_t *event)
 */
void button_event_add_callback(uint32_t pin, BUTTON_EVENT_CALLBACK callback)
{
    button_callback_t *callback_obj = os_malloc(sizeof(button_callback_t));
    callback_obj->pin = pin;
    callback_obj->event_callback = callback;
    callback_obj->next = NULL;
    if (g_callback_list_head == NULL)
    {
        g_callback_list_head = callback_obj;
        g_callback_list_tail = g_callback_list_head;
    }
    else
    {
        g_callback_list_tail->next = callback_obj;
        g_callback_list_tail = callback_obj;
    }
}

void button_event_remove_callback(uint32_t pin, BUTTON_EVENT_CALLBACK callback)
{
    button_callback_t *tmp = g_callback_list_head;
    button_callback_t *previous_item = NULL;
    SYS_LOG_DBG("remove pin:%d", pin);
    while (tmp != NULL)
    {
        if (tmp->event_callback == callback && tmp->pin == pin)
        {
            if (previous_item)
            {
                previous_item->next = tmp->next; // 断开当前点节，并将前一个和后一个连接起来
                os_free(tmp);
                SYS_LOG_DBG("found item");
                tmp = previous_item->next;
            }
            else
            {
                // 没有上一个节点，则将头部节点，设置为待移除节点的下一个节点
                if (tmp->next)
                {
                    g_callback_list_head = tmp->next;
                }
                else
                {
                    g_callback_list_head = NULL;
                    SYS_LOG_DBG("####!!!!#####!!!!");
                }
                os_free(tmp);
                tmp = g_callback_list_head;
                SYS_LOG_DBG("case111");
            }
            SYS_LOG_DBG("2222");
        }
        else
        {
            previous_item = tmp;
            tmp = tmp->next;
            SYS_LOG_DBG("1111");
        }
    }

    // 更新g_callback_list_tail指针
    while (tmp != NULL)
    {
        g_callback_list_tail = tmp;
        tmp = tmp->next;
    }

    SYS_LOG_DBG("doneeee1");
}