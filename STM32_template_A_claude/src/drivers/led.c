#include <assert.h>
#include <stddef.h>

#include "drivers/led.h"

static bool gpio_level_for(const led_t *self, led_state_t state)
{
    if (self->polarity == LED_ACTIVE_HIGH) {
        return state == LED_ON;
    }

    return state == LED_OFF;
}

static void led_apply(led_t *self, led_state_t state)
{
    self->state = state;
    hal_gpio_write(self->pin, gpio_level_for(self, state));
}

void led_init(led_t *self, const hal_gpio_pin_t *pin, led_polarity_t polarity)
{
    assert(self != NULL);
    assert(pin != NULL);

    self->pin = pin;
    self->polarity = polarity;
    self->mode = LED_MODE_STEADY;
    self->state = LED_OFF;
    self->on_ms = 0U;
    self->off_ms = 0U;
    self->next_toggle_ms = 0U;
    led_apply(self, LED_OFF);
}

void led_set(led_t *self, led_state_t state)
{
    assert(self != NULL);

    self->mode = LED_MODE_STEADY;
    led_apply(self, state);
}

void led_toggle(led_t *self)
{
    assert(self != NULL);

    led_set(self, self->state == LED_ON ? LED_OFF : LED_ON);
}

void led_set_blink(led_t *self, uint16_t on_ms, uint16_t off_ms, uint32_t now_ms)
{
    assert(self != NULL);

    self->mode = LED_MODE_BLINK;
    self->on_ms = on_ms;
    self->off_ms = off_ms;
    led_apply(self, LED_ON);
    self->next_toggle_ms = now_ms + on_ms;
}

void led_tick(led_t *self, uint32_t now_ms)
{
    led_state_t next;

    assert(self != NULL);

    if (self->mode != LED_MODE_BLINK) {
        return;
    }

    if ((int32_t)(now_ms - self->next_toggle_ms) < 0) {
        return;
    }

    next = self->state == LED_ON ? LED_OFF : LED_ON;
    led_apply(self, next);
    self->next_toggle_ms = now_ms + (next == LED_ON ? self->on_ms : self->off_ms);
}

led_state_t led_is_on(const led_t *self)
{
    assert(self != NULL);

    return self->state;
}
