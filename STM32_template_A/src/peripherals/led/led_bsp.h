#ifndef PERIPHERALS_LED_LED_BSP_H
#define PERIPHERALS_LED_LED_BSP_H

#include "peripherals/led/led.h"
#include "peripherals/led/bicolor_led.h"

#include <stdint.h>

/* LED 板级配置层。
 *
 * 本文件把"硬件描述表 + 平台胶水 + 实例化"集中在一起，
 * 让 bsp.c 不再关心 LED 的任何细节，只调 led_bsp_init / led_bsp_get。
 *
 * 移植/改板：
 *  - 换引脚、改极性：改 led_bsp.c 里的 s_led_descs 表
 *  - 换芯片平台：改 led_bsp.c 里的 led_write 平台回调 + led_gpio_init
 */

/* 板级 LED 编号（作为数组下标） */
typedef enum {
    BSP_LED_1 = 0,  /* PC0 */
    BSP_LED_2,      /* PC1 */
    BSP_LED_3,      /* PC2 双色-绿 */
    BSP_LED_4,      /* PC3 双色-红 */
    BSP_LED_COUNT,
} led_id_t;

/* —— 单灯灯效枚举 ——
 * 时序参数（on_ms/off_ms）集中在本层管理，app 只传枚举值。
 * 加新灯效 = 在此加一项 + led_bsp.c 的 s_effect_timing 表加一行。 */
typedef enum {
    LE_OFF = 0,
    LE_ON,
    LE_BLINK_HEARTBEAT,  /* 500/500  1Hz 心跳      */
    LE_BLINK_FAST,       /* 200/800  快闪          */
    LE_BLINK_SLOW,       /* 800/200  慢闪          */
    LE_COUNT,
} led_effect_t;

/* —— 双色灯灯效枚举 —— */
typedef enum {
    BE_OFF = 0,
    BE_GREEN,
    BE_RED,
    BE_YELLOW,
    BE_GREEN_BLINK,      /* 500/500 */
    BE_RED_BLINK,
    BE_YELLOW_BLINK,
    BE_COUNT,
} bicolor_effect_t;

/* 初始化所有 LED 硬件 + 驱动实例 + 双色灯组合。
 * 由 bsp.c 在 bsp_init 里调用一次。 */
void led_bsp_init(void);

/* 周期推进：LED 闪烁（非阻塞）。
 * 由 bsp.c 在 bsp_tick 里调用。 */
void led_bsp_tick(uint32_t now_ms);

/* 按 id 取驱动实例（越界返回 NULL）。
 * 保留给特殊场景用；常规调用建议用 led_bsp_apply。 */
led_t *led_bsp_get(led_id_t id);

/* 取双色灯实例（LED3 绿 + LED4 红） */
bicolor_led_t *led_bsp_bicolor(void);

/* —— 灯效驱动：app 传枚举，本层负责翻译成 led_set/led_set_blink —— */

/* 应用灯效到单灯 */
void led_bsp_apply(led_id_t id, led_effect_t e, uint32_t now_ms);

/* 应用灯效到双色灯 */
void led_bsp_bicolor_apply(bicolor_effect_t e, uint32_t now_ms);

#endif
