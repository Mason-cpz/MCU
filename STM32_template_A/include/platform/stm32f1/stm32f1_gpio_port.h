#ifndef PLATFORM_STM32F1_GPIO_PORT_H
#define PLATFORM_STM32F1_GPIO_PORT_H

#include "hal/gpio.h"

#include "stm32f1xx_hal.h"

#include <stdint.h>

/* STM32F1 GPIO 端口适配。
 * 只负责把 HAL 引脚转成通用 pin 抽象。
 */
typedef struct {
    GPIO_TypeDef *port;
    uint16_t pin;
} stm32f1_gpio_pin_ctx_t;

void stm32f1_gpio_pin_init(
    hal_gpio_pin_t *pin,
    stm32f1_gpio_pin_ctx_t *ctx,
    GPIO_TypeDef *port,
    uint16_t gpio_pin);

#endif
