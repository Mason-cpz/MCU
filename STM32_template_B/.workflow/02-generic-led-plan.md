# 通用 LED 框架重构 — 执行计划

## 目标
把"写死正好 4 个 LED"的框架，重构成可覆盖以下场景的通用框架，且增改 LED 只需改一张表：
1. 单个 LED；2. 多个（4 个）状态灯；3. 每个灯独立常亮 / 闪烁；4. 双引脚双色灯（绿 / 红 / 黄 / 灭）。

三项已定的设计决策：
- **编译期静态描述表**管理 LED 集合（数组 + 描述表，不再用 `led1..led4` 字面字段）。
- **闪烁折进 `led_t`**（模式 OFF/ON/BLINK + `on_ms`/`off_ms` + `led_tick`），删除 `status_light` 服务。
- 新增 **`bicolor_led`**，由两个单通道 `led_t` 组合出颜色。
- **极性从 app 移到板级描述表**（接线事实归板级）。
- 兼容现有 4 灯 demo（重写成展示 闪烁 + 常亮 + 双色 的等价 demo）。

## 范围
主改 / 新增：
- `include/drivers/led.h`、`src/drivers/led.c`（折入模式与 tick）
- `include/drivers/bicolor_led.h`、`src/drivers/bicolor_led.c`（**新建**）
- `include/platform/stm32f1_board.h`、`src/platform/stm32f1_board.c`（表驱动 + 极性）
- `include/app/app.h`、`src/app/app.c`（数组化 + 新 demo）
- `Core/Src/main.c`（描述表）
- `MDK-ARM/STM32_template.uvprojx`（注册 bicolor_led.c、移除 status_light.c）

删除：
- `include/services/status_light.h`、`src/services/status_light.c`

**不动**：`include/hal/gpio.h`、`include/hal/timebase.h`、`src/platform/stm32f1/stm32f1_gpio_port.*`、`stm32f1_timebase.*`、`button.*`、`Core/Inc/main.h` 的引脚宏。

## 设计细节

### 1. `led.h` / `led.c` —— 折入模式与闪烁
保留现有枚举 `led_state_t`(OFF=0/ON=1)、`led_polarity_t`(ACTIVE_LOW=0/ACTIVE_HIGH=1)，新增：
```c
typedef enum { LED_MODE_STEADY = 0, LED_MODE_BLINK = 1 } led_mode_t;

typedef struct {
    const hal_gpio_pin_t *pin;
    led_polarity_t polarity;
    led_mode_t     mode;
    led_state_t    state;          /* 当前逻辑亮灭 */
    uint16_t       on_ms;
    uint16_t       off_ms;
    uint32_t       next_toggle_ms;
} led_t;

void        led_init(led_t *self, const hal_gpio_pin_t *pin, led_polarity_t polarity);
void        led_set(led_t *self, led_state_t state);                              /* 强制 STEADY */
void        led_toggle(led_t *self);                                              /* 强制 STEADY，翻转 */
void        led_set_blink(led_t *self, uint16_t on_ms, uint16_t off_ms, uint32_t now_ms);
void        led_tick(led_t *self, uint32_t now_ms);
led_state_t led_is_on(const led_t *self);
```
实现要点（保持 assert 前置检查，沿用 round 1 风格）：
- 抽一个内部 `static void led_apply(led_t*, led_state_t)`：写 `self->state` 并 `hal_gpio_write(pin, gpio_level_for(self,state))`，供 `led_set`/`led_set_blink`/`led_tick` 复用。
- `led_init`：`mode=STEADY; on_ms=off_ms=0; next_toggle_ms=0;` 然后 `led_apply(self, LED_OFF)`。
- `led_set`：`self->mode = LED_MODE_STEADY; led_apply(self, state);`
- `led_toggle`：`led_set(self, self->state == LED_ON ? LED_OFF : LED_ON);`
- `led_set_blink`：`mode=BLINK; on_ms=on_ms; off_ms=off_ms; led_apply(self, LED_ON); next_toggle_ms = now_ms + on_ms;`
- `led_tick`：
  ```c
  if (self->mode != LED_MODE_BLINK) return;
  if ((int32_t)(now_ms - self->next_toggle_ms) < 0) return;
  led_state_t next = (self->state == LED_ON) ? LED_OFF : LED_ON;
  led_apply(self, next);
  self->next_toggle_ms = now_ms + (next == LED_ON ? self->on_ms : self->off_ms);
  ```
- 回绕判断必须用 `(int32_t)(now - next) < 0`，与 `status_light` 原逻辑一致，不要换写法。
- `led_is_on` 返回 `self->state`。

