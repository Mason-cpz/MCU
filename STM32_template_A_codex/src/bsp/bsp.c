/* bsp.c
 * 板级支持包组合根。
 * 只负责硬件映射与驱动实例，Demo 逻辑放到 app 层。
 */
#include "bsp/bsp.h"

#include "app/app.h"
#include "drivers/bicolor_led.h"
#include "drivers/button.h"
#include "drivers/led.h"
#include "platform/stm32f1/stm32f1_gpio_port.h"
#include "platform/stm32f1/stm32f1_timebase.h"

typedef enum {
    BSP_LED1 = 0U,
    BSP_LED2,
    BSP_LED3,
    BSP_LED4,
    BSP_LED_COUNT,
} bsp_led_id_t;

typedef enum {
    BSP_KEY1 = 0U,
    BSP_KEY2,
    BSP_KEY3,
    BSP_KEY_COUNT,
} bsp_key_id_t;

enum {
    BSP_BUTTON_DEBOUNCE_MS = 20U,
};

typedef struct {
    GPIO_TypeDef *port;
    uint16_t pin;
    led_polarity_t polarity;
} led_desc_t;

typedef struct {
    GPIO_TypeDef *port;
    uint16_t pin;
    bool active_low;
    button_edge_t edge;
    bool pull_up;
} key_desc_t;

static const led_desc_t s_led_descs[BSP_LED_COUNT] = {
    [BSP_LED1] = { GPIOC, GPIO_PIN_0, LED_ACTIVE_HIGH },
    [BSP_LED2] = { GPIOC, GPIO_PIN_1, LED_ACTIVE_HIGH },
    [BSP_LED3] = { GPIOC, GPIO_PIN_2, LED_ACTIVE_HIGH },
    [BSP_LED4] = { GPIOC, GPIO_PIN_3, LED_ACTIVE_HIGH },
};

static const key_desc_t s_key_descs[BSP_KEY_COUNT] = {
    [BSP_KEY1] = { GPIOA, GPIO_PIN_0, false, BUTTON_EDGE_RISING, false },
    [BSP_KEY2] = { GPIOA, GPIO_PIN_1, true, BUTTON_EDGE_FALLING, true },
    [BSP_KEY3] = { GPIOA, GPIO_PIN_4, true, BUTTON_EDGE_FALLING, true },
};

static hal_timebase_t s_timebase;
static hal_gpio_pin_t s_led_pins[BSP_LED_COUNT];
static stm32f1_gpio_pin_ctx_t s_led_ctx[BSP_LED_COUNT];
static led_t s_leds[BSP_LED_COUNT];
static hal_gpio_pin_t s_key_pins[BSP_KEY_COUNT];
static stm32f1_gpio_pin_ctx_t s_key_ctx[BSP_KEY_COUNT];
static button_t s_keys[BSP_KEY_COUNT];
static bicolor_led_t s_bicolor;
static app_t s_app;

static void bsp_leds_init(void)
{
    uint8_t i;

    for (i = 0U; i < BSP_LED_COUNT; ++i) {
        const bool initial_off = (s_led_descs[i].polarity == LED_ACTIVE_LOW);
        stm32f1_gpio_pin_init(&s_led_pins[i], &s_led_ctx[i],
                              s_led_descs[i].port, s_led_descs[i].pin,
                              initial_off);
        led_init(&s_leds[i], &s_led_pins[i], s_led_descs[i].polarity);
    }
}

static void bsp_keys_init(void)
{
    uint8_t i;

    for (i = 0U; i < BSP_KEY_COUNT; ++i) {
        stm32f1_gpio_input_init(&s_key_pins[i], &s_key_ctx[i],
                                s_key_descs[i].port, s_key_descs[i].pin,
                                s_key_descs[i].pull_up);
        button_init(&s_keys[i], &s_key_pins[i],
                    s_key_descs[i].active_low,
                    BSP_BUTTON_DEBOUNCE_MS,
                    s_key_descs[i].edge);
    }
}

void bsp_init(void)
{
    stm32f1_timebase_init(&s_timebase);
    const uint32_t now_ms = hal_millis(&s_timebase);
    bsp_leds_init();
    bsp_keys_init();

    bicolor_led_init(&s_bicolor, &s_leds[BSP_LED3], &s_leds[BSP_LED4]);
    app_init(&s_app,
             &s_keys[BSP_KEY1], &s_leds[BSP_LED1],
             &s_keys[BSP_KEY2], &s_leds[BSP_LED2],
             &s_keys[BSP_KEY3], &s_bicolor,
             now_ms);
}

void bsp_tick(void)
{
    uint8_t i;
    const uint32_t now_ms = hal_millis(&s_timebase);

    app_tick(&s_app, now_ms);

    for (i = 0U; i < BSP_LED_COUNT; ++i) {
        led_tick(&s_leds[i], now_ms);
    }
}
