# 方案 B：新增 BSP 组合层 + 砍掉 chip-agnostic 接缝

> 工程根：`STM32_template_B`。本计划只在本工程内操作。
> 目标：把"装配仪式"从 main.c 抽到新的 bsp 组合层，main.c 收成两行；
> **同时砍掉 `app_dependencies_t` 接缝**，app_init 直接吃 board，省掉 deps 那几行装配。
> 代价：**app 层从此耦合 STM32F1**（app.h include board 头、出现 GPIO 类型）。
> 这是 A/B 对比里的 **B**：极简优先，牺牲分层纯度与可移植性。

## 背景

与 A 同样要把装配从 main.c 抽到 bsp 层。区别：A 保留 `app_dependencies_t`（全 chip-agnostic
类型）；B 删掉它，让 `app_init` 直接收 `stm32f1_board_t*` + 两张行为表，省去手搓 deps 的
6 行 bundling。这样能直观对比"省那几行" vs "app 绑死 STM32"的取舍。

## 目标形态

main.c 的用户代码区与 A 完全相同：
```c
/* USER CODE BEGIN 2 */
bsp_init();
/* USER CODE END 2 */
/* USER CODE BEGIN 3 */
bsp_tick();
```
差异全在 app 接口与 bsp.c 的装配写法。

## 范围

新增：
- `include/bsp/bsp.h`
- `src/bsp/bsp.c`

主改：
- `include/app/app.h`（删 `app_dependencies_t`，app_init 改签名，引入 board 类型）
- `src/app/app.c`（从 board 取 pins/polarities/count，不再走 deps）
- `Core/Src/main.c`（删装配，改两行调用）
- `MDK-ARM/STM32_template.uvprojx`（注册 bsp.c）

**不动**：`led.*`、`bicolor_led.*`、`stm32f1_board.*`、`hal/*`、`gpio_port.*`、`timebase.*`、
`button.*`、`Core/Inc/main.h` 引脚宏。

## 设计细节

### 1. 新增 `include/bsp/bsp.h`（与 A 相同）
```c
#ifndef BSP_BSP_H
#define BSP_BSP_H
void bsp_init(void);
void bsp_tick(void);
#endif
```

### 2. 改 `include/app/app.h`：删接缝、改签名
- **删除** `app_dependencies_t` 整个结构体。
- 新增 include：`#include "platform/stm32f1_board.h"`（B 方案 app 显式依赖 board）。
  保留 `app_led_behavior_t` / `app_led_behavior_desc_t` / `app_bicolor_desc_t`（chip-agnostic，
  这几个不变，仍由 bsp.c 填表）。
- `app_t` 不变（`leds[APP_LED_MAX]` / `led_count` / `bicolor` / `has_bicolor` / `timebase`）。
- `app_init` 新签名：
```c
void app_init(
    app_t *self,
    const stm32f1_board_t *board,
    const app_led_behavior_desc_t *behaviors,   /* 与 board 内 LED 按 index 对齐 */
    const app_bicolor_desc_t *bicolor);          /* 可为 NULL */
```
- `app_tick(app_t *self)` 不变。

### 3. 改 `src/app/app.c`：从 board 直接取
`app_init` 内部改为从 board 取数据（替代原 deps 字段）：
```c
void app_init(app_t *self, const stm32f1_board_t *board,
              const app_led_behavior_desc_t *behaviors,
              const app_bicolor_desc_t *bicolor)
{
    uint8_t i;
    const hal_timebase_t *timebase = stm32f1_board_timebase(board);
    const hal_gpio_pin_t *pins     = stm32f1_board_led_pins(board);
    const led_polarity_t *pols      = stm32f1_board_led_polarities(board);
    const uint32_t now_ms = hal_millis(timebase);

    self->timebase = timebase;
    self->led_count = stm32f1_board_led_count(board);
    if (self->led_count > APP_LED_MAX) self->led_count = APP_LED_MAX;
    self->has_bicolor = false;

    for (i = 0U; i < self->led_count; ++i) {
        led_init(&self->leds[i], &pins[i], pols[i]);
        switch (behaviors[i].behavior) {
        case APP_LED_STEADY_ON: led_set(&self->leds[i], LED_ON); break;
        case APP_LED_BLINK:
            led_set_blink(&self->leds[i], behaviors[i].on_ms, behaviors[i].off_ms, now_ms);
            break;
        case APP_LED_OFF: default: break;
        }
    }

    if (bicolor != NULL && bicolor->enabled &&
        bicolor->green_index < self->led_count &&
        bicolor->red_index < self->led_count) {
        bicolor_led_init(&self->bicolor,
            &self->leds[bicolor->green_index], &self->leds[bicolor->red_index]);
        if (bicolor->blink)
            bicolor_led_blink(&self->bicolor, bicolor->color,
                bicolor->on_ms, bicolor->off_ms, now_ms);
        else
            bicolor_led_set(&self->bicolor, bicolor->color);
        self->has_bicolor = true;
    }
}
```
`app_tick` 不变。注意：app.c 现在依赖 `stm32f1_board_*` 访问函数（已经在 board.h 里），不需要
新增 board 接口。

