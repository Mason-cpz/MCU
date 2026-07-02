#ifndef BSP_BSP_H
#define BSP_BSP_H

#include "peripherals/led/led_bsp.h"   /* led_t / bicolor_led_t / led_id_t */
#include "drivers/button.h"

#include <stdint.h>

/* 板级支持包：总装配入口。
 * 各外设细节在 peripherals/<name>/<name>_bsp.c，本文件只调 init + 暴露访问器。
 *
 * 三个对外访问器分别暴露 LED1 / LED2 / 双色灯，以及三个按键。
 * app 通过这些访问器消费事件、驱动状态机。 */

/* 板级按键编号 */
typedef enum {
    BSP_KEY_1 = 0,   /* PA0 上升沿 */
    BSP_KEY_2,       /* PA1 下降沿 */
    BSP_KEY_3,       /* PA4 下降沿 */
    BSP_KEY_COUNT,
} bsp_key_id_t;

/* 初始化全部硬件 + 驱动实例 + 初始行为 */
void bsp_init(void);

/* 周期推进：LED 闪烁、按键消抖。
 * 由 main 主循环以几毫秒为节拍调用。 */
void bsp_tick(void);

/* —— 对 app 暴露的访问器 —— */
button_t      *bsp_key(bsp_key_id_t id);
led_t         *bsp_led1(void);     /* PC0 独立 LED */
led_t         *bsp_led2(void);     /* PC1 独立 LED */
bicolor_led_t *bsp_bicolor(void);  /* PC2+PC3 双色灯 */
uint32_t       bsp_now_ms(void);   /* 当前毫秒时间戳 */

#endif