### 2. `bicolor_led.h` / `bicolor_led.c` —— 双色组合（新建）
```c
#include "drivers/led.h"

typedef enum {
    BICOLOR_OFF = 0,
    BICOLOR_GREEN,
    BICOLOR_RED,
    BICOLOR_YELLOW,
} bicolor_color_t;

typedef struct { led_t *green; led_t *red; } bicolor_led_t;

void            bicolor_led_init(bicolor_led_t *self, led_t *green, led_t *red);
void            bicolor_led_set(bicolor_led_t *self, bicolor_color_t color);
bicolor_color_t bicolor_led_get(const bicolor_led_t *self);
```
实现：`bicolor_led_set` 按颜色对两个通道 `led_set`：
- OFF → green OFF, red OFF
- GREEN → green ON, red OFF
- RED → green OFF, red ON
- YELLOW → green ON, red ON

`bicolor_led_get` 用 `led_is_on(green)`/`led_is_on(red)` 反推颜色。assert `self/green/red` 非空。
**本轮只做静态颜色**（通道走 STEADY），不实现"闪烁颜色"，注释里写明可后续扩展。

### 3. 板级表驱动 + 极性下沉（`stm32f1_board.h/.c`）
不再用 `led1..led4` 字面字段，改成描述表 + 数组。`stm32f1_board.h`：
```c
#include "drivers/led.h"   /* 为了 led_polarity_t */

enum { STM32F1_BOARD_LED_MAX = 8 };

typedef struct {
    GPIO_TypeDef   *port;
    uint16_t        pin;
    led_polarity_t  polarity;   /* 极性归板级（接线事实） */
} stm32f1_board_led_desc_t;

typedef struct {
    const stm32f1_board_led_desc_t *descs;   /* 指向 main.c 里的 static const 表 */
    uint8_t count;
} stm32f1_board_config_t;

typedef struct {
    stm32f1_gpio_pin_ctx_t ctx[STM32F1_BOARD_LED_MAX];
    hal_gpio_pin_t         pins[STM32F1_BOARD_LED_MAX];
    led_polarity_t         polarities[STM32F1_BOARD_LED_MAX];
    uint8_t                count;
    hal_timebase_t         timebase;
} stm32f1_board_t;

void                 stm32f1_board_init(stm32f1_board_t *self, const stm32f1_board_config_t *config);
uint8_t              stm32f1_board_led_count(const stm32f1_board_t *self);
const hal_gpio_pin_t *stm32f1_board_led_pin(const stm32f1_board_t *self, uint8_t index);
const hal_gpio_pin_t *stm32f1_board_led_pins(const stm32f1_board_t *self);        /* 数组基址 */
const led_polarity_t *stm32f1_board_led_polarities(const stm32f1_board_t *self);  /* 数组基址 */
const hal_timebase_t *stm32f1_board_timebase(const stm32f1_board_t *self);
```
`stm32f1_board.c`：
- `stm32f1_board_init`：`count = min(config->count, STM32F1_BOARD_LED_MAX)`；assert `config->count <= STM32F1_BOARD_LED_MAX`。`for i in [0,count)`：`stm32f1_board_init_led(&pins[i], &ctx[i], descs[i].port, descs[i].pin)` 并 `polarities[i] = descs[i].polarity`。最后 `stm32f1_timebase_init(&timebase)`。
- 保留现有 `stm32f1_board_enable_gpio_clock` / `stm32f1_board_gpio_output_init` / `stm32f1_board_init_led` 逻辑，只把"四次手写"改成循环。
- `stm32f1_board_led_pin(self, i)`：assert `i < count`，返回 `&self->pins[i]`。

### 4. app 数组化 + 新 demo（`app.h/.c`）
`app.h`：
```c
#include "drivers/led.h"
#include "drivers/bicolor_led.h"
#include "hal/gpio.h"
#include "hal/timebase.h"

enum { APP_LED_MAX = 8 };

typedef struct {
    const hal_timebase_t *timebase;
    const hal_gpio_pin_t *led_pins;        /* 指向板级 pins 数组基址 */
    const led_polarity_t *led_polarities;  /* 指向板级 polarities 数组基址 */
    uint8_t               led_count;
} app_dependencies_t;

typedef struct {
    const hal_timebase_t *timebase;
    led_t        leds[APP_LED_MAX];
    uint8_t      led_count;
    bicolor_led_t bicolor;     /* 由 leds[2]/leds[3] 组合，count>=4 时启用 */
    bool         has_bicolor;
} app_t;

void app_init(app_t *self, const app_dependencies_t *deps);
void app_tick(app_t *self);
```
`app.c`：
- `app_init`：`led_count = min(deps->led_count, APP_LED_MAX)`。`for i in [0,led_count)`：`led_init(&leds[i], &deps->led_pins[i], deps->led_polarities[i])`（极性来自板级，不再写死 `LED_ACTIVE_HIGH`）。
- **新 demo**（在 4 灯时体现三种能力）：
  - `led_set_blink(&leds[0], 500, 500, now)` —— 运行状态灯闪烁（替代原 status_light 的效果）。
  - `if (led_count >= 2) led_set(&leds[1], LED_ON)` —— 常亮。
  - `if (led_count >= 4) { bicolor_led_init(&bicolor, &leds[2], &leds[3]); bicolor_led_set(&bicolor, BICOLOR_YELLOW); has_bicolor = true; }` 否则 `has_bicolor=false`。
