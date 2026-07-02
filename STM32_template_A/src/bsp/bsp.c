/* bsp.c
 * 板级支持包组合根。持有所有硬件和驱动实例，对外暴露 bsp_init/bsp_tick
 * 以及 app 所需的访问器。
 *
 * 本文件只做"总装配"：调各外设模块的 *_bsp_init，不持有外设细节。
 * LED 的硬件映射/胶水在 peripherals/led/led_bsp.c，
 * KEY 的硬件映射/胶水留在本文件（后续也可按 LED 模式拆分）。
 *
 * 硬件映射（KEY_LED_FRAMEWORK_SPEC.md）：
 *   KEY1 = PA0 上升沿   KEY2 = PA1 下降沿   KEY3 = PA4 下降沿
 *   LED  见 peripherals/led/led_bsp.c
 */
#include "bsp/bsp.h"

#include "peripherals/led/led_bsp.h"
#include "platform/stm32f1/stm32f1_gpio_port.h"
#include "platform/stm32f1/stm32f1_timebase.h"
#include "stm32f1xx_hal.h"   /* GPIOA / GPIO_PIN_x */

#define DEBOUNCE_MS  20U

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

/* —— 运行期实例：bsp 自己拥有（按键部分）—— */
static hal_gpio_pin_t        s_key_pins[BSP_KEY_COUNT];
static stm32f1_gpio_pin_ctx_t s_key_ctx[BSP_KEY_COUNT];
static button_t              s_keys[BSP_KEY_COUNT];

static hal_timebase_t        s_timebase;

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
    led_bsp_init();
    bsp_keys_init();
}

void bsp_tick(void)
{
    const uint32_t now_ms = hal_millis(&s_timebase);

    /* LED 闪烁推进（非阻塞）。
     * 按键消抖采样由 app 通过 bsp_key()->button_poll 自行消费事件，
     * 这里不做"吃掉"事件的操作。 */
    led_bsp_tick(now_ms);
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
    return led_bsp_get(BSP_LED_1);
}

led_t *bsp_led2(void)
{
    return led_bsp_get(BSP_LED_2);
}

bicolor_led_t *bsp_bicolor(void)
{
    return led_bsp_bicolor();
}

uint32_t bsp_now_ms(void)
{
    return hal_millis(&s_timebase);
}
