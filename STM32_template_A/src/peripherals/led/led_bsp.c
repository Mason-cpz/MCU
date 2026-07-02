/* led_bsp.c
 * LED 板级配置层：硬件描述表 + 平台胶水 + 实例化。
 *
 * bsp.c 不再持有任何 LED 细节，只调本文件暴露的：
 *   led_bsp_init() / led_bsp_tick() / led_bsp_get() / led_bsp_bicolor()
 *
 * 移植/改板（只动本文件）：
 *  - 换引脚、改极性 → 改 s_led_descs
 *  - 换芯片平台     → 改 led_write_stm32 + led_gpio_init（GPIO 时钟/模式配置）
 */
#include "peripherals/led/led_bsp.h"

#include "stm32f1xx_hal.h"   /* GPIOA / GPIOC / GPIO_PIN_x */

/* —— LED 硬件描述表 ——
 * 板级 LED 编号作为下标，提升可读性；加灯 = 在此追加一项。
 * 按规格：LED1=PC0, LED2=PC1, LED3=PC2, LED4=PC3，全部高电平点亮 */
typedef struct {
    GPIO_TypeDef  *port;
    uint16_t       pin;
    led_polarity_t polarity;
} led_desc_t;

static const led_desc_t s_led_descs[BSP_LED_COUNT] = {
    [BSP_LED_1] = { GPIOC, GPIO_PIN_0, LED_ACTIVE_HIGH },
    [BSP_LED_2] = { GPIOC, GPIO_PIN_1, LED_ACTIVE_HIGH },
    [BSP_LED_3] = { GPIOC, GPIO_PIN_2, LED_ACTIVE_HIGH },
    [BSP_LED_4] = { GPIOC, GPIO_PIN_3, LED_ACTIVE_HIGH },
};

/* —— 平台私有上下文 —— */
typedef struct {
    GPIO_TypeDef *port;
    uint16_t      pin;
} led_gpio_ctx_t;

/* —— 运行期实例：led_bsp 自己拥有 —— */
static led_gpio_t      s_gpios[BSP_LED_COUNT];
static led_gpio_ctx_t  s_ctxs[BSP_LED_COUNT];
static led_t           s_leds[BSP_LED_COUNT];
static bicolor_led_t   s_bicolor;   /* LED3 / LED4 组双色灯 */

/* —— 平台 write 胶水：把 led_gpio_t 的 write 落到 STM32 HAL —— */
static void led_write_stm32(void *ctx, bool level)
{
    const led_gpio_ctx_t *p = (const led_gpio_ctx_t *)ctx;
    HAL_GPIO_WritePin(p->port, p->pin, level ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/* 配置 LED 引脚硬件（推挽输出）+ 装平台 ctx，并构造 led_gpio_t。
 * 返回的 led_gpio_t 可直接喂给 led_init。 */
static void led_gpio_init(
    led_gpio_t *gpio, led_gpio_ctx_t *ctx,
    GPIO_TypeDef *port, uint16_t pin, bool initial_level)
{
    GPIO_InitTypeDef io = {0};

    /* 开端口时钟 */
    if (port == GPIOA) { __HAL_RCC_GPIOA_CLK_ENABLE(); }
    else if (port == GPIOB) { __HAL_RCC_GPIOB_CLK_ENABLE(); }
    else if (port == GPIOC) { __HAL_RCC_GPIOC_CLK_ENABLE(); }
    else if (port == GPIOD) { __HAL_RCC_GPIOD_CLK_ENABLE(); }

    /* 先写初始电平，再配置模式，避免上电瞬间出现毛刺 */
    HAL_GPIO_WritePin(port, pin, initial_level ? GPIO_PIN_SET : GPIO_PIN_RESET);

    io.Pin = pin;
    io.Mode = GPIO_MODE_OUTPUT_PP;
    io.Pull = GPIO_NOPULL;
    io.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(port, &io);

    /* 装平台私有 ctx + 绑定 write 回调 */
    ctx->port = port;
    ctx->pin = pin;
    gpio->ctx = ctx;
    gpio->write = led_write_stm32;
}

/* —— 对外接口实现 —— */
void led_bsp_init(void)
{
    uint8_t i;

    for (i = 0U; i < BSP_LED_COUNT; ++i) {
        const bool initial_off = (s_led_descs[i].polarity == LED_ACTIVE_LOW);
        led_gpio_init(&s_gpios[i], &s_ctxs[i],
                      s_led_descs[i].port, s_led_descs[i].pin,
                      initial_off);
        led_init(&s_leds[i], &s_gpios[i], s_led_descs[i].polarity);
    }

    /* 组合双色灯：绿=PC2, 红=PC3 */
    bicolor_led_init(&s_bicolor, &s_leds[BSP_LED_3], &s_leds[BSP_LED_4]);
}

void led_bsp_tick(uint32_t now_ms)
{
    uint8_t i;
    for (i = 0U; i < BSP_LED_COUNT; ++i) {
        led_tick(&s_leds[i], now_ms);
    }
}

led_t *led_bsp_get(led_id_t id)
{
    if (id >= BSP_LED_COUNT) {
        return NULL;
    }
    return &s_leds[id];
}

bicolor_led_t *led_bsp_bicolor(void)
{
    return &s_bicolor;
}
