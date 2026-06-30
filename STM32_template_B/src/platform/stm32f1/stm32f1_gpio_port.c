#include "platform/stm32f1/stm32f1_gpio_port.h"

static bool stm32f1_gpio_read(void *ctx)
{
    const stm32f1_gpio_pin_ctx_t *pin = (const stm32f1_gpio_pin_ctx_t *)ctx;
    return HAL_GPIO_ReadPin(pin->port, pin->pin) == GPIO_PIN_SET;
}

static void stm32f1_gpio_write(void *ctx, bool level)
{
    const stm32f1_gpio_pin_ctx_t *pin = (const stm32f1_gpio_pin_ctx_t *)ctx;
    HAL_GPIO_WritePin(pin->port, pin->pin, level ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void stm32f1_gpio_pin_init(
    hal_gpio_pin_t *pin,
    stm32f1_gpio_pin_ctx_t *ctx,
    GPIO_TypeDef *port,
    uint16_t gpio_pin)
{
    ctx->port = port;
    ctx->pin = gpio_pin;

    pin->ctx = ctx;
    pin->read = stm32f1_gpio_read;
    pin->write = stm32f1_gpio_write;
}
