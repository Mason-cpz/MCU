#include "drivers/button.h"

static bool read_pressed(const button_t *self)
{
    const bool level = hal_gpio_read(self->pin);
    return self->active_low ? !level : level;
}

void button_init(
    button_t             *self,
    const hal_gpio_pin_t *pin,
    bool                  active_low,
    uint16_t              debounce_ms,
    button_edge_t         edge)
{
    self->pin               = pin;
    self->active_low        = active_low;
    self->debounce_ms       = debounce_ms;
    self->edge              = edge;
    self->stable_pressed    = false;
    self->candidate_pressed = false;
    self->candidate_since_ms = 0U;
    self->started           = false;
}

button_event_t button_poll(button_t *self, uint32_t now_ms)
{
    const bool pressed = read_pressed(self);
    button_event_t raw;
    button_event_t out;

    /* 第一次采样：建立基线，不上报任何事件。 */
    if (!self->started) {
        self->started           = true;
        self->stable_pressed    = pressed;
        self->candidate_pressed = pressed;
        self->candidate_since_ms = now_ms;
        return BUTTON_EVENT_NONE;
    }

    /* 候选电平发生变化：重置候选计时。 */
    if (pressed != self->candidate_pressed) {
        self->candidate_pressed = pressed;
        self->candidate_since_ms = now_ms;
        return BUTTON_EVENT_NONE;
    }

    /* 候选维持到超过消抖窗口：确认为一次稳定跳变。 */
    if (self->candidate_pressed != self->stable_pressed &&
        (uint32_t)(now_ms - self->candidate_since_ms) >= self->debounce_ms) {
        self->stable_pressed = self->candidate_pressed;
        raw = self->stable_pressed ? BUTTON_EVENT_PRESS : BUTTON_EVENT_RELEASE;
    } else {
        return BUTTON_EVENT_NONE;
    }

    /* 按 edge 配置过滤：只把 app 关心的边沿上抛。 */
    if (raw == BUTTON_EVENT_PRESS && (self->edge & BUTTON_EDGE_RISING)) {
        out = BUTTON_EVENT_PRESS;
    } else if (raw == BUTTON_EVENT_RELEASE && (self->edge & BUTTON_EDGE_FALLING)) {
        out = BUTTON_EVENT_RELEASE;
    } else {
        out = BUTTON_EVENT_NONE;
    }

    return out;
}

bool button_is_pressed(const button_t *self)
{
    return self->stable_pressed;
}
