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
} bicolor_led_t;

void bicolor_led_init(bicolor_led_t *self, led_t *green, led_t *red);
void bicolor_led_set(bicolor_led_t *self, bicolor_color_t color);
/* 以指定颜色闪烁：该颜色点亮的通道按 on_ms/off_ms 闪烁，其余通道熄灭。
 * 两通道共用同一 now_ms，保证绿/红同相（YELLOW 即同步亮灭的闪烁黄）。 */
void bicolor_led_blink(
    bicolor_led_t *self,
    bicolor_color_t color,
    uint16_t on_ms,
    uint16_t off_ms,
    uint32_t now_ms);
bicolor_color_t bicolor_led_get(const bicolor_led_t *self);

#endif
