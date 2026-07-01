#ifndef PLATFORM_STM32F1_GPIO_PORT_H
#define PLATFORM_STM32F1_GPIO_PORT_H

#include "hal/gpio.h"

#include "stm32f1xx_hal.h"

#include <stdbool.h>
#include <stdint.h>

/* STM32F1 GPIO 端口适配。
 * 把 STM32 的 (port, pin) 配成推挽输出，并构造为通用 hal_gpio_pin_t 抽象。
 * initial_level 为 true 表示初始高电平，false 表示初始低电平。
 */
typedef struct {
    GPIO_TypeDef *port;
    uint16_t pin;
} stm32f1_gpio_pin_ctx_t;

void stm32f1_gpio_pin_init(
    hal_gpio_pin_t *pin,
    stm32f1_gpio_pin_ctx_t *ctx,
    GPIO_TypeDef *port,
    uint16_t gpio_pin,
    bool initial_level);

#endif
