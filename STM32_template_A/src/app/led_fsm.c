/* led_fsm.c
 * LED 状态机层：三组灯的状态枚举 + 跳转规则 + 灯效映射。
 *
 * 从 app.c 抽出，让 app.c 只做"按键事件路由"。
 * 本层职责：
 *  - 定义每组灯的状态枚举（业务规则）
 *  - 定义状态跳转规则（next）
 *  - 定义 状态->灯效 的映射表
 *  - 调 led_bsp_apply 应用灯效（时序参数在 led_bsp）
 *
 * 改业务流程（增删状态、改顺序）：只动本文件的枚举 + 映射表。
 */
#include "app/led_fsm.h"

#include "peripherals/led/led_bsp.h"

/* —— KEY1 -> LED1 状态机 ——
 * HEARBEAT 是上电初始态，不计入业务循环计数；
 * 第一次按键从 HEARBEAT 直接跳到 OFF（循环起点）。 */
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
    LED2_STATE_BLINK_FAST,
    LED2_STATE_BLINK_SLOW,
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

/* —— 状态 -> 灯效 映射表 —— */
static const led_effect_t s_led1_effect[LED1_STATE_COUNT] = {
    [LED1_STATE_HEARTBEAT] = LE_BLINK_HEARTBEAT,
    [LED1_STATE_OFF]       = LE_OFF,
    [LED1_STATE_ON]        = LE_ON,
    [LED1_STATE_BLINK]     = LE_BLINK_HEARTBEAT,
};

static const led_effect_t s_led2_effect[LED2_STATE_COUNT] = {
    [LED2_STATE_OFF]         = LE_OFF,
    [LED2_STATE_ON]          = LE_ON,
    [LED2_STATE_BLINK_FAST]  = LE_BLINK_FAST,
    [LED2_STATE_BLINK_SLOW]  = LE_BLINK_SLOW,
};

static const bicolor_effect_t s_bi_effect[BI_STATE_COUNT] = {
    [BI_STATE_OFF]          = BE_OFF,
    [BI_STATE_GREEN]        = BE_GREEN,
    [BI_STATE_RED]          = BE_RED,
    [BI_STATE_YELLOW]       = BE_YELLOW,
    [BI_STATE_YELLOW_BLINK] = BE_YELLOW_BLINK,
};

/* —— 运行期状态 —— */
static led1_state_t s_led1_state;
static led2_state_t s_led2_state;
static bi_state_t   s_bi_state;

/* LED1 跳转规则：HEARTBEAT -> OFF；之后 OFF->ON->BLINK->OFF 循环 */
static led1_state_t led1_next(led1_state_t cur)
{
    if (cur == LED1_STATE_HEARTBEAT) {
        return LED1_STATE_OFF;
    }
    return (cur == LED1_STATE_BLINK) ? LED1_STATE_OFF : (led1_state_t)(cur + 1U);
}

/* —— 对外接口 —— */
void led_fsm_init(uint32_t now_ms)
{
    s_led1_state = LED1_STATE_HEARTBEAT;
    s_led2_state = LED2_STATE_OFF;
    s_bi_state   = BI_STATE_OFF;

    led_bsp_apply(BSP_LED_1, s_led1_effect[s_led1_state], now_ms);
    led_bsp_apply(BSP_LED_2, s_led2_effect[s_led2_state], now_ms);
    led_bsp_bicolor_apply(s_bi_effect[s_bi_state], now_ms);
}

void led_fsm_on_key1(uint32_t now_ms)
{
    s_led1_state = led1_next(s_led1_state);
    led_bsp_apply(BSP_LED_1, s_led1_effect[s_led1_state], now_ms);
}

void led_fsm_on_key2(uint32_t now_ms)
{
    s_led2_state = (led2_state_t)((s_led2_state + 1U) % LED2_STATE_COUNT);
    led_bsp_apply(BSP_LED_2, s_led2_effect[s_led2_state], now_ms);
}

void led_fsm_on_key3(uint32_t now_ms)
{
    s_bi_state = (bi_state_t)((s_bi_state + 1U) % BI_STATE_COUNT);
    led_bsp_bicolor_apply(s_bi_effect[s_bi_state], now_ms);
}
