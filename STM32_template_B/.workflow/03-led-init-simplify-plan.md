# LED 初始化简化计划（修订版 · 保分层）

> 本文件由 Claude 重写，覆盖 Codex 初版。核心修正：**app 层保持 chip-agnostic，
> 绝不引入 `GPIO_TypeDef`**。Codex 初版用 app 耦合 STM32 换单表配置，会作废已建立的
> HAL 抽象边界（换芯片要改 app），不采纳。本版用「main.c 双表 + 去掉中转壳 + 修
> ACTIVE_LOW」达到同样的「加灯只改 main.c」效果，且不破坏分层。

## 背景问题

1. 初始化链路里有一个**纯中转壳** `stm32f1_board_config_t`，只是把 `{descs, count}`
   包一层再传给 `stm32f1_board_init`，可以去掉。
2. LED 的「行为」（常亮 / 闪烁 / 双色）目前**硬编码在 `app.c` 的 demo 里**（`leds[0]`
   blink、`leds[1]` on、`leds[2/3]` bicolor）。改行为要进 app.c 改逻辑，不够集中。
3. `stm32f1_board.c` 的 GPIO 初始化固定先写 `GPIO_PIN_RESET`，对 **active-low** 灯会在
   board_init → led_init 之间短暂点亮（真 bug）。

## 目标

- 加 / 改 LED（引脚、极性、默认行为）只改 `Core/Src/main.c` 的两张表。
- **app 层不出现任何 STM32 类型**（不 include `stm32f1xx_hal.h`，不见 `GPIO_TypeDef`）。
- 去掉冗余的 `stm32f1_board_config_t`。
- 顺手修掉 active-low 上电误亮。

## 不做（与 Codex 初版的关键区别）

- **不**把 `GPIO_TypeDef` / `port` 放进 `app.h` 或 `app_led_desc_t`。
- **不**让 `app_init` 做 GPIO 硬件初始化（硬件初始化留在 platform/board 层）。
- **不**把双色做成 per-LED 的 `BEHAVIOR_BICOLOR_GREEN/RED` 再「找第一组」配对；双色是
  两个灯的关系，用显式配置表达。
- **保留** `app_dependencies_t`：它是 app 的依赖注入接缝（全是 chip-agnostic 类型），
  不是中转壳，不删。

## 范围

主改：
- `include/platform/stm32f1_board.h`、`src/platform/stm32f1_board.c`（去 config 壳 + 修极性）
- `include/app/app.h`、`src/app/app.c`（行为表驱动 + 显式双色，仍 chip-agnostic）
- `Core/Src/main.c`（两张表 + 简化装配）

不动：
- `include/hal/*`、`src/platform/stm32f1/stm32f1_gpio_port.*`、`stm32f1_timebase.*`
- `include/drivers/led.*`、`bicolor_led.*`、`button.*`
- `Core/Inc/main.h` 引脚宏
- `MDK-ARM/STM32_template.uvprojx`（无新增 / 删除 .c 文件，构建工程不用动）

## 设计细节

### 1. board 层：去掉 `stm32f1_board_config_t`，直接收参数

`stm32f1_board.h`：保留 `stm32f1_board_led_desc_t`（chip 层类型，`{port, pin, polarity}`），
**删除** `stm32f1_board_config_t`。`init` 签名改为直接收数组 + count：

```c
void stm32f1_board_init(
    stm32f1_board_t *self,
    const stm32f1_board_led_desc_t *descs,
    uint8_t count);
```

`stm32f1_board.c`：`stm32f1_board_init` 内部逻辑不变（clamp count、循环 init_led、存
polarities、init timebase），只是参数来源从 `config->descs/config->count` 改成形参
`descs/count`。其余访问函数（`led_count` / `led_pins` / `led_polarities` / `timebase`）保持不变。

### 2. board 层：修 ACTIVE_LOW 上电误亮

`stm32f1_board_gpio_output_init` 增加 `polarity` 参数，按极性写「灭」电平：

```c
static void stm32f1_board_gpio_output_init(
    GPIO_TypeDef *port, uint16_t pin, led_polarity_t polarity)
{
    GPIO_InitTypeDef gpio = {0};
    GPIO_PinState off_level =
        (polarity == LED_ACTIVE_HIGH) ? GPIO_PIN_RESET : GPIO_PIN_SET;

    stm32f1_board_enable_gpio_clock(port);
    HAL_GPIO_WritePin(port, pin, off_level);   /* 先把电平摆到「灭」，再配方向 */

    gpio.Pin = pin; gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL; gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(port, &gpio);
}
```

