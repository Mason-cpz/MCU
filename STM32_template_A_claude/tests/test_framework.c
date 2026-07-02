/* test_framework.c
 * 主机侧行为测试：用假 GPIO/时基驱动 button/led/bicolor/app，
 * 断言消抖、短按窗口、闪烁时序、三条状态机循环全部符合 KEY_LED_FRAMEWORK_SPEC.md。
 * 编译运行见同目录 run_tests.sh / build.bat。
 */
#include "fake_hal.h"

#include "drivers/key/button.h"
#include "drivers/led/led.h"
#include "drivers/led/bicolor_led.h"
#include "app/app.h"

#include <stdio.h>

uint32_t g_fake_now_ms;   /* fake_hal.h 里 extern 的时基 */

/* ---- 极简断言框架 ---- */
static int g_checks;
static int g_fails;

#define CHECK(cond, msg) do {                                   \
    ++g_checks;                                                 \
    if (!(cond)) {                                              \
        ++g_fails;                                              \
        printf("  FAIL: %s (%s:%d)\n", (msg), __FILE__, __LINE__); \
    }                                                           \
} while (0)

/* 推进时基到指定毫秒（绝对时间）。 */
static void advance_to(uint32_t ms) { g_fake_now_ms = ms; }

/* ============================ 按键：消抖 ============================ */
/* KEY2 型：active_low（下降沿，按下=低电平），消抖 20ms。 */
static void test_button_debounce_rejects_glitch(void)
{
    printf("[button] 抖动小于消抖窗口不产生事件\n");
    fake_pin_t fp;
    hal_gpio_pin_t pin = fake_pin_bind(&fp, true /*空闲高*/);
    button_t btn;

    advance_to(0);
    button_init(&btn, &pin, true /*active_low*/, 20U, 1000U);
    CHECK(button_poll(&btn, 0) == BUTTON_EVENT_NONE, "初始无事件");

    /* 10ms 的毛刺（拉低后又抬高），不足 20ms 窗口 */
    fp.level = false;                 /* 按下（低） */
    advance_to(5);  CHECK(button_poll(&btn, 5)  == BUTTON_EVENT_NONE, "5ms 未稳定");
    fp.level = true;                  /* 又抬起，毛刺结束 */
    advance_to(10); CHECK(button_poll(&btn, 10) == BUTTON_EVENT_NONE, "毛刺被滤除");
    advance_to(50); CHECK(button_poll(&btn, 50) == BUTTON_EVENT_NONE, "此后仍无事件");
}

/* 稳定按下后在窗口内抬起 = 一次 SHORT_PRESS，只发一次。 */
static void test_button_short_press(void)
{
    printf("[button] 按下并在 1000ms 内抬起 = 一次短按\n");
    fake_pin_t fp;
    hal_gpio_pin_t pin = fake_pin_bind(&fp, true /*空闲高*/);
    button_t btn;
    button_event_t ev;

    advance_to(0);
    button_init(&btn, &pin, true, 20U, 1000U);
    button_poll(&btn, 0);                     /* 建立空闲基线 */

    fp.level = false;                          /* 按下 */
    advance_to(100); ev = button_poll(&btn, 100); CHECK(ev == BUTTON_EVENT_NONE, "按下沿不发事件");
    advance_to(125); ev = button_poll(&btn, 125); CHECK(ev == BUTTON_EVENT_NONE, "按住期间不发事件");

    fp.level = true;                           /* 抬起 */
    advance_to(200); ev = button_poll(&btn, 200); CHECK(ev == BUTTON_EVENT_NONE, "抬起沿待稳定");
    advance_to(225); ev = button_poll(&btn, 225); CHECK(ev == BUTTON_EVENT_SHORT_PRESS, "稳定抬起=短按");
    advance_to(260); ev = button_poll(&btn, 260); CHECK(ev == BUTTON_EVENT_NONE, "短按只发一次");
}

