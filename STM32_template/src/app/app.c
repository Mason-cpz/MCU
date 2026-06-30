#include "app/app.h"

#include <stddef.h>

void app_init(app_t *self, const app_dependencies_t *deps)
{
    uint8_t i;
    const uint32_t now_ms = hal_millis(deps->timebase);

    self->timebase = deps->timebase;
    self->led_count = deps->led_count;
    if (self->led_count > APP_LED_MAX) {
        self->led_count = APP_LED_MAX;
    }
    self->has_bicolor = false;

    for (i = 0U; i < self->led_count; ++i) {
        led_init(&self->leds[i], &deps->led_pins[i], deps->led_polarities[i]);

        switch (deps->behaviors[i].behavior) {
        case APP_LED_STEADY_ON:
            led_set(&self->leds[i], LED_ON);
            break;
        case APP_LED_BLINK:
            led_set_blink(
                &self->leds[i],
                deps->behaviors[i].on_ms,
                deps->behaviors[i].off_ms,
                now_ms);
            break;
        case APP_LED_OFF:
        default:
            break;
        }
    }

    if (deps->bicolor != NULL &&
        deps->bicolor->enabled &&
        deps->bicolor->green_index < self->led_count &&
        deps->bicolor->red_index < self->led_count) {
        bicolor_led_init(
            &self->bicolor,
            &self->leds[deps->bicolor->green_index],
            &self->leds[deps->bicolor->red_index]);

        if (deps->bicolor->blink) {
            bicolor_led_blink(
                &self->bicolor,
                deps->bicolor->color,
                deps->bicolor->on_ms,
                deps->bicolor->off_ms,
                now_ms);
        } else {
            bicolor_led_set(&self->bicolor, deps->bicolor->color);
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
