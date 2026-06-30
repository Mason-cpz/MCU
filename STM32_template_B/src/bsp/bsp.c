#include "bsp/bsp.h"

#include "app/app.h"
#include "main.h"
#include "platform/stm32f1_board.h"

static stm32f1_board_t board;
static app_t app;

static const stm32f1_board_led_desc_t board_leds[] = {
    { LED1_GPIO_Port, LED1_Pin, LED_ACTIVE_HIGH },
    { LED2_GPIO_Port, LED2_Pin, LED_ACTIVE_HIGH },
    { LED3_GPIO_Port, LED3_Pin, LED_ACTIVE_HIGH },
    { LED4_GPIO_Port, LED4_Pin, LED_ACTIVE_HIGH },
};

static const app_led_behavior_desc_t led_behaviors[] = {
    { APP_LED_STEADY_ON, 0U, 0U },
    { APP_LED_BLINK, 500U, 500U },
    { APP_LED_OFF, 0U, 0U },
    { APP_LED_OFF, 0U, 0U },
};

static const app_bicolor_desc_t bicolor_desc = {
    .enabled = true,
    .green_index = 2U,
    .red_index = 3U,
    .color = BICOLOR_YELLOW,
    .blink = true,
    .on_ms = 500U,
    .off_ms = 500U,
};

void bsp_init(void)
{
    stm32f1_board_init(
        &board,
        board_leds,
        sizeof(board_leds) / sizeof(board_leds[0]));

    app_init(&app, &board, led_behaviors, &bicolor_desc);
}

void bsp_tick(void)
{
    app_tick(&app);
}
