#ifndef PLATFORM_STM32F1_BOARD_H
#define PLATFORM_STM32F1_BOARD_H

#include "drivers/led.h"
#include "hal/gpio.h"
#include "hal/timebase.h"
#include "platform/stm32f1/stm32f1_gpio_port.h"

#include <stdint.h>

enum {
    STM32F1_BOARD_LED_MAX = 8,
};

typedef struct {
    GPIO_TypeDef *port;
    uint16_t pin;
    led_polarity_t polarity;
} stm32f1_board_led_desc_t;

typedef struct {
    stm32f1_gpio_pin_ctx_t ctx[STM32F1_BOARD_LED_MAX];
    hal_gpio_pin_t pins[STM32F1_BOARD_LED_MAX];
    led_polarity_t polarities[STM32F1_BOARD_LED_MAX];
    uint8_t count;
    hal_timebase_t timebase;
} stm32f1_board_t;

void stm32f1_board_init(
    stm32f1_board_t *self,
    const stm32f1_board_led_desc_t *descs,
    uint8_t count);

uint8_t stm32f1_board_led_count(const stm32f1_board_t *self);
const hal_gpio_pin_t *stm32f1_board_led_pin(const stm32f1_board_t *self, uint8_t index);
const hal_gpio_pin_t *stm32f1_board_led_pins(const stm32f1_board_t *self);
const led_polarity_t *stm32f1_board_led_polarities(const stm32f1_board_t *self);
const hal_timebase_t *stm32f1_board_timebase(const stm32f1_board_t *self);

#endif