/* 按住超过 short_press_ms 再抬起，本 Demo 不产生事件（留给长按）。 */
static void test_button_long_press_no_event(void)
{
    printf("[button] 按住超过 1000ms 抬起不产生短按\n");
    fake_pin_t fp;
    hal_gpio_pin_t pin = fake_pin_bind(&fp, true);
    button_t btn;
    button_event_t ev;

    advance_to(0);
    button_init(&btn, &pin, true, 20U, 1000U);
    button_poll(&btn, 0);

    fp.level = false;                          /* 按下 */
    advance_to(0);   button_poll(&btn, 0);
    advance_to(25);  button_poll(&btn, 25);    /* 按下沿确认，pressed_since=25 */
    fp.level = true;                           /* 1500ms 后才抬起 */
    advance_to(1500); button_poll(&btn, 1500);
    advance_to(1525); ev = button_poll(&btn, 1525);
    CHECK(ev == BUTTON_EVENT_NONE, "超时长按不算短按");
}

/* ============================ LED：闪烁时序 ============================ */
/* ACTIVE_HIGH：ON=高电平。BLINK(200/800) 应亮 200ms、灭 800ms，非阻塞由 tick 推进。 */
static void test_led_blink_timing(void)
{
    printf("[led] BLINK(200/800) 时序由 tick 推进\n");
    fake_pin_t fp;
    hal_gpio_pin_t pin = fake_pin_bind(&fp, false);
    led_t led;

    advance_to(0);
    led_init(&led, &pin, LED_ACTIVE_HIGH);
    CHECK(fp.level == false, "init 后灭（低电平）");

    led_set_blink(&led, 200U, 800U, 0);
    CHECK(fp.level == true, "起振即点亮");

    advance_to(199); led_tick(&led, 199); CHECK(fp.level == true,  "199ms 仍亮");
    advance_to(200); led_tick(&led, 200); CHECK(fp.level == false, "200ms 转灭");
    advance_to(999); led_tick(&led, 999); CHECK(fp.level == false, "灭持续到 999ms");
    advance_to(1000);led_tick(&led, 1000);CHECK(fp.level == true,  "1000ms 再亮");
}

/* steady 模式不受 tick 影响。 */
static void test_led_steady_ignores_tick(void)
{
    printf("[led] 常亮/关闭不被 tick 改变\n");
    fake_pin_t fp;
    hal_gpio_pin_t pin = fake_pin_bind(&fp, false);
    led_t led;

    led_init(&led, &pin, LED_ACTIVE_HIGH);
    led_set(&led, LED_ON);
    CHECK(fp.level == true, "ON=高");
    advance_to(5000); led_tick(&led, 5000);
    CHECK(fp.level == true, "tick 不改常亮");
}

/* ============================ 双色灯：颜色 = 两路组合 ============================ */
static void test_bicolor_colors(void)
{
    printf("[bicolor] 黄 = 绿+红同亮；各颜色映射正确\n");
    fake_pin_t fg, fr;
    hal_gpio_pin_t pg = fake_pin_bind(&fg, false);
    hal_gpio_pin_t pr = fake_pin_bind(&fr, false);
    led_t green, red;
    bicolor_led_t bi;

    led_init(&green, &pg, LED_ACTIVE_HIGH);
    led_init(&red,   &pr, LED_ACTIVE_HIGH);
    bicolor_led_init(&bi, &green, &red);

    bicolor_led_set(&bi, BICOLOR_GREEN);
    CHECK(fg.level == true && fr.level == false, "GREEN=绿亮红灭");
    bicolor_led_set(&bi, BICOLOR_RED);
    CHECK(fg.level == false && fr.level == true, "RED=绿灭红亮");
    bicolor_led_set(&bi, BICOLOR_YELLOW);
    CHECK(fg.level == true && fr.level == true, "YELLOW=绿红同亮");
    bicolor_led_set(&bi, BICOLOR_OFF);
    CHECK(fg.level == false && fr.level == false, "OFF=全灭");
}

