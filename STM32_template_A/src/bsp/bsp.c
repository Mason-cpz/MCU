/* bsp.c
 * 板级支持包组合根。持有所有硬件和驱动实例，对外暴露 bsp_init/bsp_tick
 * 以及 app 所需的访问器。
 *
 * 加外设或改行为：在本文件的描述表或 init 里修改，main.c 不用动。
 *
 * 硬件映射（KEY_LED_FRAMEWORK_SPEC.md）：
 *   KEY1 = PA0 上升沿   KEY2 = PA1 下降沿   KEY3 = PA4 下降沿
 *   LED1 = PC0          LED2 = PC1
 *   LED3 = PC2(绿)      LED4 = PC3(红)   双色灯：PC2+PC3 同亮=黄
 */
#include "bsp/bsp.h"

#include "drivers/bicolor_led.h"
#include "drivers/led.h"
#include "platform/stm32f1/stm32f1_gpio_port.h"
#include "platform/stm32f1/stm32f1_timebase.h"
#include "stm32f1xx_hal.h"   /* GPIOA / GPIOC / GPIO_PIN_x */

#define DEBOUNCE_MS  20U

/* —— LED 硬件描述表 ——
 * 板级 LED 编号作为下标，提升可读性；加灯 = 在此追加一项。 */
enum {
    BSP_LED_1 = 0,  /* PC0 */
    BSP_LED_2,      /* PC1 */
    BSP_LED_3,      /* PC2 双色-绿 */
    BSP_LED_4,      /* PC3 双色-红 */
    BSP_LED_COUNT,
};

typedef struct {
    GPIO_TypeDef  *port;
    uint16_t       pin;
    led_polarity_t polarity;
} led_desc_t;

/* 按规格：LED1=PC0, LED2=PC1, LED3=PC2, LED4=PC3，全部高电平点亮 */
static const led_desc_t s_led_descs[BSP_LED_COUNT] = {
    [BSP_LED_1] = { GPIOC, GPIO_PIN_0, LED_ACTIVE_HIGH },
    [BSP_LED_2] = { GPIOC, GPIO_PIN_1, LED_ACTIVE_HIGH },
    [BSP_LED_3] = { GPIOC, GPIO_PIN_2, LED_ACTIVE_HIGH },
    [BSP_LED_4] = { GPIOC, GPIO_PIN_3, LED_ACTIVE_HIGH },
};

/* —— 按键硬件描述表 ——
 * 按规格硬件映射（下拉/上拉、空闲/按下电平）：
 *   KEY1 PA0  下拉  空闲低 按下高  -> active_high, RISING  (物理 0->1)
 *   KEY2 PA1  上拉  空闲高 按下低  -> active_low,  FALLING (物理 1->0)
 *   KEY3 PA4  上拉  空闲高 按下低  -> active_low,  FALLING (物理 1->0)
 *
 * pull_up 字段：true=GPIO_PULLUP，false=GPIO_PULLDOWN。
 * active_low 只决定"物理按下"对应电平；edge 决定上抛哪一类稳定边沿。 */
typedef struct {
    GPIO_TypeDef *port;
    uint16_t      pin;
    bool          pull_up;      /* true=上拉, false=下拉 */
    bool          active_low;   /* 物理按下是否为低电平 */
    button_edge_t edge;         /* 业务关心的物理边沿    */
} key_desc_t;

static const key_desc_t s_key_descs[BSP_KEY_COUNT] = {
    [BSP_KEY_1] = { GPIOA, GPIO_PIN_0, false, false, BUTTON_EDGE_RISING  },
    [BSP_KEY_2] = { GPIOA, GPIO_PIN_1, true,  true,  BUTTON_EDGE_FALLING },
    [BSP_KEY_3] = { GPIOA, GPIO_PIN_4, true,  true,  BUTTON_EDGE_FALLING },
};

/* —— 运行期实例：bsp 自己拥有 —— */
static hal_gpio_pin_t        s_led_pins[BSP_LED_COUNT];
static stm32f1_gpio_pin_ctx_t s_led_ctx[BSP_LED_COUNT];
static led_t                 s_leds[BSP_LED_COUNT];
static bicolor_led_t         s_bicolor;   /* LED3 / LED4 组双色灯 */

static hal_gpio_pin_t        s_key_pins[BSP_KEY_COUNT];
static stm32f1_gpio_pin_ctx_t s_key_ctx[BSP_KEY_COUNT];
static button_t              s_keys[BSP_KEY_COUNT];

static hal_timebase_t        s_timebase;

/* —— 初始化 LED —— */
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

    /* 组合双色灯：绿=PC2, 红=PC3 */
    bicolor_led_init(&s_bicolor, &s_leds[BSP_LED_3], &s_leds[BSP_LED_4]);
}

/* —— 初始化按键 —— */
static void bsp_keys_init(void)
{
    uint8_t i;

    for (i = 0U; i < BSP_KEY_COUNT; ++i) {
        /* 输入引脚按描述表配置上拉/下拉，保证空闲电平稳定。 */
        stm32f1_gpio_input_init(&s_key_pins[i], &s_key_ctx[i],
                                s_key_descs[i].port, s_key_descs[i].pin,
                                s_key_descs[i].pull_up);
        button_init(&s_keys[i], &s_key_pins[i],
                    s_key_descs[i].active_low,
                    DEBOUNCE_MS,
                    s_key_descs[i].edge);
    }
}

void bsp_init(void)
{
    stm32f1_timebase_init(&s_timebase);
    bsp_leds_init();
    bsp_keys_init();
}

void bsp_tick(void)
{
    uint8_t i;
    const uint32_t now_ms = hal_millis(&s_timebase);

    /* LED 闪烁推进（非阻塞）。
     * 按键消抖采样由 app 通过 bsp_key()->button_poll 自行消费事件，
     * 这里不做"吃掉"事件的操作。 */
    for (i = 0U; i < BSP_LED_COUNT; ++i) {
        led_tick(&s_leds[i], now_ms);
    }
}

button_t *bsp_key(bsp_key_id_t id)
{
    if (id >= BSP_KEY_COUNT) {
        return NULL;
    }
    return &s_keys[id];
}

led_t *bsp_led1(void)
{
    return &s_leds[BSP_LED_1];
}

led_t *bsp_led2(void)
{
    return &s_leds[BSP_LED_2];
}

bicolor_led_t *bsp_bicolor(void)
{
    return &s_bicolor;
}

uint32_t bsp_now_ms(void)
{
    return hal_millis(&s_timebase);
}
