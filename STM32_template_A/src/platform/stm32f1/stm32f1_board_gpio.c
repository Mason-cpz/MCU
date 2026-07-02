/* STM32F1 平台的板级 GPIO 抽象实现。
 * 把 stm32f1_gpio_pin_init / stm32f1_timebase_init 包装为 board_* 接口。
 */
#include "platform/stm32f1/stm32f1_board_gpio.h"

#include "platform/stm32f1/stm32f1_timebase.h"

void board_gpio_pin_init(
    hal_gpio_pin_t *pin,
    board_gpio_pin_ctx_t *ctx,
    board_gpio_port_t port,
    board_pin_mask_t pin_mask,
    bool initial_level)
{
    stm32f1_gpio_pin_init(pin, ctx, port, pin_mask, initial_level);
}

void board_timebase_init(hal_timebase_t *timebase)
{
    stm32f1_timebase_init(timebase);
}