/* 黄色闪烁：绿红必须同相亮灭。 */
static void test_bicolor_yellow_blink_in_phase(void)
{
    printf("[bicolor] 黄色闪烁时绿红同相\n");
    fake_pin_t fg, fr;
    hal_gpio_pin_t pg = fake_pin_bind(&fg, false);
    hal_gpio_pin_t pr = fake_pin_bind(&fr, false);
    led_t green, red;
    bicolor_led_t bi;

    advance_to(0);
    led_init(&green, &pg, LED_ACTIVE_HIGH);
    led_init(&red,   &pr, LED_ACTIVE_HIGH);
    bicolor_led_init(&bi, &green, &red);

    bicolor_led_blink(&bi, BICOLOR_YELLOW, 500U, 500U, 0);
    CHECK(fg.level == true && fr.level == true, "起振同亮");
    advance_to(500); led_tick(&green, 500); led_tick(&red, 500);
    CHECK(fg.level == false && fr.level == false, "500ms 同灭");
    advance_to(1000); led_tick(&green, 1000); led_tick(&red, 1000);
    CHECK(fg.level == true && fr.level == true, "1000ms 同亮");
}

/* ============================ app：三条状态机循环 ============================ */
/* 用一组假引脚搭出完整 app，模拟按键并断言 LED 输出符合 spec 循环。 */
typedef struct {
    fake_pin_t     key_fp[3];
    fake_pin_t     led_fp[4];   /* LED1 LED2 LED3(绿) LED4(红) */
    hal_gpio_pin_t key_pin[3];
    hal_gpio_pin_t led_pin[4];
    button_t       key[3];
    led_t          led[4];
    bicolor_led_t  bicolor;
    app_t          app;
} app_fixture_t;

/* KEY 索引与按下电平：KEY1 上升沿(按下=高)，KEY2/3 下降沿(按下=低)。 */
enum { K1, K2, K3 };
static const bool k_active_low[3] = { false, true, true };  /* 与 bsp 一致 */

static void fixture_init(app_fixture_t *f)
{
    int i;
    advance_to(0);
    /* 按键：空闲电平 = 未按下。active_low 键空闲高，反之空闲低。 */
    for (i = 0; i < 3; ++i) {
        f->key_pin[i] = fake_pin_bind(&f->key_fp[i], k_active_low[i] ? true : false);
        button_init(&f->key[i], &f->key_pin[i], k_active_low[i], 20U, 1000U);
    }
    for (i = 0; i < 4; ++i) {
        f->led_pin[i] = fake_pin_bind(&f->led_fp[i], false);
        led_init(&f->led[i], &f->led_pin[i], LED_ACTIVE_HIGH);
    }
    bicolor_led_init(&f->bicolor, &f->led[2], &f->led[3]);
    app_init(&f->app, &f->key[K1], &f->led[0], &f->key[K2], &f->led[1],
             &f->key[K3], &f->bicolor, 0);
}

/* 模拟一次短按：按下→保持过消抖→抬起→过消抖，其间只跑 app_tick(不跑 led_tick，
 * 以免闪烁翻转干扰对"档位"的判定)。调用后 g_fake_now_ms 停在抬起已稳定处。 */
static void press(app_fixture_t *f, int k)
{
    const bool pressed_level = k_active_low[k] ? false : true;
    const bool idle_level    = !pressed_level;
    uint32_t t = g_fake_now_ms + 10U;

    f->key_fp[k].level = pressed_level;
    advance_to(t);        app_tick(&f->app, t);        /* 候选=按下 */
    t += 25U; advance_to(t); app_tick(&f->app, t);     /* 按下沿确认 */
    f->key_fp[k].level = idle_level;
    t += 10U; advance_to(t); app_tick(&f->app, t);     /* 候选=抬起 */
    t += 25U; advance_to(t); app_tick(&f->app, t);     /* 抬起确认→短按触发 */
}

