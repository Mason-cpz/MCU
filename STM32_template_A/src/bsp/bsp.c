/* bsp.c
 * 板级支持包组合根。持有所有硬件和驱动实例，对外只暴露 bsp_init / bsp_tick。
 * 加外设或改行为：在本文件的描述表或 bsp_init 里修改，main.c 不用动。
 */
#include "bsp/bsp.h"

#include "drivers/bicolor_led.h"
#include "drivers/led.h"
#include "platform/stm32f1/stm32f1_gpio_port.h"
#include "platform/stm32f1/stm32f1_timebase.h"
#include "stm32f1xx_hal.h"   /* GPIOA / GPIO_PIN_x */

/* 板级 LED 编号：作为 s_leds[] 的下标，提升行为配置处的可读性。
 * 加灯 = 在此追加一项 + 同步更新 s_led_descs[]。 */
enum {
    BSP_LED_0,
    BSP_LED_1,
    BSP_LED_2,
    BSP_LED_3,
    BSP_LED_COUNT,
};

/* 板级 LED 硬件描述：端口、引脚、极性。
 * 下标与 BSP_LED_x 一一对应。 */
typedef struct {
    GPIO_TypeDef  *port;
    uint16_t       pin;
    led_polarity_t polarity;
} led_desc_t;

static const led_desc_t s_led_descs[BSP_LED_COUNT] = {
    [BSP_LED_0] = { GPIOA, GPIO_PIN_1, LED_ACTIVE_HIGH },
    [BSP_LED_1] = { GPIOA, GPIO_PIN_2, LED_ACTIVE_HIGH },
    [BSP_LED_2] = { GPIOA, GPIO_PIN_3, LED_ACTIVE_HIGH },
    [BSP_LED_3] = { GPIOA, GPIO_PIN_4, LED_ACTIVE_HIGH },
};

/* 运行期实例：bsp 自己拥有 */
static hal_gpio_pin_t s_led_pins[BSP_LED_COUNT];
static stm32f1_gpio_pin_ctx_t s_led_ctx[BSP_LED_COUNT];
static led_t           s_leds[BSP_LED_COUNT];
static bicolor_led_t   s_bicolor;  /* LED2 / LED3 组双色灯 */
static hal_timebase_t  s_timebase;

/* 一次性配好所有 LED 的硬件 + 驱动实例 */
static void bsp_leds_init(void)
{
    uint8_t i;

    for (i = 0U; i < BSP_LED_COUNT; ++i) {
        /* 初始电平按极性取"灭"状态：ACTIVE_HIGH→低，ACTIVE_LOW→高 */
        const bool initial_off = (s_led_descs[i].polarity == LED_ACTIVE_LOW);
        stm32f1_gpio_pin_init(&s_led_pins[i], &s_led_ctx[i],
                              s_led_descs[i].port, s_led_descs[i].pin,
                              initial_off);
        led_init(&s_leds[i], &s_led_pins[i], s_led_descs[i].polarity);
    }
}

/* 配置各 LED 的初始行为（常亮/闪烁/双色等） */
static void bsp_leds_configure(void)
{
    const uint32_t now_ms = hal_millis(&s_timebase);

    led_set(&s_leds[BSP_LED_0], LED_ON);                      /* 常亮      */
    led_set_blink(&s_leds[BSP_LED_1], 500U, 500U, now_ms);    /* 500ms 闪烁 */

    /* LED2 / LED3 组成双色灯，初始黄色 500ms 闪烁 */
    bicolor_led_init(&s_bicolor, &s_leds[BSP_LED_2], &s_leds[BSP_LED_3]);
    bicolor_led_blink(&s_bicolor, BICOLOR_YELLOW, 500U, 500U, now_ms);
}

void bsp_init(void)
{
    /* 初始化时基 */
    stm32f1_timebase_init(&s_timebase);

    /* 初始化所有 LED 硬件与驱动实例，再配置初始行为 */
    bsp_leds_init();
    bsp_leds_configure();
}

void bsp_tick(void)
{
    uint8_t        i;
    const uint32_t now_ms = hal_millis(&s_timebase);

    for (i = 0U; i < BSP_LED_COUNT; ++i) {
        led_tick(&s_leds[i], now_ms);
    }
}
