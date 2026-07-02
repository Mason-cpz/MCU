#ifndef APP_LED_FSM_H
#define APP_LED_FSM_H

#include <stdint.h>

/* LED 状态机层。
 *
 * 本层从 app.c 抽出，专职管理三组灯的状态跳转规则：
 *   KEY1 -> LED1   KEY2 -> LED2   KEY3 -> 双色灯
 * 状态机只管"下一状态是什么 + 应用对应灯效"，
 * 时序参数（on_ms/off_ms）由 led_bsp 的灯效表管理。
 *
 * app.c 只需：按键事件 → 调对应的 fsm_on_keyX()。
 */

/* 上电初始化三组灯到初始状态。
 * 由 app_init 调用一次。 */
void led_fsm_init(uint32_t now_ms);

/* KEY1 触发：LED1 状态机前进一步。 */
void led_fsm_on_key1(uint32_t now_ms);

/* KEY2 触发：LED2 状态机前进一步。 */
void led_fsm_on_key2(uint32_t now_ms);

/* KEY3 触发：双色灯状态机前进一步。 */
void led_fsm_on_key3(uint32_t now_ms);

#endif
