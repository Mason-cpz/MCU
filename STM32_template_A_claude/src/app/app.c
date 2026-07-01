/* app.c
 * Demo 状态机实现。核心只剩一个通用循环：
 *   一次 SHORT_PRESS -> index = (index+1) % count -> apply(output, index)
 * 每种输出(单 LED / 双色灯)只提供一个 apply 回调，把 index 翻译成具体效果。
 * 想改 Demo 行为，只改本文件顶部的效果表；想加通道，加表 + apply + 通道。
 */
#include "app/app.h"

#include <assert.h>
#include <stddef.h>

/* ---- KEY1 -> LED1: OFF -> ON -> BLINK(500/500) -> OFF ---- */
static const led_effect_t s_led1_effects[] = {
    { LED_MODE_STEADY, LED_OFF, 0U,   0U   },
    { LED_MODE_STEADY, LED_ON,  0U,   0U   },
    { LED_MODE_BLINK,  LED_ON,  500U, 500U },
};

/* ---- KEY2 -> LED2: OFF -> ON -> BLINK(200/800) -> BLINK(800/200) -> OFF ---- */
static const led_effect_t s_led2_effects[] = {
    { LED_MODE_STEADY, LED_OFF, 0U,   0U   },
    { LED_MODE_STEADY, LED_ON,  0U,   0U   },
    { LED_MODE_BLINK,  LED_ON,  200U, 800U },
    { LED_MODE_BLINK,  LED_ON,  800U, 200U },
};

/* ---- KEY3 -> 双色灯: OFF -> GREEN -> RED -> YELLOW -> YELLOW BLINK(500/500) -> OFF ---- */
static const bicolor_effect_t s_bicolor_effects[] = {
    { BICOLOR_OFF,    false, 0U,   0U   },
    { BICOLOR_GREEN,  false, 0U,   0U   },
    { BICOLOR_RED,    false, 0U,   0U   },
    { BICOLOR_YELLOW, false, 0U,   0U   },
    { BICOLOR_YELLOW, true,  500U, 500U },
};

/* ---- 每种输出的 apply：把 index 档效果渲染到具体输出 ---- */

static void led1_apply(void *output, uint8_t index, uint32_t now_ms)
{
    const led_effect_t *e = &s_led1_effects[index];
    if (e->mode == LED_MODE_BLINK) {
        led_set_blink((led_t *)output, e->on_ms, e->off_ms, now_ms);
    } else {
        led_set((led_t *)output, e->steady);
    }
}

static void led2_apply(void *output, uint8_t index, uint32_t now_ms)
{
    const led_effect_t *e = &s_led2_effects[index];
    if (e->mode == LED_MODE_BLINK) {
        led_set_blink((led_t *)output, e->on_ms, e->off_ms, now_ms);
    } else {
        led_set((led_t *)output, e->steady);
    }
}

static void bicolor_apply(void *output, uint8_t index, uint32_t now_ms)
{
    const bicolor_effect_t *e = &s_bicolor_effects[index];
    if (e->blink) {
        bicolor_led_blink((bicolor_led_t *)output, e->color,
                          e->on_ms, e->off_ms, now_ms);
    } else {
        bicolor_led_set((bicolor_led_t *)output, e->color);
    }
}

/* ---- 通用通道状态机 ---- */

static void cycle_init(
    button_cycle_t *self,
    button_t       *button,
    void           *output,
    cycle_apply_fn  apply,
    uint8_t         count,
    uint8_t         start_index,
    uint32_t        now_ms)
{
    assert(button != NULL);
    assert(output != NULL);
    assert(apply != NULL);
    assert(count > 0U);
    assert(start_index < count);

    self->button = button;
    self->output = output;
    self->apply  = apply;
    self->count  = count;
    self->index  = start_index;
    self->apply(output, start_index, now_ms);   /* 写入上电默认档 */
}

static void cycle_tick(button_cycle_t *self, uint32_t now_ms)
{
    if (button_poll(self->button, now_ms) == BUTTON_EVENT_SHORT_PRESS) {
        self->index = (uint8_t)((self->index + 1U) % self->count);
        self->apply(self->output, self->index, now_ms);
    }
}

/* 效果表长度，避免在 app_init 里重复写 sizeof 表达式。 */
#define COUNT_OF(a)  ((uint8_t)(sizeof(a) / sizeof((a)[0])))

void app_init(
    app_t         *self,
    button_t      *key1,
    led_t         *led1,
    button_t      *key2,
    led_t         *led2,
    button_t      *key3,
    bicolor_led_t *bicolor,
    uint32_t       now_ms)
{
    assert(self != NULL);

    /* KEY1 -> LED1：上电默认 = 心跳闪烁(表内 BLINK 档，下标 2)。
     * 首次短按前进到下标 0(OFF)，与规范"循环"一致。 */
    cycle_init(&self->cycles[0], key1, led1, led1_apply,
               COUNT_OF(s_led1_effects), 2U, now_ms);

    /* KEY2 -> LED2：上电默认 = 关闭(下标 0)。 */
    cycle_init(&self->cycles[1], key2, led2, led2_apply,
               COUNT_OF(s_led2_effects), 0U, now_ms);

    /* KEY3 -> 双色灯：上电默认 = 关闭(下标 0)。 */
    cycle_init(&self->cycles[2], key3, bicolor, bicolor_apply,
               COUNT_OF(s_bicolor_effects), 0U, now_ms);
}

void app_tick(app_t *self, uint32_t now_ms)
{
    uint8_t i;

    assert(self != NULL);

    for (i = 0U; i < APP_CYCLE_COUNT; ++i) {
        cycle_tick(&self->cycles[i], now_ms);
    }
}
