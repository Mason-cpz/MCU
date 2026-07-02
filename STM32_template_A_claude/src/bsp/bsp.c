/* bsp.c
 * 板级支持包组合根。持有全部硬件与驱动实例，对外只暴露 bsp_init / bsp_tick。
 * 加外设或改硬件映射：只改本文件的描述表；main.c 与 app 逻辑都不用动。
 *
 * 本文件只用平台无关门面(platform_gpio.h / platform_timebase.h)与中性端口号，
 * 不出现任何 HAL_* / GPIOx 厂商符号 —— 迁移 GD32 只换 platform 实现，bsp 不动。
 *
 * 硬件映射(见 KEY_LED_FRAMEWORK_SPEC.md)：
 *   KEY1 PA0 上升沿 / KEY2 PA1 下降沿 / KEY3 PA4 下降沿
 *   LED1 PC0 / LED2 PC1 / LED3 PC2(绿) / LED4 PC3(红)，LED3+LED4 = 黄色
 */
#include "bsp/bsp.h"

#include "app/app.h"
#include "drivers/led/bicolor_led.h"
#include "drivers/key/button.h"
#include "drivers/led/led.h"
#include "platform/platform_gpio.h"
#include "platform/platform_timebase.h"

/* ------------------------------------------------------------------ LED 表 */

/* 板级 LED 编号：作为 s_leds[] 的下标。
 * 加灯 = 在此追加一项 + 同步给 s_led_descs[] 补一行。 */
enum {
    BSP_LED1,          /* PC0 独立 LED            */
    BSP_LED2,          /* PC1 独立 LED            */
    BSP_LED3_GREEN,    /* PC2 双色灯绿色通道       */
    BSP_LED4_RED,      /* PC3 双色灯红色通道       */
    BSP_LED_COUNT,
};

/* LED 硬件描述：端口、引脚、有效电平。下标与 BSP_LEDx 一一对应。
 * spec 定义 LED 高电平点亮 → ACTIVE_HIGH。若换共阳板，只改这一列。 */
typedef struct {
    port_id_t      port;
    uint8_t        pin;
    led_polarity_t polarity;
} led_desc_t;

static const led_desc_t s_led_descs[BSP_LED_COUNT] = {
    [BSP_LED1]       = { PORT_C, 0U, LED_ACTIVE_HIGH },
    [BSP_LED2]       = { PORT_C, 1U, LED_ACTIVE_HIGH },
    [BSP_LED3_GREEN] = { PORT_C, 2U, LED_ACTIVE_HIGH },
    [BSP_LED4_RED]   = { PORT_C, 3U, LED_ACTIVE_HIGH },
};

/* --------------------------------------------------------------- 按键表 */

/* 板级按键编号：作为 s_buttons[] 的下标。 */
enum {
    BSP_KEY1,          /* PA0 上升沿 */
    BSP_KEY2,          /* PA1 下降沿 */
    BSP_KEY3,          /* PA4 下降沿 */
    BSP_KEY_COUNT,
};

/* 按键硬件描述(全部对应 spec 硬件映射表)：
 *   active_low = 按下为低电平(下降沿键)。
 *   pull       = 空闲态内部上下拉，保证未按下时电平稳定。
 * 上升沿键：空闲低 → 下拉 + active_low=false。
 * 下降沿键：空闲高 → 上拉 + active_low=true。 */
typedef struct {
    port_id_t   port;
    uint8_t     pin;
    bool        active_low;
    gpio_pull_t pull;
} button_desc_t;

#define BSP_DEBOUNCE_MS      20U   /* spec: DEBOUNCE_MS            */
#define BSP_SHORT_PRESS_MS   0U    /* 0 = 用驱动默认 1000ms(spec)  */

