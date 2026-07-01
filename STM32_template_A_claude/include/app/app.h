#ifndef APP_APP_H
#define APP_APP_H

/* 应用层：只消费按键事件，驱动 LED 状态机，承载 Demo 行为。
 *
 * 设计要点（对应框架扩展目标）：
 * - 每个通道 = 一个按键 + 一个"渲染函数" + 一个循环长度。
 * - 按键每产生一次 SHORT_PRESS，就在 [0,count) 里循环前进一格，
 *   并调用该通道的 apply(index) 把对应效果渲染到输出。
 * - 具体输出类型(单 LED / 双色灯)与效果表都藏在 app.c 里，
 *   通过 apply 回调解耦：app 核心状态机只认 index，不认输出类型。
 * - 闪烁由 led_tick / bsp_tick 非阻塞推进，app 只负责切换模式。
 * - 加按键 / 加状态 = 加一张表 + 一个 apply + 一个通道，不改状态机逻辑。
 */
#include "drivers/bicolor_led.h"
#include "drivers/button.h"
#include "drivers/led.h"

#include <stdint.h>

/* 单 LED 的一种效果：常亮/关闭走 steady，闪烁走 on_ms/off_ms。 */
typedef struct {
    led_mode_t  mode;    /* LED_MODE_STEADY / LED_MODE_BLINK */
    led_state_t steady;  /* mode == STEADY 时的目标电平       */
    uint16_t    on_ms;   /* mode == BLINK 时的点亮时长        */
    uint16_t    off_ms;  /* mode == BLINK 时的熄灭时长        */
} led_effect_t;

/* 双色灯的一种效果：静态颜色，或以该颜色闪烁。 */
typedef struct {
    bicolor_color_t color;
    bool            blink;
    uint16_t        on_ms;
    uint16_t        off_ms;
} bicolor_effect_t;

/* 把第 index 档效果渲染到某个输出。output 的具体类型由该 apply 自己清楚。 */
typedef void (*cycle_apply_fn)(void *output, uint8_t index, uint32_t now_ms);

/* 通用"按键循环通道"：一个按键在 [0,count) 里循环推进并渲染。
 * 不含输出类型信息，加新输出类型也不用改本结构与状态机。 */
typedef struct {
    button_t       *button;
    void           *output;   /* led_t* / bicolor_led_t* ... 由 apply 解释 */
    cycle_apply_fn  apply;
    uint8_t         count;    /* 循环长度 */
    uint8_t         index;    /* 当前档位 */
} button_cycle_t;

/* Demo 通道数量。加通道 = 加表 + 加 apply + 这里 +1，并在 app_init 里配置。 */
#define APP_CYCLE_COUNT  3U

typedef struct {
    button_cycle_t cycles[APP_CYCLE_COUNT];
} app_t;

/* 组合根传入已初始化好的按键与 LED 实例；app 只持有引用并写入初始效果。 */
void app_init(
    app_t         *self,
    button_t      *key1,
    led_t         *led1,
    button_t      *key2,
    led_t         *led2,
    button_t      *key3,
    bicolor_led_t *bicolor,
    uint32_t       now_ms);

/* 轮询各按键并推进各自状态机(闪烁推进由 led_tick 负责)。 */
void app_tick(app_t *self, uint32_t now_ms);

#endif
