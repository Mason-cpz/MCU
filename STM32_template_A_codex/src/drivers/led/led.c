#include <assert.h>
#include <stddef.h>

#include "drivers/led/led.h"

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
    self->state = LED_OFF;
    led_apply(self, LED_OFF);
}

void led_set(led_t *self, led_state_t state)
{
    assert(self != NULL);

    led_apply(self, state);
}

void led_toggle(led_t *self)
{
    assert(self != NULL);

    led_set(self, self->state == LED_ON ? LED_OFF : LED_ON);
}

led_state_t led_is_on(const led_t *self)
{
    assert(self != NULL);

    return self->state;
}