/* 判断某 LED 当前是否处于 BLINK：向后推进一个整周期看电平是否翻转。 */
static bool led_is_blinking(led_t *led, fake_pin_t *fp)
{
    const bool before = fp->level;
    uint32_t t = g_fake_now_ms;
    int i;
    for (i = 0; i < 1200; ++i) {   /* 覆盖到最长 800ms 相位 + 裕量 */
        t += 1U; advance_to(t); led_tick(led, t);
        if (fp->level != before) {
            return true;
        }
    }
    return false;
}

static void test_app_led1_cycle(void)
{
    printf("[app] KEY1: 默认心跳 -> OFF -> ON -> BLINK -> OFF\n");
    app_fixture_t f;
    fixture_init(&f);

    /* 上电默认 = 心跳闪烁 */
    CHECK(led_is_blinking(&f.led[0], &f.led_fp[0]), "默认态 LED1 闪烁");

    press(&f, K1); CHECK(f.led_fp[0].level == false, "1按: OFF");
    press(&f, K1); CHECK(f.led_fp[0].level == true,  "2按: ON(高)");
    press(&f, K1); CHECK(led_is_blinking(&f.led[0], &f.led_fp[0]), "3按: BLINK");
    press(&f, K1); CHECK(f.led_fp[0].level == false, "4按: 回到 OFF");
}

static void test_app_led2_cycle(void)
{
    printf("[app] KEY2: OFF -> ON -> BLINK(200/800) -> BLINK(800/200) -> OFF\n");
    app_fixture_t f;
    fixture_init(&f);

    CHECK(f.led_fp[1].level == false, "默认: OFF");
    press(&f, K2); CHECK(f.led_fp[1].level == true, "1按: ON");
    press(&f, K2); CHECK(led_is_blinking(&f.led[1], &f.led_fp[1]), "2按: BLINK200/800");
    press(&f, K2); CHECK(led_is_blinking(&f.led[1], &f.led_fp[1]), "3按: BLINK800/200");
    press(&f, K2); CHECK(f.led_fp[1].level == false, "4按: OFF");
}

static void test_app_bicolor_cycle(void)
{
    printf("[app] KEY3: OFF -> GREEN -> RED -> YELLOW -> YELLOW BLINK -> OFF\n");
    app_fixture_t f;
    fixture_init(&f);
    fake_pin_t *g = &f.led_fp[2], *r = &f.led_fp[3];

    CHECK(g->level == false && r->level == false, "默认: OFF");
    press(&f, K3); CHECK(g->level == true  && r->level == false, "1按: GREEN");
    press(&f, K3); CHECK(g->level == false && r->level == true,  "2按: RED");
    press(&f, K3); CHECK(g->level == true  && r->level == true,  "3按: YELLOW");
    press(&f, K3);
    CHECK(led_is_blinking(&f.led[2], g), "4按: YELLOW BLINK(绿闪)");
    press(&f, K3); CHECK(g->level == false && r->level == false, "5按: OFF");
}

/* 通道隔离：按 KEY1 不应影响 LED2/双色灯。 */
static void test_app_channels_isolated(void)
{
    printf("[app] 通道相互隔离\n");
    app_fixture_t f;
    fixture_init(&f);

    press(&f, K1);   /* 只动 LED1 */
    CHECK(f.led_fp[1].level == false, "KEY1 不影响 LED2");
    CHECK(f.led_fp[2].level == false && f.led_fp[3].level == false,
          "KEY1 不影响双色灯");
}

int main(void)
{
    printf("==== KEY+LED 框架主机测试 ====\n");

    test_button_debounce_rejects_glitch();
    test_button_short_press();
    test_button_long_press_no_event();
    test_led_blink_timing();
    test_led_steady_ignores_tick();
    test_bicolor_colors();
    test_bicolor_yellow_blink_in_phase();
    test_app_led1_cycle();
    test_app_led2_cycle();
    test_app_bicolor_cycle();
    test_app_channels_isolated();

    printf("---- 检查 %d 项，失败 %d 项 ----\n", g_checks, g_fails);
    return g_fails == 0 ? 0 : 1;
}
