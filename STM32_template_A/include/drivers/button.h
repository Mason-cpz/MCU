#ifndef DRIVERS_BUTTON_H
#define DRIVERS_BUTTON_H

#include "hal/gpio.h"

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    BUTTON_EVENT_NONE = 0,
    BUTTON_EVENT_PRESSED,
    BUTTON_EVENT_RELEASED,
} button_event_t;

typedef struct {
    const hal_gpio_pin_t *pin;
    bool active_low;
    uint16_t debounce_ms;
    bool stable_pressed;
    bool candidate_pressed;
    uint32_t candidate_since_ms;
    bool started;
} button_t;

void button_init(
    button_t *self,
    const hal_gpio_pin_t *pin,
    bool active_low,
    uint16_t debounce_ms);

button_event_t button_poll(button_t *self, uint32_t now_ms);
bool button_is_pressed(const button_t *self);

#endif
