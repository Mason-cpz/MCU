/* stm32f1_timebase.c
 * platform_timebase.h 的 STM32F1 实现。
 * HAL_GetTick() 默认由 SysTick 驱动，单位毫秒；厂商符号只出现在本文件。
 */
#include "platform/platform_timebase.h"

#include "stm32f1xx_hal.h"

#include <stddef.h>

static uint32_t stm32f1_millis(void *ctx)
{
    (void)ctx;
    return HAL_GetTick();
}

void platform_timebase_init(hal_timebase_t *timebase)
{
    timebase->ctx    = NULL;
    timebase->millis = stm32f1_millis;
}