`stm32f1_board_init_led` 把 `desc.polarity` 透传进去。`led_init` 后续仍会 `led_apply(LED_OFF)`，
两处一致，无冲突。

### 3. app 层：chip-agnostic 行为表（**不含任何 GPIO 类型**）

`app.h` 新增行为枚举与描述（注意：**没有 port/pin/GPIO_TypeDef**）：

```c
typedef enum {
    APP_LED_OFF = 0,
    APP_LED_STEADY_ON,
    APP_LED_BLINK,
} app_led_behavior_t;

typedef struct {
    app_led_behavior_t behavior;
    uint16_t on_ms;      /* 仅 BLINK 有效 */
    uint16_t off_ms;     /* 仅 BLINK 有效 */
} app_led_behavior_desc_t;

/* 双色：显式指定哪两个通道组合、初始颜色、是否闪烁。不复用 per-led behavior。 */
typedef struct {
    bool     enabled;
    uint8_t  green_index;
    uint8_t  red_index;
    bicolor_color_t color;
    bool     blink;
    uint16_t on_ms;
    uint16_t off_ms;
} app_bicolor_desc_t;
```

`app_dependencies_t` 在原有 chip-agnostic 字段基础上，增加两张「行为」表的指针：

```c
typedef struct {
    const hal_timebase_t       *timebase;
    const hal_gpio_pin_t       *led_pins;          /* 板级 pins 数组基址（不变） */
    const led_polarity_t       *led_polarities;    /* 板级 polarities 基址（不变） */
    uint8_t                     led_count;
    const app_led_behavior_desc_t *behaviors;      /* 与 led_pins 按 index 对齐，长度 led_count */
    const app_bicolor_desc_t   *bicolor;           /* 可为 NULL；NULL 表示无双色 */
} app_dependencies_t;
```

`app_t` 不变（`leds[APP_LED_MAX]` / `led_count` / `bicolor` / `has_bicolor`）。

### 4. app_init：按行为表驱动（替代硬编码 demo）

```c
void app_init(app_t *self, const app_dependencies_t *deps)
{
    uint8_t i;
    const uint32_t now_ms = hal_millis(deps->timebase);

    self->timebase  = deps->timebase;
    self->led_count = deps->led_count;
    if (self->led_count > APP_LED_MAX) self->led_count = APP_LED_MAX;
    self->has_bicolor = false;

    for (i = 0U; i < self->led_count; ++i) {
        led_init(&self->leds[i], &deps->led_pins[i], deps->led_polarities[i]);

        switch (deps->behaviors[i].behavior) {
        case APP_LED_STEADY_ON:
            led_set(&self->leds[i], LED_ON);
            break;
        case APP_LED_BLINK:
            led_set_blink(&self->leds[i],
                deps->behaviors[i].on_ms, deps->behaviors[i].off_ms, now_ms);
            break;
        case APP_LED_OFF:
        default:
            /* led_init 已置灭 */
            break;
        }
    }

    /* 双色显式组合：覆盖对应两个通道的行为 */
    if (deps->bicolor != NULL && deps->bicolor->enabled &&
        deps->bicolor->green_index < self->led_count &&
        deps->bicolor->red_index   < self->led_count) {
        bicolor_led_init(&self->bicolor,
            &self->leds[deps->bicolor->green_index],
            &self->leds[deps->bicolor->red_index]);
        if (deps->bicolor->blink) {
            bicolor_led_blink(&self->bicolor, deps->bicolor->color,
                deps->bicolor->on_ms, deps->bicolor->off_ms, now_ms);
        } else {
            bicolor_led_set(&self->bicolor, deps->bicolor->color);
        }
        self->has_bicolor = true;
    }
}
```

`app_tick` 不变（仍循环 `led_tick` 全部灯，STEADY 是 no-op）。

> 行为表与双色表都按 index 对齐 `led_pins`；双色覆盖 green/red 两个通道，所以这两个
> index 在行为表里填 `APP_LED_OFF` 即可（会被双色覆盖，填什么都行，但 OFF 最清晰）。

### 5. main.c：两张表 + 简化装配

