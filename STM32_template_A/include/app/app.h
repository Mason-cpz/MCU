#ifndef APP_APP_H
#define APP_APP_H

/* 应用层：消费按键事件，驱动 LED 状态机。
 *
 * 这里实现 Demo 行为（KEY_LED_FRAMEWORK_SPEC.md 的"推荐 Demo 行为"）：
 *   上电：LED1 1Hz 心跳闪烁，LED2/双色灯关闭。
 *   KEY1 控制 LED1：OFF -> ON -> BLINK(500/500) -> OFF 循环。
 *   KEY2 控制 LED2：OFF -> ON -> BLINK(200/800) -> BLINK(800/200) -> OFF 循环。
 *   KEY3 控制双色灯：OFF -> GREEN -> RED -> YELLOW -> YELLOW BLINK(500/500) -> OFF 循环。
 *
 * 框架扩展方向：新增按键/灯效/状态只在本文件改状态机与计数，
 * 不触碰 bsp/drivers/hal。
 */
void app_init(void);

/* 周期调用：在主循环里推进，消费按键事件、刷新灯效。 */
void app_tick(void);

#endif
