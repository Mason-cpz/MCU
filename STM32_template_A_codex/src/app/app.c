#include "app/app.h"

#include <assert.h>

enum {
    LED1_STEP_OFF = 0U,
    LED1_STEP_ON,
    LED1_STEP_BLINK_500_500,
    LED1_STEP_COUNT,
};

enum {
    LED2_STEP_OFF = 0U,
    LED2_STEP_ON,
    LED2_STEP_BLINK_200_800,
    LED2_STEP_BLINK_800_200,
    LED2_STEP_COUNT,
};

enum {
    BICOLOR_STEP_OFF = 0U,
    BICOLOR_STEP_GREEN,
    BICOLOR_STEP_RED,
    BICOLOR_STEP_YELLOW,
    BICOLOR_STEP_YELLOW_BLINK,
    BICOLOR_STEP_COUNT,
};

static uint8_t app_next_index(uint8_t index, uint8_t count)
{
    assert(count > 0U);

    return (uint8_t)((index + 1U) % count);
}

static void app_start_led_blink(
    led_t *led,
    bool *phase_on,
    uint32_t *last_toggle_ms,
    uint32_t now_ms)
{
    assert(led != NULL);
    assert(phase_on != NULL);
    assert(last_toggle_ms != NULL);

    *phase_on = true;
    *last_toggle_ms = now_ms;
    led_set(led, LED_ON);
}

static void app_service_led_blink(
    led_t *led,
    bool *phase_on,
    uint32_t *last_toggle_ms,
    uint16_t on_ms,
    uint16_t off_ms,
    uint32_t now_ms)
{
    uint16_t current_ms;

    assert(led != NULL);
    assert(phase_on != NULL);
    assert(last_toggle_ms != NULL);

    current_ms = *phase_on ? on_ms : off_ms;

    if ((uint32_t)(now_ms - *last_toggle_ms) < current_ms) {
        return;
    }

    *phase_on = !*phase_on;
    *last_toggle_ms = now_ms;
    led_set(led, *phase_on ? LED_ON : LED_OFF);
}

static void app_start_bicolor_blink(
    bicolor_led_t *bicolor,
    bool *phase_on,
    uint32_t *last_toggle_ms,
    bicolor_color_t color,
    uint32_t now_ms)
{
    assert(bicolor != NULL);
    assert(phase_on != NULL);
    assert(last_toggle_ms != NULL);

    *phase_on = true;
    *last_toggle_ms = now_ms;
    bicolor_led_set(bicolor, color);
}

static void app_service_bicolor_blink(
    bicolor_led_t *bicolor,
    bool *phase_on,
    uint32_t *last_toggle_ms,
    bicolor_color_t color,
    uint16_t on_ms,
    uint16_t off_ms,
    uint32_t now_ms)
{
    uint16_t current_ms;

    assert(bicolor != NULL);
    assert(phase_on != NULL);
    assert(last_toggle_ms != NULL);

    current_ms = *phase_on ? on_ms : off_ms;

    if ((uint32_t)(now_ms - *last_toggle_ms) < current_ms) {
        return;
    }

    *phase_on = !*phase_on;
    *last_toggle_ms = now_ms;
    bicolor_led_set(bicolor, *phase_on ? color : BICOLOR_OFF);
}

static void app_apply_led1(app_t *self, uint32_t now_ms)
{
    if (self->led1_step_index == LED1_STEP_ON) {
        led_set(self->led1, LED_ON);
        return;
    }

    if (self->led1_step_index == LED1_STEP_BLINK_500_500) {
        app_start_led_blink(
            self->led1,
            &self->led1_blink_on,
            &self->led1_last_toggle_ms,
            now_ms);
        return;
    }

    led_set(self->led1, LED_OFF);
}

static void app_apply_led2(app_t *self, uint32_t now_ms)
{
    if (self->led2_step_index == LED2_STEP_ON) {
        led_set(self->led2, LED_ON);
        return;
    }

    if (self->led2_step_index == LED2_STEP_BLINK_200_800 ||
        self->led2_step_index == LED2_STEP_BLINK_800_200) {
        app_start_led_blink(
            self->led2,
            &self->led2_blink_on,
            &self->led2_last_toggle_ms,
            now_ms);
        return;
    }

    led_set(self->led2, LED_OFF);
}