static const button_desc_t s_button_descs[BSP_KEY_COUNT] = {
    [BSP_KEY1] = { PORT_A, 0U, false, GPIO_PULL_DOWN },  /* 上升沿 */
    [BSP_KEY2] = { PORT_A, 1U, true,  GPIO_PULL_UP   },  /* 下降沿 */
    [BSP_KEY3] = { PORT_A, 4U, true,  GPIO_PULL_UP   },  /* 下降沿 */
};

/* ---------------------------------------------------------- 运行期实例 */

static hal_gpio_pin_t s_led_pins[BSP_LED_COUNT];
static led_t          s_leds[BSP_LED_COUNT];

static hal_gpio_pin_t s_key_pins[BSP_KEY_COUNT];
static button_t       s_buttons[BSP_KEY_COUNT];

static bicolor_led_t  s_bicolor;   /* LED3 绿 + LED4 红 = 双色灯 */
static hal_timebase_t s_timebase;
static app_t          s_app;

/* bsp_tick 节流：spec 建议 tick ≤ 10ms，这里按 1ms 服务一次即可。 */
#define BSP_TICK_PERIOD_MS   1U
static uint32_t s_last_service_ms;
static bool     s_service_started;

/* 配好所有 LED 的硬件方向与驱动实例(初始全灭)。 */
static void bsp_leds_init(void)
{
    uint8_t i;

    for (i = 0U; i < BSP_LED_COUNT; ++i) {
        /* 初始电平取"灭"：ACTIVE_HIGH→低，ACTIVE_LOW→高 */
        const bool initial_off = (s_led_descs[i].polarity == LED_ACTIVE_LOW);
        platform_gpio_init_output(&s_led_pins[i],
                                  s_led_descs[i].port, s_led_descs[i].pin,
                                  initial_off);
        led_init(&s_leds[i], &s_led_pins[i], s_led_descs[i].polarity);
    }
}

/* 配好所有按键的输入方向、上下拉与驱动实例(含消抖/短按参数)。 */
static void bsp_buttons_init(void)
{
    uint8_t i;

    for (i = 0U; i < BSP_KEY_COUNT; ++i) {
        platform_gpio_init_input(&s_key_pins[i],
                                 s_button_descs[i].port, s_button_descs[i].pin,
                                 s_button_descs[i].pull);
        button_init(&s_buttons[i], &s_key_pins[i],
                    s_button_descs[i].active_low,
                    BSP_DEBOUNCE_MS, BSP_SHORT_PRESS_MS);
    }
}

void bsp_init(void)
{
    uint32_t now_ms;

    platform_timebase_init(&s_timebase);
    now_ms = hal_millis(&s_timebase);

    bsp_leds_init();
    bsp_buttons_init();

    /* LED3(绿) + LED4(红) 组成双色灯 */
    bicolor_led_init(&s_bicolor, &s_leds[BSP_LED3_GREEN], &s_leds[BSP_LED4_RED]);

    /* app 只拿引用并写入上电默认态，Demo 行为都在 app 层的效果表里。 */
    app_init(&s_app,
             &s_buttons[BSP_KEY1], &s_leds[BSP_LED1],
             &s_buttons[BSP_KEY2], &s_leds[BSP_LED2],
             &s_buttons[BSP_KEY3], &s_bicolor,
             now_ms);

    s_last_service_ms = now_ms;
    s_service_started = true;
}

void bsp_tick(void)
{
    uint8_t        i;
    const uint32_t now_ms = hal_millis(&s_timebase);

    /* 节流：同一毫秒内多次调用只服务一次，稳定 tick 语义。 */
    if (s_service_started &&
        (uint32_t)(now_ms - s_last_service_ms) < BSP_TICK_PERIOD_MS) {
        return;
    }
    s_last_service_ms = now_ms;

    /* 先跑应用状态机(含按键轮询与消抖)，再推进 LED 非阻塞闪烁。 */
    app_tick(&s_app, now_ms);

    for (i = 0U; i < BSP_LED_COUNT; ++i) {
        led_tick(&s_leds[i], now_ms);
    }
}
