/**
 * @file button_event.h
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2023-09-04
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include "os/os.h"
#include "driver/gpio.h"

#define PIN_BIT(x) (1ULL << x)

#define LONG_PRESS_DURATION (1000)
#define LONG_PRESS_REPEAT (200)

typedef enum
{
    BUTTON_UP,
    BUTTON_DOWN,
    BUTTON_HELD
} button_event_e;

typedef struct
{
    uint32_t pin;
    button_event_e event;
} button_event_t;

typedef int (*BUTTON_EVENT_CALLBACK)(const button_event_t *event);

typedef struct button_callback_s
{
    struct button_callback_s *next;
    uint32_t pin;
    BUTTON_EVENT_CALLBACK event_callback;
} button_callback_t;

os_queue_t *button_init(unsigned long long pin_select, uint8_t en_lev);

void button_event_add_callback(uint32_t pin, BUTTON_EVENT_CALLBACK callback);
void button_event_remove_callback(uint32_t pin, BUTTON_EVENT_CALLBACK callback);
