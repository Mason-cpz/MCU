#ifndef APP_APP_H
#define APP_APP_H

#include "drivers/led/bicolor_led.h"
#include "drivers/button.h"
#include "drivers/led/led.h"

#include <stdbool.h>
#include <stdint.h>

/* app 层只保存 Demo 的输入输出句柄和状态游标。 */
typedef struct {
    button_t *key1;
    button_t *key2;
    button_t *key3;
    led_t *led1;
    led_t *led2;
    bicolor_led_t *bicolor;
    uint8_t led1_step_index;
    uint8_t led2_step_index;
    uint8_t bicolor_step_index;
    bool led1_boot_heartbeat;
    bool led1_blink_on;
    bool led2_blink_on;
    bool bicolor_blink_on;
    uint32_t led1_last_toggle_ms;
    uint32_t led2_last_toggle_ms;
    uint32_t bicolor_last_toggle_ms;
} app_t;

void app_init(
    app_t *self,
    button_t *key1,
    led_t *led1,
    button_t *key2,
    led_t *led2,
    button_t *key3,
    bicolor_led_t *bicolor,
    uint32_t now_ms);

void app_tick(app_t *self, uint32_t now_ms);

#endif
