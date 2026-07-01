#include "drivers/button.h"

/* 读取"逻辑按下"：按极性把原始电平换算成是否按下。 */
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
    uint16_t              short_press_ms)
{
    self->pin            = pin;
    self->active_low     = active_low;
    self->debounce_ms    = debounce_ms;
    self->short_press_ms =
        (short_press_ms != 0U) ? short_press_ms : BUTTON_SHORT_PRESS_MS_DEFAULT;

    self->stable_pressed     = false;
    self->candidate_pressed  = false;
    self->candidate_since_ms = 0U;
    self->started            = false;
    self->pressed_since_ms   = 0U;
}

/* 返回去抖后本 tick 的稳定沿：+1=按下沿，-1=抬起沿，0=无变化。 */
static int debounce_edge(button_t *self, uint32_t now_ms)
{
    const bool pressed = read_pressed(self);

    /* 首次采样：以当前电平作为稳定基准，不产生沿。 */
    if (!self->started) {
        self->started           = true;
        self->stable_pressed    = pressed;
        self->candidate_pressed = pressed;
        self->candidate_since_ms = now_ms;
        return 0;
    }

    /* 候选变化：重置候选与计时，等待稳定。 */
    if (pressed != self->candidate_pressed) {
        self->candidate_pressed  = pressed;
        self->candidate_since_ms = now_ms;
        return 0;
    }

    /* 候选与稳定不同，且已持续超过消抖窗口：确认为新的稳定状态。 */
    if (self->candidate_pressed != self->stable_pressed &&
        (uint32_t)(now_ms - self->candidate_since_ms) >= self->debounce_ms) {
        self->stable_pressed = self->candidate_pressed;
        return self->stable_pressed ? 1 : -1;
    }

    return 0;
}

button_event_t button_poll(button_t *self, uint32_t now_ms)
{
    const int edge = debounce_edge(self, now_ms);

    if (edge > 0) {
        /* 稳定按下沿：记录起始时刻，事件在抬起时才判定。 */
        self->pressed_since_ms = now_ms;
        return BUTTON_EVENT_NONE;
    }

    if (edge < 0) {
        /* 稳定抬起沿：按下时长在短按窗口内则算一次短按。 */
        const uint32_t held = (uint32_t)(now_ms - self->pressed_since_ms);
        if (held <= self->short_press_ms) {
            return BUTTON_EVENT_SHORT_PRESS;
        }
        /* 超时的长按：本 Demo 不产生事件，留给后续长按手势。 */
    }

    return BUTTON_EVENT_NONE;
}

bool button_is_pressed(const button_t *self)
{
    return self->stable_pressed;
}
