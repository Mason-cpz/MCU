#ifndef TESTS_FAKE_HAL_H
#define TESTS_FAKE_HAL_H

/* 主机侧假 HAL：用一个可写电平的引脚和一个手动推进的毫秒计数，
 * 把 driver/app 从真实硬件上剥离出来，在 PC 上验证逻辑与时序。
 * 这正是"driver/app 只依赖 hal 接口"的分层带来的可测性。 */
#include "hal/gpio.h"
#include "hal/timebase.h"

#include <stdbool.h>
#include <stdint.h>

/* --- 可控引脚 --- */
typedef struct {
    bool level;   /* 当前物理电平，测试直接读写 */
} fake_pin_t;

static bool fake_pin_read(void *ctx)
{
    return ((const fake_pin_t *)ctx)->level;
}

static void fake_pin_write(void *ctx, bool level)
{
    ((fake_pin_t *)ctx)->level = level;
}

static inline hal_gpio_pin_t fake_pin_bind(fake_pin_t *fp, bool initial_level)
{
    hal_gpio_pin_t pin;
    fp->level = initial_level;
    pin.ctx   = fp;
    pin.read  = fake_pin_read;
    pin.write = fake_pin_write;
    return pin;
}

/* --- 可控时基 --- */
extern uint32_t g_fake_now_ms;

static uint32_t fake_millis(void *ctx)
{
    (void)ctx;
    return g_fake_now_ms;
}

static inline hal_timebase_t fake_timebase(void)
{
    hal_timebase_t tb;
    tb.ctx    = 0;
    tb.millis = fake_millis;
    return tb;
}

#endif