static void app_apply_bicolor(app_t *self, uint32_t now_ms)
{
    if (self->bicolor_step_index == BICOLOR_STEP_GREEN) {
        bicolor_led_set(self->bicolor, BICOLOR_GREEN);
        return;
    }

    if (self->bicolor_step_index == BICOLOR_STEP_RED) {
        bicolor_led_set(self->bicolor, BICOLOR_RED);
        return;
    }

    if (self->bicolor_step_index == BICOLOR_STEP_YELLOW) {
        bicolor_led_set(self->bicolor, BICOLOR_YELLOW);
        return;
    }

    if (self->bicolor_step_index == BICOLOR_STEP_YELLOW_BLINK) {
        app_start_bicolor_blink(
            self->bicolor,
            &self->bicolor_blink_on,
            &self->bicolor_last_toggle_ms,
            BICOLOR_YELLOW,
            now_ms);
        return;
    }

    bicolor_led_set(self->bicolor, BICOLOR_OFF);
}

static void app_service_led1(app_t *self, uint32_t now_ms)
{
    if (self->led1_boot_heartbeat ||
        self->led1_step_index == LED1_STEP_BLINK_500_500) {
        app_service_led_blink(
            self->led1,
            &self->led1_blink_on,
            &self->led1_last_toggle_ms,
            500U,
            500U,
            now_ms);
    }
}

static void app_service_led2(app_t *self, uint32_t now_ms)
{
    if (self->led2_step_index == LED2_STEP_BLINK_200_800) {
        app_service_led_blink(
            self->led2,
            &self->led2_blink_on,
            &self->led2_last_toggle_ms,
            200U,
            800U,
            now_ms);
        return;
    }

    if (self->led2_step_index == LED2_STEP_BLINK_800_200) {
        app_service_led_blink(
            self->led2,
            &self->led2_blink_on,
            &self->led2_last_toggle_ms,
            800U,
            200U,
            now_ms);
    }
}

static void app_service_bicolor(app_t *self, uint32_t now_ms)
{
    if (self->bicolor_step_index == BICOLOR_STEP_YELLOW_BLINK) {
        app_service_bicolor_blink(
            self->bicolor,
            &self->bicolor_blink_on,
            &self->bicolor_last_toggle_ms,
            BICOLOR_YELLOW,
            500U,
            500U,
            now_ms);
    }
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
    self->led1_step_index = LED1_STEP_OFF;
    self->led2_step_index = LED2_STEP_OFF;
    self->bicolor_step_index = BICOLOR_STEP_OFF;
    self->led1_boot_heartbeat = true;
    self->led1_blink_on = false;
    self->led2_blink_on = false;
    self->bicolor_blink_on = false;
    self->led1_last_toggle_ms = now_ms;
    self->led2_last_toggle_ms = now_ms;
    self->bicolor_last_toggle_ms = now_ms;

    app_start_led_blink(
        self->led1,
        &self->led1_blink_on,
        &self->led1_last_toggle_ms,
        now_ms);
    led_set(self->led2, LED_OFF);
    bicolor_led_set(self->bicolor, BICOLOR_OFF);
}

static void app_advance_led1(app_t *self, uint32_t now_ms)
{
    if (self->led1_boot_heartbeat) {
        self->led1_boot_heartbeat = false;
        self->led1_step_index = LED1_STEP_OFF;
    } else {
        self->led1_step_index = app_next_index(self->led1_step_index, LED1_STEP_COUNT);
    }

    app_apply_led1(self, now_ms);
}

static void app_advance_led2(app_t *self, uint32_t now_ms)
{
    self->led2_step_index = app_next_index(self->led2_step_index, LED2_STEP_COUNT);
    app_apply_led2(self, now_ms);
}

static void app_advance_bicolor(app_t *self, uint32_t now_ms)
{
    self->bicolor_step_index = app_next_index(self->bicolor_step_index, BICOLOR_STEP_COUNT);
    app_apply_bicolor(self, now_ms);
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

    app_service_led1(self, now_ms);
    app_service_led2(self, now_ms);
    app_service_bicolor(self, now_ms);
}
