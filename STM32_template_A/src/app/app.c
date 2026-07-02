/* app.c
 * 应用层：纯事件路由。
 *
 * 职责：从 bsp 收按键事件，转发给对应的状态机（led_fsm）。
 * 不持有任何状态枚举、灯效表、时序参数——这些都在 led_fsm / led_bsp。
 *
 * 加新外设/事件：在本文件加一行路由，不碰 fsm/bsp/drivers。
 */
#include "app/app.h"

#include "bsp/bsp.h"
#include "app/led_fsm.h"
#include "drivers/button.h"

#include <stddef.h>   /* NULL */

void app_init(void)
{
    led_fsm_init(bsp_now_ms());
}

void app_tick(void)
{
    const uint32_t now_ms = bsp_now_ms();
    button_t *key;

    /* KEY1 -> LED1 状态机 */
    key = bsp_key(BSP_KEY_1);
    if (key != NULL && button_poll(key, now_ms) != BUTTON_EVENT_NONE) {
        led_fsm_on_key1(now_ms);
    }

    /* KEY2 -> LED2 状态机 */
    key = bsp_key(BSP_KEY_2);
    if (key != NULL && button_poll(key, now_ms) != BUTTON_EVENT_NONE) {
        led_fsm_on_key2(now_ms);
    }

    /* KEY3 -> 双色灯状态机 */
    key = bsp_key(BSP_KEY_3);
    if (key != NULL && button_poll(key, now_ms) != BUTTON_EVENT_NONE) {
        led_fsm_on_key3(now_ms);
    }
}
