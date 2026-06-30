#include "app/app.h"

#include <stddef.h>

void app_init(
    app_t *self,
    const stm32f1_board_t *board,
    const app_led_behavior_desc_t *behaviors,
    const app_bicolor_desc_t *bicolor)
{
    uint8_t i;
    const hal_timebase_t *timebase = stm32f1_board_timebase(board);
    const hal_gpio_pin_t *pins = stm32f1_board_led_pins(board);
    const led_polarity_t *polarities = stm32f1_board_led_polarities(board);
    const uint32_t now_ms = hal_millis(timebase);

    self->timebase = timebase;
    self->led_count = stm32f1_board_led_count(board);
    if (self->led_count > APP_LED_MAX) {
        self->led_count = APP_LED_MAX;
    }
    self->has_bicolor = false;

    for (i = 0U; i < self->led_count; ++i) {
        led_init(&self->leds[i], &pins[i], polarities[i]);

        switch (behaviors[i].behavior) {
        case APP_LED_STEADY_ON:
            led_set(&self->leds[i], LED_ON);
            break;
        case APP_LED_BLINK:
            led_set_blink(
                &self->leds[i],
                behaviors[i].on_ms,
                behaviors[i].off_ms,
                now_ms);
            break;
        case APP_LED_OFF:
        default:
            break;
        }
    }

    if (bicolor != NULL &&
        bicolor->enabled &&
        bicolor->green_index < self->led_count &&
        bicolor->red_index < self->led_count) {
        bicolor_led_init(
            &self->bicolor,
            &self->leds[bicolor->green_index],
            &self->leds[bicolor->red_index]);

        if (bicolor->blink) {
            bicolor_led_blink(
                &self->bicolor,
                bicolor->color,
                bicolor->on_ms,
                bicolor->off_ms,
                now_ms);
        } else {
            bicolor_led_set(&self->bicolor, bicolor->color);
        }

        self->has_bicolor = true;
    }
}

void app_tick(app_t *self)
{
    uint8_t i;
    const uint32_t now_ms = hal_millis(self->timebase);

    for (i = 0U; i < self->led_count; ++i) {
        led_tick(&self->leds[i], now_ms);
    }
}
