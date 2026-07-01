#ifndef DRIVERS_BUTTON_H
#define DRIVERS_BUTTON_H

#include "hal/gpio.h"

#include <stdbool.h>
#include <stdint.h>

/* 按键驱动：把抖动的原始 GPIO 电平转成稳定的语义事件。
 *
 * 两级处理：
 *   1) 消抖：DEBOUNCE_MS 窗口内电平稳定才认账，得到稳定的按下/抬起沿。
 *   2) 手势：按下后在 short_press_ms 内抬起 = 一次 SHORT_PRESS。
 *
 * 只输出语义事件，上层(app)不碰原始电平。
 * 加长按/双击/连按时在此扩展事件与手势判定，app 侧只多认几个枚举。 */
typedef enum {
    BUTTON_EVENT_NONE = 0,
    BUTTON_EVENT_SHORT_PRESS,   /* 按下并在 short_press_ms 内抬起 */
    /* 预留：BUTTON_EVENT_LONG_PRESS / DOUBLE_CLICK / REPEAT ... */
} button_event_t;

#define BUTTON_SHORT_PRESS_MS_DEFAULT  1000U

typedef struct {
    const hal_gpio_pin_t *pin;
    bool     active_low;         /* 按下时为低电平(下降沿键)则为 true */
    uint16_t debounce_ms;
    uint16_t short_press_ms;     /* 短按上限，超过则本次不算短按 */

    /* 消抖状态 */
    bool     stable_pressed;     /* 去抖后的稳定按下状态 */
    bool     candidate_pressed;  /* 待确认的候选状态 */
    uint32_t candidate_since_ms; /* 候选状态起始时刻 */
    bool     started;            /* 是否已完成首次采样初始化 */

    /* 手势状态 */
    uint32_t pressed_since_ms;   /* 稳定按下的起始时刻，用于时长判定 */
} button_t;

/* short_press_ms 传 0 表示用 BUTTON_SHORT_PRESS_MS_DEFAULT。 */
void button_init(
    button_t             *self,
    const hal_gpio_pin_t *pin,
    bool                  active_low,
    uint16_t              debounce_ms,
    uint16_t              short_press_ms);

/* 每个 tick 调用一次；返回本 tick 产生的语义事件(通常为 NONE)。 */
button_event_t button_poll(button_t *self, uint32_t now_ms);

/* 去抖后的当前是否按下(供长按等后续手势或调试使用)。 */
bool button_is_pressed(const button_t *self);

#endif
