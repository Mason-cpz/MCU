#ifndef PERIPHERALS_LED_LED_H
#define PERIPHERALS_LED_LED_H

#include <stdint.h>

/* LED 驱动（2 层架构：driver + platform 注入）。
 *
 * 本层不依赖任何具体 HAL，只通过 led_gpio_t 这个最小接口操作引脚；
 * 接口由平台层（如 led_bsp.c）在初始化时注入。
 * led_init 假设引脚已由平台层配置为推挽输出，本层只负责按逻辑状态写电平。
 *
 * 设计要点：
 *  - 接口最小化：LED 只需要 write，不需要 read。
 *  - 平台无关：换 MCU 只改 led_bsp 里的注入胶水，本文件不动。
 */

/* LED 用到的最小 GPIO 接口：只要写。 */
typedef struct {
    void *ctx;                       /* 平台私有上下文 */
    void (*write)(void *ctx, bool level);
} led_gpio_t;

typedef enum {
    LED_OFF = 0,
    LED_ON = 1,
} led_state_t;

typedef enum {
    LED_ACTIVE_LOW = 0,
    LED_ACTIVE_HIGH = 1,
} led_polarity_t;

typedef enum {
    LED_MODE_STEADY = 0,
    LED_MODE_BLINK = 1,
} led_mode_t;

typedef struct {
    const led_gpio_t *gpio;
    led_polarity_t polarity;
    led_mode_t mode;
    led_state_t state;
    uint16_t on_ms;
    uint16_t off_ms;
    uint32_t next_toggle_ms;
} led_t;

void led_init(led_t *self, const led_gpio_t *gpio, led_polarity_t polarity);
void led_set(led_t *self, led_state_t state);
void led_toggle(led_t *self);
void led_set_blink(led_t *self, uint16_t on_ms, uint16_t off_ms, uint32_t now_ms);
void led_tick(led_t *self, uint32_t now_ms);
led_state_t led_get_state(const led_t *self);

#endif
