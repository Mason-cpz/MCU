#include "drivers/button.h"

static bool read_pressed(const button_t *self)
{
    const bool level = hal_gpio_read(self->pin);
    return self->active_low ? !level : level;
}

void button_init(
    button_t *self,
    const hal_gpio_pin_t *pin,
    bool active_low,
    uint16_t debounce_ms)
{
    self->pin = pin;
    self->active_low = active_low;
    self->debounce_ms = debounce_ms;
    self->stable_pressed = false;
    self->candidate_pressed = false;
    self->candidate_since_ms = 0U;
    self->started = false;
}

button_event_t button_poll(button_t *self, uint32_t now_ms)
{
    const bool pressed = read_pressed(self);

    if (!self->started) {
        self->started = true;
        self->stable_pressed = pressed;
        self->candidate_pressed = pressed;
        self->candidate_since_ms = now_ms;
        return BUTTON_EVENT_NONE;
    }

    if (pressed != self->candidate_pressed) {
        self->candidate_pressed = pressed;
        self->candidate_since_ms = now_ms;
        return BUTTON_EVENT_NONE;
    }

    if (self->candidate_pressed != self->stable_pressed &&
        (uint32_t)(now_ms - self->candidate_since_ms) >= self->debounce_ms) {
        self->stable_pressed = self->candidate_pressed;
        return self->stable_pressed ? BUTTON_EVENT_PRESSED : BUTTON_EVENT_RELEASED;
    }

    return BUTTON_EVENT_NONE;
}

bool button_is_pressed(const button_t *self)
{
    return self->stable_pressed;
}
