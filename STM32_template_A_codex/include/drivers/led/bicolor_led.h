#ifndef DRIVERS_BICOLOR_LED_H
#define DRIVERS_BICOLOR_LED_H

#include "drivers/led/led.h"

#include <stdint.h>

typedef enum {
    BICOLOR_OFF = 0,
    BICOLOR_GREEN,
    BICOLOR_RED,
    BICOLOR_YELLOW,
} bicolor_color_t;

typedef struct {
    led_t *green;
    led_t *red;
    bicolor_color_t color;
} bicolor_led_t;

void bicolor_led_init(bicolor_led_t *self, led_t *green, led_t *red);
void bicolor_led_set(bicolor_led_t *self, bicolor_color_t color);
bicolor_color_t bicolor_led_get(const bicolor_led_t *self);

#endif
