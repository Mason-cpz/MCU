#include "platform/stm32f1/stm32f1_gpio_port.h"

static bool stm32f1_gpio_read(void *ctx);
static void stm32f1_gpio_write(void *ctx, bool level);

/* 根据端口指针打开对应 GPIO 时钟。 */
static void stm32f1_enable_gpio_clock(GPIO_TypeDef *port)
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

void stm32f1_gpio_pin_init(
    hal_gpio_pin_t *pin,
    stm32f1_gpio_pin_ctx_t *ctx,
    GPIO_TypeDef *port,
    uint16_t gpio_pin,
    bool initial_level)
{
    GPIO_InitTypeDef gpio = {0};

    /* 配置 STM32 硬件：开时钟 → 写初始电平 → 推挽输出 */
    stm32f1_enable_gpio_clock(port);
    HAL_GPIO_WritePin(port, gpio_pin, initial_level ? GPIO_PIN_SET : GPIO_PIN_RESET);

    gpio.Pin = gpio_pin;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(port, &gpio);

    /* 构造 HAL 抽象对象 */
    ctx->port = port;
    ctx->pin = gpio_pin;

    pin->ctx = ctx;
    pin->read = stm32f1_gpio_read;
    pin->write = stm32f1_gpio_write;
}

void stm32f1_gpio_input_init(
    hal_gpio_pin_t *pin,
    stm32f1_gpio_pin_ctx_t *ctx,
    GPIO_TypeDef *port,
    uint16_t gpio_pin,
    bool pull_up)
{
    GPIO_InitTypeDef gpio = {0};

    stm32f1_enable_gpio_clock(port);

    gpio.Pin = gpio_pin;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = pull_up ? GPIO_PULLUP : GPIO_PULLDOWN;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(port, &gpio);

    ctx->port = port;
    ctx->pin = gpio_pin;

    pin->ctx = ctx;
    pin->read = stm32f1_gpio_read;
    pin->write = stm32f1_gpio_write;
}

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
