#include "drivers/button.h"

#include <assert.h>
#include <stddef.h>

static bool read_level(const button_t *self)
{
    return hal_gpio_read(self->pin);
}

void button_init(
    button_t *self,
    const hal_gpio_pin_t *pin,
    bool active_low,
    uint16_t debounce_ms,
    button_edge_t edge)
{
    assert(self != NULL);
    assert(pin != NULL);

    self->pin = pin;
    self->active_low = active_low;
    self->debounce_ms = debounce_ms;
    self->edge = edge;
    self->stable_level = false;
    self->candidate_level = false;
    self->candidate_since_ms = 0U;
    self->started = false;
}

button_event_t button_poll(button_t *self, uint32_t now_ms)
{
    button_event_t event;

    assert(self != NULL);
    assert(self->pin != NULL);

    const bool level = read_level(self);

    if (!self->started) {
        self->started = true;
        self->stable_level = level;
        self->candidate_level = level;
        self->candidate_since_ms = now_ms;
        return BUTTON_EVENT_NONE;
    }

    if (level != self->candidate_level) {
        self->candidate_level = level;
        self->candidate_since_ms = now_ms;
        return BUTTON_EVENT_NONE;
    }

    if (self->candidate_level == self->stable_level) {
        return BUTTON_EVENT_NONE;
    }

    if ((uint32_t)(now_ms - self->candidate_since_ms) < self->debounce_ms) {
        return BUTTON_EVENT_NONE;
    }

    self->stable_level = self->candidate_level;
    event = self->stable_level ? BUTTON_EVENT_RISING : BUTTON_EVENT_FALLING;

    if (event == BUTTON_EVENT_RISING &&
        (self->edge & BUTTON_EDGE_RISING) != 0U) {
        return event;
    }

    if (event == BUTTON_EVENT_FALLING &&
        (self->edge & BUTTON_EDGE_FALLING) != 0U) {
        return event;
    }

    return BUTTON_EVENT_NONE;
}

bool button_is_pressed(const button_t *self)
{
    assert(self != NULL);

    return self->active_low ? !self->stable_level : self->stable_level;
}
