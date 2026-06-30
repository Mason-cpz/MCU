#include "platform/stm32f1_board.h"

#include <assert.h>
#include <stddef.h>

#include "platform/stm32f1/stm32f1_timebase.h"

/* 根据端口指针打开对应 GPIO 时钟，便于板级配置复用。 */
static void stm32f1_board_enable_gpio_clock(GPIO_TypeDef *port)
{
    if (port == GPIOA) {
        __HAL_RCC_GPIOA_CLK_ENABLE();
        return;
    }

    if (port == GPIOB) {
        __HAL_RCC_GPIOB_CLK_ENABLE();
        return;
    }

    if (port == GPIOC) {
        __HAL_RCC_GPIOC_CLK_ENABLE();
        return;
    }

    if (port == GPIOD) {
        __HAL_RCC_GPIOD_CLK_ENABLE();
        return;
    }

#if defined(GPIOE)
    if (port == GPIOE) {
        __HAL_RCC_GPIOE_CLK_ENABLE();
        return;
    }
#endif
}

static void stm32f1_board_gpio_output_init(
    GPIO_TypeDef *port,
    uint16_t pin,
    led_polarity_t polarity)
{
    GPIO_InitTypeDef gpio = {0};
    const GPIO_PinState off_level =
        polarity == LED_ACTIVE_HIGH ? GPIO_PIN_RESET : GPIO_PIN_SET;

    stm32f1_board_enable_gpio_clock(port);
    HAL_GPIO_WritePin(port, pin, off_level);

    gpio.Pin = pin;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(port, &gpio);
}

static void stm32f1_board_init_led(
    hal_gpio_pin_t *pin,
    stm32f1_gpio_pin_ctx_t *ctx,
    GPIO_TypeDef *port,
    uint16_t gpio_pin,
    led_polarity_t polarity)
{
    stm32f1_gpio_pin_init(pin, ctx, port, gpio_pin);
    stm32f1_board_gpio_output_init(port, gpio_pin, polarity);
}

void stm32f1_board_init(
    stm32f1_board_t *self,
    const stm32f1_board_led_desc_t *descs,
    uint8_t count)
{
    uint8_t i;

    assert(self != NULL);
    assert(descs != NULL);
    assert(count <= STM32F1_BOARD_LED_MAX);

    self->count = count;
    if (self->count > STM32F1_BOARD_LED_MAX) {
        self->count = STM32F1_BOARD_LED_MAX;
    }

    for (i = 0U; i < self->count; ++i) {
        stm32f1_board_init_led(
            &self->pins[i],
            &self->ctx[i],
            descs[i].port,
            descs[i].pin,
            descs[i].polarity);
        self->polarities[i] = descs[i].polarity;
    }

    stm32f1_timebase_init(&self->timebase);
}

uint8_t stm32f1_board_led_count(const stm32f1_board_t *self)
{
    assert(self != NULL);

    return self->count;
}

const hal_gpio_pin_t *stm32f1_board_led_pin(const stm32f1_board_t *self, uint8_t index)
{
    assert(self != NULL);
    assert(index < self->count);

    return &self->pins[index];
}

const hal_gpio_pin_t *stm32f1_board_led_pins(const stm32f1_board_t *self)
{
    assert(self != NULL);

    return self->pins;
}

const led_polarity_t *stm32f1_board_led_polarities(const stm32f1_board_t *self)
{
    assert(self != NULL);

    return self->polarities;
}

const hal_timebase_t *stm32f1_board_timebase(const stm32f1_board_t *self)
{
    return &self->timebase;
}
