#include "platform/stm32f1/stm32f1_timebase.h"

#include "stm32f1xx_hal.h"

static uint32_t stm32f1_millis(void *ctx)
{
    (void)ctx;
    return HAL_GetTick();
}

void stm32f1_timebase_init(hal_timebase_t *timebase)
{
    timebase->ctx = 0;
    timebase->millis = stm32f1_millis;
}
