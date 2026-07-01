/* app.c
 * 应用层 Demo：消费按键事件 -> 驱动三组 LED 状态机。
 *
 * 状态机用"状态枚举 + 计数器"实现，每次有效事件只做：
 *   state = (state + 1) % N;   apply(state);
 * 加状态/改顺序只动枚举与 apply_*，不动 BSP/驱动。
 *
 * LED1 设计：上电进入 HEARTBEAT（1Hz 心跳），第一次按 KEY1 跳进
 * 业务循环并从 OFF 开始；之后严格按规格 OFF->ON->BLINK->OFF 循环。
 * HEARTBEAT 只作为启动态，不在业务循环里复现，符合规格"上电默认态"。
 */
#include "app/app.h"

#include "bsp/bsp.h"
#include "drivers/bicolor_led.h"
#include "drivers/button.h"
#include "drivers/led.h"

#include <stddef.h>   /* NULL */

/* —— KEY1 -> LED1 状态机 ——
 * HEARTBEAT 是上电初始态，不计入业务循环计数；
 * 第一次按键从 HEARTBEAT 直接跳到 OFF（循环起点）。 */
typedef enum {
    LED1_STATE_HEARTBEAT = 0,  /* 上电心跳，仅启动态 */
    LED1_STATE_OFF,           /* 业务循环起点       */
    LED1_STATE_ON,
    LED1_STATE_BLINK,
    LED1_STATE_COUNT,
} led1_state_t;

/* —— KEY2 -> LED2 状态机 —— */
typedef enum {
    LED2_STATE_OFF = 0,
    LED2_STATE_ON,
    LED2_STATE_BLINK_FAST,    /* 200/800 */
    LED2_STATE_BLINK_SLOW,    /* 800/200 */
    LED2_STATE_COUNT,
} led2_state_t;

/* —— KEY3 -> 双色灯状态机 —— */
typedef enum {
    BI_STATE_OFF = 0,
    BI_STATE_GREEN,
    BI_STATE_RED,
    BI_STATE_YELLOW,
    BI_STATE_YELLOW_BLINK,    /* 500/500 同相同步闪黄 */
    BI_STATE_COUNT,
} bi_state_t;

static led1_state_t s_led1_state;
static led2_state_t s_led2_state;
static bi_state_t   s_bi_state;

/* —— 各状态机应用到硬件 —— */
static void apply_led1(led1_state_t st, uint32_t now_ms)
{
    led_t *led = bsp_led1();

    switch (st) {
    case LED1_STATE_HEARTBEAT:
        led_set_blink(led, 500U, 500U, now_ms);  /* 1Hz 心跳 */
        break;
    case LED1_STATE_OFF:
        led_set(led, LED_OFF);
        break;
    case LED1_STATE_ON:
        led_set(led, LED_ON);
        break;
    case LED1_STATE_BLINK:
        led_set_blink(led, 500U, 500U, now_ms);
        break;
    default:
        break;
    }
}

static void apply_led2(led2_state_t st, uint32_t now_ms)
{
    led_t *led = bsp_led2();

    switch (st) {
    case LED2_STATE_OFF:
        led_set(led, LED_OFF);
        break;
    case LED2_STATE_ON:
        led_set(led, LED_ON);
        break;
    case LED2_STATE_BLINK_FAST:
        led_set_blink(led, 200U, 800U, now_ms);
        break;
    case LED2_STATE_BLINK_SLOW:
        led_set_blink(led, 800U, 200U, now_ms);
        break;
    default:
        break;
    }
}

static void apply_bi(bi_state_t st, uint32_t now_ms)
{
    bicolor_led_t *bi = bsp_bicolor();

    switch (st) {
    case BI_STATE_OFF:
        bicolor_led_set(bi, BICOLOR_OFF);
        break;
    case BI_STATE_GREEN:
        bicolor_led_set(bi, BICOLOR_GREEN);
        break;
    case BI_STATE_RED:
        bicolor_led_set(bi, BICOLOR_RED);
        break;
    case BI_STATE_YELLOW:
        bicolor_led_set(bi, BICOLOR_YELLOW);
        break;
    case BI_STATE_YELLOW_BLINK:
        bicolor_led_blink(bi, BICOLOR_YELLOW, 500U, 500U, now_ms);
        break;
    default:
        break;
    }
}

/* LED1 第一次按键：从 HEARTBEAT 直接跳到业务循环起点 OFF。
 * 之后按规格 OFF->ON->BLINK->OFF 循环。 */
static led1_state_t led1_next(led1_state_t cur)
{
    if (cur == LED1_STATE_HEARTBEAT) {
        return LED1_STATE_OFF;
    }
    /* 业务循环：OFF -> ON -> BLINK -> OFF */
    return (cur == LED1_STATE_BLINK) ? LED1_STATE_OFF : (led1_state_t)(cur + 1U);
}

void app_init(void)
{
    const uint32_t now_ms = bsp_now_ms();

    s_led1_state = LED1_STATE_HEARTBEAT;
    s_led2_state = LED2_STATE_OFF;
    s_bi_state   = BI_STATE_OFF;

    apply_led1(s_led1_state, now_ms);
    apply_led2(s_led2_state, now_ms);
    apply_bi(s_bi_state, now_ms);
}

void app_tick(void)
{
    const uint32_t now_ms = bsp_now_ms();
    button_t *key;

    /* KEY1 上升沿 -> LED1 状态机前进一步。 */
    key = bsp_key(BSP_KEY_1);
    if (key != NULL && button_poll(key, now_ms) != BUTTON_EVENT_NONE) {
        s_led1_state = led1_next(s_led1_state);
        apply_led1(s_led1_state, now_ms);
    }

    /* KEY2 下降沿 -> LED2 状态机前进一步 */
    key = bsp_key(BSP_KEY_2);
    if (key != NULL && button_poll(key, now_ms) != BUTTON_EVENT_NONE) {
        s_led2_state = (led2_state_t)((s_led2_state + 1U) % LED2_STATE_COUNT);
        apply_led2(s_led2_state, now_ms);
    }

    /* KEY3 下降沿 -> 双色灯状态机前进一步 */
    key = bsp_key(BSP_KEY_3);
    if (key != NULL && button_poll(key, now_ms) != BUTTON_EVENT_NONE) {
        s_bi_state = (bi_state_t)((s_bi_state + 1U) % BI_STATE_COUNT);
        apply_bi(s_bi_state, now_ms);
    }
}
