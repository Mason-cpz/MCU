#ifndef DRIVERS_BUTTON_H
#define DRIVERS_BUTTON_H

#include "hal/gpio.h"

#include <stdbool.h>
#include <stdint.h>

/* 按键驱动。
 * 对上层输出稳定的 GPIO 物理边沿事件，不暴露原始抖动电平。
 * active_low 只影响 button_is_pressed() 的解释，不参与边沿判断。
 * edge 只过滤上报哪一类稳定边沿，按键本身仍按物理电平采样。 */
typedef enum {
    BUTTON_EVENT_NONE = 0,
    BUTTON_EVENT_RISING,
    BUTTON_EVENT_FALLING,
} button_event_t;

typedef enum {
    BUTTON_EDGE_NONE = 0,
    BUTTON_EDGE_RISING = 0x1U,
    BUTTON_EDGE_FALLING = 0x2U,
    BUTTON_EDGE_BOTH = BUTTON_EDGE_RISING | BUTTON_EDGE_FALLING,
} button_edge_t;

typedef struct {
    const hal_gpio_pin_t *pin;
    bool active_low;
    uint16_t debounce_ms;
    button_edge_t edge;
    bool stable_level;
    bool candidate_level;
    uint32_t candidate_since_ms;
    bool started;
} button_t;

void button_init(
    button_t *self,
    const hal_gpio_pin_t *pin,
    bool active_low,
    uint16_t debounce_ms,
    button_edge_t edge);

button_event_t button_poll(button_t *self, uint32_t now_ms);
bool button_is_pressed(const button_t *self);

#endif
