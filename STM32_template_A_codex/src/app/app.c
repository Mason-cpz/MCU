#include "app/app.h"

#include <assert.h>

typedef struct {
    bool blink;
    led_state_t state;
    uint16_t on_ms;
    uint16_t off_ms;
} app_led_step_t;

typedef struct {
    bool blink;
    bicolor_color_t color;
    uint16_t on_ms;
    uint16_t off_ms;
} app_bicolor_step_t;

static const app_led_step_t s_led1_boot_step = { true, LED_ON, 500U, 500U };

static const app_led_step_t s_led1_steps[] = {
    { false, LED_OFF, 0U, 0U },
    { false, LED_ON, 0U, 0U },
    { true, LED_ON, 500U, 500U },
};

static const app_led_step_t s_led2_steps[] = {
    { false, LED_OFF, 0U, 0U },
    { false, LED_ON, 0U, 0U },
    { true, LED_ON, 200U, 800U },
    { true, LED_ON, 800U, 200U },
};

static const app_bicolor_step_t s_bicolor_steps[] = {
    { false, BICOLOR_OFF, 0U, 0U },
    { false, BICOLOR_GREEN, 0U, 0U },
    { false, BICOLOR_RED, 0U, 0U },
    { false, BICOLOR_YELLOW, 0U, 0U },
    { true, BICOLOR_YELLOW, 500U, 500U },
};

enum {
    APP_LED1_STEP_COUNT = (uint8_t)(sizeof(s_led1_steps) / sizeof(s_led1_steps[0])),
    APP_LED2_STEP_COUNT = (uint8_t)(sizeof(s_led2_steps) / sizeof(s_led2_steps[0])),
    APP_BICOLOR_STEP_COUNT = (uint8_t)(sizeof(s_bicolor_steps) / sizeof(s_bicolor_steps[0])),
};

static uint8_t app_next_index(uint8_t index, uint8_t count)
{
    assert(count > 0U);

    return (uint8_t)((index + 1U) % count);
}

static void app_apply_led_step(led_t *led, const app_led_step_t *step, uint32_t now_ms)
{
    assert(led != NULL);
    assert(step != NULL);

    if (step->blink) {
        led_set_blink(led, step->on_ms, step->off_ms, now_ms);
        return;
    }

    led_set(led, step->state);
}

static void app_apply_bicolor_step(
    bicolor_led_t *bicolor,
    const app_bicolor_step_t *step,
    uint32_t now_ms)
{
    assert(bicolor != NULL);
    assert(step != NULL);

    if (step->blink) {
        bicolor_led_blink(bicolor, step->color, step->on_ms, step->off_ms, now_ms);
        return;
    }

    bicolor_led_set(bicolor, step->color);
}

void app_init(
    app_t *self,
    button_t *key1,
    led_t *led1,
    button_t *key2,
    led_t *led2,
    button_t *key3,
    bicolor_led_t *bicolor,
    uint32_t now_ms)
{
    assert(self != NULL);
    assert(key1 != NULL);
    assert(led1 != NULL);
    assert(key2 != NULL);
    assert(led2 != NULL);
    assert(key3 != NULL);
    assert(bicolor != NULL);

    self->key1 = key1;
    self->led1 = led1;
    self->key2 = key2;
    self->led2 = led2;
    self->key3 = key3;
    self->bicolor = bicolor;
    self->led1_step_index = 0U;
    self->led2_step_index = 0U;
    self->bicolor_step_index = 0U;
    self->led1_boot_heartbeat = true;

    app_apply_led_step(self->led1, &s_led1_boot_step, now_ms);
    app_apply_led_step(self->led2, &s_led2_steps[self->led2_step_index], now_ms);
    app_apply_bicolor_step(self->bicolor, &s_bicolor_steps[self->bicolor_step_index], now_ms);
}

static void app_advance_led1(app_t *self, uint32_t now_ms)
{
    if (self->led1_boot_heartbeat) {
        self->led1_boot_heartbeat = false;
        self->led1_step_index = 0U;
    } else {
        self->led1_step_index = app_next_index(self->led1_step_index, APP_LED1_STEP_COUNT);
    }

    app_apply_led_step(self->led1, &s_led1_steps[self->led1_step_index], now_ms);
}

static void app_advance_led2(app_t *self, uint32_t now_ms)
{
    self->led2_step_index = app_next_index(self->led2_step_index, APP_LED2_STEP_COUNT);
    app_apply_led_step(self->led2, &s_led2_steps[self->led2_step_index], now_ms);
}

static void app_advance_bicolor(app_t *self, uint32_t now_ms)
{
    self->bicolor_step_index = app_next_index(self->bicolor_step_index, APP_BICOLOR_STEP_COUNT);
    app_apply_bicolor_step(self->bicolor, &s_bicolor_steps[self->bicolor_step_index], now_ms);
}

void app_tick(app_t *self, uint32_t now_ms)
{
    assert(self != NULL);

    if (button_poll(self->key1, now_ms) != BUTTON_EVENT_NONE) {
        app_advance_led1(self, now_ms);
    }

    if (button_poll(self->key2, now_ms) != BUTTON_EVENT_NONE) {
        app_advance_led2(self, now_ms);
    }

    if (button_poll(self->key3, now_ms) != BUTTON_EVENT_NONE) {
        app_advance_bicolor(self, now_ms);
    }
}
