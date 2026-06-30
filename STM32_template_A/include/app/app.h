#ifndef APP_APP_H
#define APP_APP_H

#include "drivers/bicolor_led.h"
#include "drivers/led.h"
#include "hal/gpio.h"
#include "hal/timebase.h"

#include <stdbool.h>
#include <stdint.h>

enum {
    APP_LED_MAX = 8,
};

typedef enum {
    APP_LED_OFF = 0,
    APP_LED_STEADY_ON,
    APP_LED_BLINK,
} app_led_behavior_t;

typedef struct {
    app_led_behavior_t behavior;
    uint16_t on_ms;
    uint16_t off_ms;
} app_led_behavior_desc_t;

typedef struct {
    bool enabled;
    uint8_t green_index;
    uint8_t red_index;
    bicolor_color_t color;
    bool blink;
    uint16_t on_ms;
    uint16_t off_ms;
} app_bicolor_desc_t;

typedef struct {
    const hal_timebase_t *timebase;
    const hal_gpio_pin_t *led_pins;
    const led_polarity_t *led_polarities;
    uint8_t led_count;
    const app_led_behavior_desc_t *behaviors;
    const app_bicolor_desc_t *bicolor;
} app_dependencies_t;

typedef struct {
    const hal_timebase_t *timebase;
    led_t leds[APP_LED_MAX];
    uint8_t led_count;
    bicolor_led_t bicolor;
    bool has_bicolor;
} app_t;

void app_init(app_t *self, const app_dependencies_t *deps);
void app_tick(app_t *self);

#endif
