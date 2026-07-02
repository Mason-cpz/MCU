#include <assert.h>
#include <stddef.h>

#include "drivers/led/bicolor_led.h"

void bicolor_led_init(bicolor_led_t *self, led_t *green, led_t *red)
{
    assert(self != NULL);
    assert(green != NULL);
    assert(red != NULL);

    self->green = green;
    self->red = red;
    self->color = BICOLOR_OFF;
}

void bicolor_led_set(bicolor_led_t *self, bicolor_color_t color)
{
    assert(self != NULL);
    assert(self->green != NULL);
    assert(self->red != NULL);

    self->color = color;
    /* 本轮只组合静态颜色；颜色闪烁可在后续基于通道 blink 扩展。 */
    switch (color) {
    case BICOLOR_GREEN:
        led_set(self->green, LED_ON);
        led_set(self->red, LED_OFF);
        break;
    case BICOLOR_RED:
        led_set(self->green, LED_OFF);
        led_set(self->red, LED_ON);
        break;
    case BICOLOR_YELLOW:
        led_set(self->green, LED_ON);
        led_set(self->red, LED_ON);
        break;
    case BICOLOR_OFF:
    default:
        led_set(self->green, LED_OFF);
        led_set(self->red, LED_OFF);
        break;
    }
}

bicolor_color_t bicolor_led_get(const bicolor_led_t *self)
{
    assert(self != NULL);
    assert(self->green != NULL);
    assert(self->red != NULL);

    return self->color;
}
