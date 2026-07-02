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

/* 初始化所有 LED 硬件 + 驱动实例 + 双色灯组合。
 * 由 bsp.c 在 bsp_init 里调用一次。 */
void led_bsp_init(void);

/* 周期推进：LED 闪烁（非阻塞）。
 * 由 bsp.c 在 bsp_tick 里调用。 */
void led_bsp_tick(uint32_t now_ms);

/* 按 id 取驱动实例（越界返回 NULL） */
led_t *led_bsp_get(led_id_t id);

/* 取双色灯实例（LED3 绿 + LED4 红） */
bicolor_led_t *led_bsp_bicolor(void);

#endif
