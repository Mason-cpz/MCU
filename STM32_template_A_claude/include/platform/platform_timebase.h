#ifndef PLATFORM_PLATFORM_TIMEBASE_H
#define PLATFORM_PLATFORM_TIMEBASE_H

#include "hal/timebase.h"

/* 平台无关的时基门面。
 * 填充 timebase，使上层通过 hal_millis() 取毫秒计数，不依赖任何厂商符号。
 * 迁移到其它芯片时只换平台实现。 */
void platform_timebase_init(hal_timebase_t *timebase);

#endif