```c
static const stm32f1_board_led_desc_t board_leds[] = {   /* 硬件接线（chip 层类型） */
  { LED1_GPIO_Port, LED1_Pin, LED_ACTIVE_HIGH },
  { LED2_GPIO_Port, LED2_Pin, LED_ACTIVE_HIGH },
  { LED3_GPIO_Port, LED3_Pin, LED_ACTIVE_HIGH },
  { LED4_GPIO_Port, LED4_Pin, LED_ACTIVE_HIGH },
};

static const app_led_behavior_desc_t led_behaviors[] = { /* 行为（chip-agnostic） */
  { APP_LED_STEADY_ON, 0U,   0U   },   /* LED1 常亮 */
  { APP_LED_BLINK,     500U, 500U },   /* LED2 闪烁 */
  { APP_LED_OFF,       0U,   0U   },   /* LED3 → 交给双色 */
  { APP_LED_OFF,       0U,   0U   },   /* LED4 → 交给双色 */
};

static const app_bicolor_desc_t bicolor_desc = {         /* LED3 绿 + LED4 红，闪烁黄 */
  .enabled = true, .green_index = 2U, .red_index = 3U,
  .color = BICOLOR_YELLOW, .blink = true, .on_ms = 500U, .off_ms = 500U,
};
```

装配（`board_config` 与 `deps` 里多个独立取值调用消失，改为）：

```c
stm32f1_board_init(&board, board_leds, sizeof(board_leds)/sizeof(board_leds[0]));

const app_dependencies_t deps = {
  .timebase       = stm32f1_board_timebase(&board),
  .led_pins       = stm32f1_board_led_pins(&board),
  .led_polarities = stm32f1_board_led_polarities(&board),
  .led_count      = stm32f1_board_led_count(&board),
  .behaviors      = led_behaviors,
  .bicolor        = &bicolor_desc,
};
app_init(&app, &deps);
```

> 加一个灯 = board_leds[] 加一行 + led_behaviors[] 加一行（两表 index 对齐）。两张表都在
> main.c，app 层无 STM32 依赖。

## 验收标准（评审逐条核对真实代码）

1. `app.h` / `app.c` **不** include `stm32f1xx_hal.h`，**不**出现 `GPIO_TypeDef`、`port` 字段。
2. `app.h` 有 `app_led_behavior_t` / `app_led_behavior_desc_t` / `app_bicolor_desc_t`，且双色用显式 index 组合（非 per-led BICOLOR 枚举）。
3. `stm32f1_board_config_t` 已删除；`stm32f1_board_init` 直接收 `(self, descs, count)`。
4. `stm32f1_board_gpio_output_init` 按 polarity 写「灭」电平（ACTIVE_LOW → `GPIO_PIN_SET`）。
5. `main.c` 有 `board_leds[]` + `led_behaviors[]` + `bicolor_desc` 三处静态表，装配里无 `stm32f1_board_config_t`。
6. demo 行为不变：LED1 常亮、LED2 闪烁、LED3+LED4 闪烁黄。
7. `app_dependencies_t` 保留且全为 chip-agnostic 类型 + 两张行为表指针。
8. 未改 `hal/*`、`stm32f1_gpio_port.*`、`stm32f1_timebase.*`、`led.*`、`bicolor_led.*`、`button.*`、`main.h` 引脚宏、uvprojx。
9. Keil 编译 `0 Error(s), 0 Warning(s)`，生成 axf/hex。

## 风险点（人工重点盯）

- **分层红线**：本轮的全部意义就是「app 不碰 STM32」。评审必查 `app.h`/`app.c` 没有任何
  STM32 类型渗入。这是与 Codex 初版的根本分歧，**一旦 GPIO_TypeDef 进了 app 就算 REJECT**。
- **两表 index 对齐**：`board_leds[]` 与 `led_behaviors[]` 必须等长且按 index 一一对应；
  双色的 `green_index/red_index` 要落在 `[0, led_count)`，且指向的两个灯在行为表里应为 OFF
  （否则单灯行为会和双色抢，虽然双色覆盖在后、结果仍对，但语义混乱）。
- **ACTIVE_LOW 修复别只改一半**：`gpio_output_init` 写灭电平 + `led_init` 的 `led_apply(LED_OFF)`
  两处都基于 polarity，确认换 LOW 极性后上电不闪一下。当前 demo 全 HIGH，需在 review 时
  心算一遍 LOW 路径。
- **NULL 双色**：`deps->bicolor` 允许为 NULL（无双色场景），`app_init` 必须先判 NULL 再解引用。
