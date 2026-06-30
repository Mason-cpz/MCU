#ifndef HAL_GPIO_H
#define HAL_GPIO_H

#include <stdbool.h>

/* GPIO 最小抽象。
 *
 * 上层只关心逻辑电平，不直接碰 STM32 HAL。
 */
typedef struct {
    void *ctx;
    bool (*read)(void *ctx);
    void (*write)(void *ctx, bool level);
} hal_gpio_pin_t;

static inline bool hal_gpio_read(const hal_gpio_pin_t *pin)
{
    return pin->read(pin->ctx);
}

static inline void hal_gpio_write(const hal_gpio_pin_t *pin, bool level)
{
    pin->write(pin->ctx, level);
}

#endif
