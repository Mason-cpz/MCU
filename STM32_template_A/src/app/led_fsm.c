/* led_fsm.c
 * LED 状态机层：把按键事件翻译成下一步灯效。
 *
 * app.c 只负责路由按键事件；本文件只维护三组灯效循环。
 * 改业务流程（增删状态、改顺序）：只调整下面的循环表。
 */
#include "app/led_fsm.h"

#include "peripherals/led/led_bsp.h"

#define ARRAY_SIZE(a) ((uint8_t)(sizeof(a) / sizeof((a)[0])))

/* 每张表表示“下一次对应按键触发时要应用的灯效”。 */
static const led_effect_t s_led1_cycle[] = {
    LE_OFF,              /* 首次按键退出上电心跳，进入 OFF */
    LE_ON,
    LE_BLINK_HEARTBEAT,  /* 500/500 */
};

static const led_effect_t s_led2_cycle[] = {
    LE_ON,
    LE_BLINK_FAST,
    LE_BLINK_SLOW,
    LE_OFF,
};

static const bicolor_effect_t s_bi_cycle[] = {
    BE_GREEN,
    BE_RED,
    BE_YELLOW,
    BE_YELLOW_BLINK,
    BE_OFF,
};

static uint8_t s_led1_next;
static uint8_t s_led2_next;
static uint8_t s_bi_next;

static void advance_index(uint8_t *index, uint8_t count)
{
    *index = (uint8_t)((*index + 1U) % count);
}

/* —— 对外接口 —— */
void led_fsm_init(uint32_t now_ms)
{
    s_led1_next = 0U;
    s_led2_next = 0U;
    s_bi_next   = 0U;

    led_bsp_apply(BSP_LED_1, LE_BLINK_HEARTBEAT, now_ms);
    led_bsp_apply(BSP_LED_2, LE_OFF, now_ms);
    led_bsp_bicolor_apply(BE_OFF, now_ms);
}

void led_fsm_on_key1(uint32_t now_ms)
{
    led_bsp_apply(BSP_LED_1, s_led1_cycle[s_led1_next], now_ms);
    advance_index(&s_led1_next, ARRAY_SIZE(s_led1_cycle));
}

void led_fsm_on_key2(uint32_t now_ms)
{
    led_bsp_apply(BSP_LED_2, s_led2_cycle[s_led2_next], now_ms);
    advance_index(&s_led2_next, ARRAY_SIZE(s_led2_cycle));
}

void led_fsm_on_key3(uint32_t now_ms)
{
    led_bsp_bicolor_apply(s_bi_cycle[s_bi_next], now_ms);
    advance_index(&s_bi_next, ARRAY_SIZE(s_bi_cycle));
}