### 4. 新增 `src/bsp/bsp.c`（无 deps，直接传 board）
```c
#include "bsp/bsp.h"
#include "app/app.h"
#include "platform/stm32f1_board.h"
#include "main.h"

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
    .enabled = true, .green_index = 2U, .red_index = 3U,
    .color = BICOLOR_YELLOW, .blink = true, .on_ms = 500U, .off_ms = 500U,
};

void bsp_init(void)
{
    stm32f1_board_init(&board, board_leds,
        sizeof(board_leds) / sizeof(board_leds[0]));
    app_init(&app, &board, led_behaviors, &bicolor_desc);   /* 无 deps */
}

void bsp_tick(void) { app_tick(&app); }
```

### 5. 改 `Core/Src/main.c`（与 A 相同）
- includes 改为 `#include "bsp/bsp.h"`，删 app/board 头。
- PV 区删 board/app 实例与三张表。
- `USER CODE BEGIN 2` → `bsp_init();`；`USER CODE BEGIN 3` → `bsp_tick();`。

### 6. 构建工程同步 `MDK-ARM/STM32_template.uvprojx`
`Application/User/Framework` 组内新增：
```xml
<File>
  <FileName>bsp.c</FileName>
  <FileType>1</FileType>
  <FilePath>../src/bsp/bsp.c</FilePath>
</File>
```

## 验收标准（评审逐条核对真实代码）
1. `include/bsp/bsp.h` 存在，只声明 `bsp_init` / `bsp_tick`。
2. `app.h` **已删除** `app_dependencies_t`；`app_init` 新签名收 `(self, board, behaviors, bicolor)`；
   `app.h` include 了 `platform/stm32f1_board.h`。
3. `app.c` 从 `stm32f1_board_*` 访问函数取 pins/polarities/count/timebase，不再有 deps。
4. `src/bsp/bsp.c` 装配里 **无** `app_dependencies_t`，直接 `app_init(&app, &board, ...)`。
5. `main.c` `USER CODE BEGIN 2` 仅 `bsp_init();`、`BEGIN 3` 仅 `bsp_tick();`，PV 区干净。
6. uvprojx 已注册 bsp.c。
7. demo 行为不变：LED1 常亮、LED2 闪烁、LED3+LED4 闪烁黄。
8. 未改 led.*、bicolor_led.*、stm32f1_board.*、hal/*、gpio_port.*、timebase.*、button.*、main.h。
9. Keil 编译 `0 Error(s), 0 Warning(s)`：
   `D:\6.IED\Keil_v5\UV4\UV4.exe -b E:\Workspace\mcu\mcu_discussion\BSP_HAL_APP\STM32_template_B\MDK-ARM\STM32_template.uvprojx -j0`

## 风险点（人工重点盯）
- **这是有意为之的耦合**：B 方案 app.h include board 头、app 绑死 STM32 是**预期结果**，
  不是 bug。评审时确认它确实砍掉了 deps（而不是又偷偷保留），这才是 B 与 A 的区别点。
- **main.c 残留**：三张表与 board/app 实例必须从 main.c 彻底删除。
- **行为表 index 对齐**：`led_behaviors[]` 必须与 `board_leds[]` 等长、按 index 对应；
  双色 index 落在 `[0, led_count)`。
- **NULL 双色**：`app_init` 里先判 `bicolor != NULL` 再解引用。
