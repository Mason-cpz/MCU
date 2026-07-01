#ifndef DRIVERS_BUTTON_H
#define DRIVERS_BUTTON_H

#include "hal/gpio.h"

#include <stdbool.h>
#include <stdint.h>

/* 按键驱动框架。
 *
 * 设计目标：
 *  - 对上层只输出"边沿事件"，不暴露抖动的原始 GPIO。
 *  - 支持配置为上升沿触发 / 下降沿触发 / 双边沿触发。
 *  - 内置固定窗口消抖，app 不需要再处理抖动。
 *  - 上层新增按键只需要：描述表加一行 + app 收事件做映射。
 */

typedef enum {
    BUTTON_EVENT_NONE = 0,   /* 无事件          */
    BUTTON_EVENT_PRESS,      /* "按下"边沿事件  */
    BUTTON_EVENT_RELEASE,    /* "释放"边沿事件  */
} button_event_t;

/* 触发边沿选择：决定哪一类稳定边沿会作为"业务事件"上报。
 *  - BUTTON_EDGE_RISING  : pressed(0->1) 稳定后上报一次 PRESS
 *  - BUTTON_EDGE_FALLING : released(1->0) 稳定后上报一次 RELEASE
 *  - BUTTON_EDGE_BOTH    : 两种稳定边沿都上报
 *
 * 按键逻辑层仍按 "物理 pressed/released" 工作，
 * active_low / edge 只是决定哪一类事件上抛给 app。 */
typedef enum {
    BUTTON_EDGE_NONE    = 0,
    BUTTON_EDGE_RISING  = 0x1,
    BUTTON_EDGE_FALLING = 0x2,
    BUTTON_EDGE_BOTH    = BUTTON_EDGE_RISING | BUTTON_EDGE_FALLING,
} button_edge_t;

typedef struct {
    const hal_gpio_pin_t *pin;
    bool        active_low;      /* true=低电平视为按下 */
    uint16_t    debounce_ms;     /* 消抖窗口           */
    button_edge_t edge;          /* 业务关心的稳定边沿 */

    bool        stable_pressed;  /* 已稳定的电平状态     */
    bool        candidate_pressed;
    uint32_t    candidate_since_ms;
    bool        started;
} button_t;

/* 初始化一个按键实例。
 * pin          底层 GPIO 抽象
 * active_low   物理按下是否为低电平
 * debounce_ms  消抖窗口（建议 10~30ms）
 * edge         关心的业务边沿（上升/下降/双边） */
void button_init(
    button_t             *self,
    const hal_gpio_pin_t *pin,
    bool                  active_low,
    uint16_t              debounce_ms,
    button_edge_t         edge);

/* 周期调用（建议 1~10ms 一次），喂入当前时间戳。
 * 返回值：按 edge 配置过滤后的事件；BUTTON_EVENT_NONE 表示本次无事件。 */
button_event_t button_poll(button_t *self, uint32_t now_ms);

/* 读取当前稳定按下状态（不带抖动）。 */
bool button_is_pressed(const button_t *self);

#endif
