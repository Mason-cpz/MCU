#ifndef DRIVERS_LED_H
#define DRIVERS_LED_H

#include "hal/gpio.h"

#include <stdint.h>

/* LED 驱动。
 * led_init 假设引脚已经由平台层配置为推挽输出；
 * 本层只负责按逻辑状态写电平，不负责方向配置。
 */
typedef enum {
    LED_OFF = 0,
    LED_ON = 1,
} led_state_t;

typedef enum {
    LED_ACTIVE_LOW = 0,
    LED_ACTIVE_HIGH = 1,
} led_polarity_t;

typedef struct {
    const hal_gpio_pin_t *pin;
    led_polarity_t polarity;
    led_state_t state;
} led_t;

void led_init(led_t *self, const hal_gpio_pin_t *pin, led_polarity_t polarity);
void led_set(led_t *self, led_state_t state);
void led_toggle(led_t *self);
led_state_t led_is_on(const led_t *self);

#endif
