/* stm32f1_gpio_port.c
 * platform_gpio.h 的 STM32F1 实现（唯一出现 HAL_* / GPIOx 的 GPIO 层）。
 *
 * 上层只给中性的 (port_id, pin_num)：
 *   - 本文件把 port_id 映射到 GPIO_TypeDef* 并开对应时钟；
 *   - pin_num 映射到 GPIO_PIN_x；
 *   - 引脚上下文用文件内静态池管理，上层无需分配、也看不到厂商类型。
 * 迁移 GD32 时只需另写一份实现同名门面的源文件，bsp/drivers/app 不动。
 */
#include "platform/platform_gpio.h"

#include "stm32f1xx_hal.h"

#include <assert.h>
#include <stddef.h>

/* 厂商相关的引脚上下文：只在本文件可见。 */
typedef struct {
    GPIO_TypeDef *port;
    uint16_t      pin;
} stm32f1_pin_ctx_t;

/* 静态上下文池：容量 = 可用引脚数上限。用尽则拒绝(返回时 pin 不可用)。
 * 池大小按“本工程实际用到的引脚数”留裕量，加引脚只需调大这个数。 */
#define STM32F1_PIN_POOL_SIZE  16U
static stm32f1_pin_ctx_t s_pin_pool[STM32F1_PIN_POOL_SIZE];
static uint8_t           s_pin_used;

static bool stm32f1_gpio_read(void *ctx)
{
    const stm32f1_pin_ctx_t *c = (const stm32f1_pin_ctx_t *)ctx;
    return HAL_GPIO_ReadPin(c->port, c->pin) == GPIO_PIN_SET;
}

static void stm32f1_gpio_write(void *ctx, bool level)
{
    const stm32f1_pin_ctx_t *c = (const stm32f1_pin_ctx_t *)ctx;
    HAL_GPIO_WritePin(c->port, c->pin, level ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/* 中性端口号 -> STM32 端口指针，并按需开时钟。未知端口返回 NULL。 */
static GPIO_TypeDef *resolve_port(port_id_t port)
{
    switch (port) {
    case PORT_A: __HAL_RCC_GPIOA_CLK_ENABLE(); return GPIOA;
    case PORT_B: __HAL_RCC_GPIOB_CLK_ENABLE(); return GPIOB;
    case PORT_C: __HAL_RCC_GPIOC_CLK_ENABLE(); return GPIOC;
    case PORT_D: __HAL_RCC_GPIOD_CLK_ENABLE(); return GPIOD;
#if defined(GPIOE)
    case PORT_E: __HAL_RCC_GPIOE_CLK_ENABLE(); return GPIOE;
#endif
    default:     return NULL;
    }
}

/* 从静态池取一个上下文并填好端口/引脚；池满或端口非法返回 NULL。 */
static stm32f1_pin_ctx_t *alloc_ctx(port_id_t port, uint8_t pin_num)
{
    GPIO_TypeDef      *gpio;
    stm32f1_pin_ctx_t *ctx;

    if (pin_num > 15U || s_pin_used >= STM32F1_PIN_POOL_SIZE) {
        return NULL;
    }
    gpio = resolve_port(port);
    if (gpio == NULL) {
        return NULL;
    }

    ctx = &s_pin_pool[s_pin_used++];
    ctx->port = gpio;
    ctx->pin  = (uint16_t)(1U << pin_num);
    return ctx;
}

static void bind_pin(hal_gpio_pin_t *pin, stm32f1_pin_ctx_t *ctx)
{
    pin->ctx   = ctx;
    pin->read  = stm32f1_gpio_read;
    pin->write = stm32f1_gpio_write;
}

void platform_gpio_init_output(
    hal_gpio_pin_t *pin,
    port_id_t       port,
    uint8_t         pin_num,
    bool            initial_level)
{
    GPIO_InitTypeDef   gpio = {0};
    stm32f1_pin_ctx_t *ctx  = alloc_ctx(port, pin_num);

    /* 配置错误(端口非法/引脚越界/池用尽)属编程期问题，直接断言暴露，
     * 不静默返回 —— 否则后续 hal_gpio_* 会解引用空指针崩在别处。 */
    assert(ctx != NULL);

    /* 先写初始电平，再切输出，避免上电毛刺 */
    HAL_GPIO_WritePin(ctx->port, ctx->pin,
                      initial_level ? GPIO_PIN_SET : GPIO_PIN_RESET);

    gpio.Pin   = ctx->pin;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(ctx->port, &gpio);

    bind_pin(pin, ctx);
}

void platform_gpio_init_input(
    hal_gpio_pin_t *pin,
    port_id_t       port,
    uint8_t         pin_num,
    gpio_pull_t     pull)
{
    GPIO_InitTypeDef   gpio = {0};
    stm32f1_pin_ctx_t *ctx  = alloc_ctx(port, pin_num);

    assert(ctx != NULL);

    gpio.Pin  = ctx->pin;
    gpio.Mode = GPIO_MODE_INPUT;
    switch (pull) {
    case GPIO_PULL_UP:   gpio.Pull = GPIO_PULLUP;   break;
    case GPIO_PULL_DOWN: gpio.Pull = GPIO_PULLDOWN; break;
    case GPIO_PULL_NONE:
    default:             gpio.Pull = GPIO_NOPULL;   break;
    }
    HAL_GPIO_Init(ctx->port, &gpio);

    bind_pin(pin, ctx);
}
