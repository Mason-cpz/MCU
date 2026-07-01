#ifndef HAL_TIMEBASE_H
#define HAL_TIMEBASE_H

#include <stdint.h>

/* 时间基准只保留毫秒接口。
 * 业务层用 tick 推进，不在框架里塞阻塞延时。
 */
typedef struct {
    void *ctx;
    uint32_t (*millis)(void *ctx);
} hal_timebase_t;

static inline uint32_t hal_millis(const hal_timebase_t *timebase)
{
    return timebase->millis(timebase->ctx);
}

#endif
