#ifndef PLATFORM_STM32F1_TIMEBASE_H
#define PLATFORM_STM32F1_TIMEBASE_H

#include "hal/timebase.h"

/* STM32F1 时间基准适配。
 * HAL_GetTick() 默认由 SysTick 驱动，单位是毫秒。
 */
void stm32f1_timebase_init(hal_timebase_t *timebase);

#endif