- `app_tick`：`for i in [0,led_count) led_tick(&leds[i], now)`。STEADY 灯的 tick 是 no-op，所以双色通道（leds[2]/leds[3]）被无害地一起 tick，不需要单独处理。`now = hal_millis(self->timebase)`。

### 5. `main.c` —— 描述表
把 `board_config` 改成描述表 + count：
```c
static const stm32f1_board_led_desc_t board_leds[] = {
  { LED1_GPIO_Port, LED1_Pin, LED_ACTIVE_HIGH },
  { LED2_GPIO_Port, LED2_Pin, LED_ACTIVE_HIGH },
  { LED3_GPIO_Port, LED3_Pin, LED_ACTIVE_HIGH },
  { LED4_GPIO_Port, LED4_Pin, LED_ACTIVE_HIGH },
};
static const stm32f1_board_config_t board_config = {
  .descs = board_leds,
  .count = sizeof(board_leds) / sizeof(board_leds[0]),
};
```
`deps` 改成：
```c
const app_dependencies_t deps = {
  .timebase       = stm32f1_board_timebase(&board),
  .led_pins       = stm32f1_board_led_pins(&board),
  .led_polarities = stm32f1_board_led_polarities(&board),
  .led_count      = stm32f1_board_led_count(&board),
};
```
其余 main.c（时钟、HAL_Init、while 循环 `app_tick`）不动。

### 6. 构建工程同步（`STM32_template.uvprojx`）
在 `Application/User/Framework` 组内：
- **新增**一个 `<File>` 块（紧跟 led.c 之后即可）：
  ```xml
  <File>
    <FileName>bicolor_led.c</FileName>
    <FileType>1</FileType>
    <FilePath>../src/drivers/bicolor_led.c</FilePath>
  </File>
  ```
- **删除** status_light.c 的 `<File>` 块（`<FileName>status_light.c</FileName>` 那一段，行约 442-446）。

### 7. 删除 status_light
删除 `include/services/status_light.h`、`src/services/status_light.c`。确认全工程无残留 `#include "services/status_light.h"`（仅 app.h/app.c 引用过，已在上面改掉）。

## 验收标准（评审逐条核对真实代码）
1. `led.h` 有 `led_mode_t` 与 `led_set_blink`/`led_tick`；`led_t` 含 `mode/on_ms/off_ms/next_toggle_ms`。
2. `led.c` 的 `led_tick` 用 `(int32_t)(now - next_toggle_ms) < 0` 回绕判断；`led_set`/`led_toggle` 会把 mode 复位成 STEADY；闪烁周期按 on_ms/off_ms 区分。
3. `bicolor_led.h/.c` 存在，四种颜色映射正确（OFF/GREEN/RED/YELLOW → 两通道 off/on 组合）。
4. `stm32f1_board.*` 用描述表 + 数组，**无** `led1..led4` 字面字段；极性来自描述表；`init` 用循环且有 `count <= MAX` 的 assert/clamp。
5. `app.*` 用 `leds[]` 数组 + `led_count`，极性来自 `deps->led_polarities`（**无** 硬编码 `LED_ACTIVE_HIGH`）；demo 体现 闪烁 + 常亮 + 双色黄。
6. `main.c` 用 `board_leds[]` 描述表 + `count`，deps 传数组基址。
7. `status_light.h/.c` 已删除；全工程无残留 include；uvprojx 已移除 status_light.c、新增 bicolor_led.c。
8. 工程可编译：`0 Error(s)`，生成 axf/hex（用 round 1 同一条 UV4 命令）。

## 风险点（人工重点盯）
- **uvprojx 是 XML，手改易坏**：增删 `<File>` 块后务必保持标签配对、缩进一致；改完先确认 Keil 能正常加载工程再编译。这是本轮最易翻车的点。
- **极性下沉别改错语义**：`led_init` 内部 `led_apply(LED_OFF)` 必须在拿到正确 polarity 后执行，保证上电即为"灭"（尤其 ACTIVE_LOW 板子）。当前 demo 全是 ACTIVE_HIGH，但表里要能换 LOW 而不用动 app。
- **双色通道被 tick**：确认 `leds[2]/leds[3]` 设成 STEADY 后 `led_tick` 是 no-op，不会和 `bicolor_led_set` 抢状态。STEADY 直接 return 即可，别在 tick 里动 STEADY 灯。
- **数组越界**：`APP_LED_MAX`/`STM32F1_BOARD_LED_MAX` 与 `count` 的 clamp 要到位，`led_pin(i)` 的 `i<count` assert 不能漏。
- **不要越界改动**：hal 抽象、`stm32f1_gpio_port.*`、`stm32f1_timebase.*`、`button.*`、`main.h` 引脚宏本轮都不动。
