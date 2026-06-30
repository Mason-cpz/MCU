# 方案 A：新增 BSP 组合层（保留 chip-agnostic 接缝）

> 工程根：`STM32_template_A`。本计划只在本工程内操作。
> 目标：把"装配仪式"从 main.c 抽到新的 bsp 组合层，main.c 收成两行；
> **app 层保持 chip-agnostic**（不出现 GPIO_TypeDef、不 include stm32f1xx_hal/board 头）。
> 这是 A/B 对比里的 **A**：保留 `app_dependencies_t` 接缝。

## 背景

当前 main.c 的 `USER CODE BEGIN 2` 里堆了：board 实例、app 实例、三张配置表、
`stm32f1_board_init`、手搓 `app_dependencies_t deps`、`app_init`。每加一种外设，这套装配
还要再来一遍。把它收敛到一个 bsp 组合层，main.c 永远只调 `bsp_init()` / `bsp_tick()`。

## 目标形态

main.c 的用户代码区：
```c
/* USER CODE BEGIN 2 */
bsp_init();
/* USER CODE END 2 */
/* ... while(1) ... */
/* USER CODE BEGIN 3 */
bsp_tick();
```

## 范围

新增：
- `include/bsp/bsp.h`
- `src/bsp/bsp.c`

主改：
- `Core/Src/main.c`（删装配，改两行调用）
- `MDK-ARM/STM32_template.uvprojx`（注册 bsp.c）

**不动**：`app.*`（A 方案 app 接口完全不变）、`led.*`、`bicolor_led.*`、`stm32f1_board.*`、
`hal/*`、`gpio_port.*`、`timebase.*`、`button.*`、`Core/Inc/main.h` 引脚宏。

## 设计细节

### 1. 新增 `include/bsp/bsp.h`
```c
#ifndef BSP_BSP_H
#define BSP_BSP_H

/* 板级支持包：持有 board/app 实例与全部配置表，对外只露两个入口。
 * 加外设、改配置都进 bsp.c，main.c 不再改动。 */
void bsp_init(void);
void bsp_tick(void);

#endif
```

### 2. 新增 `src/bsp/bsp.c`（装配从 main.c 整体搬来）
```c
#include "bsp/bsp.h"

#include "app/app.h"
#include "platform/stm32f1_board.h"
#include "main.h"   /* LEDx_Pin / LEDx_GPIO_Port */

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

    const app_dependencies_t deps = {
        .timebase       = stm32f1_board_timebase(&board),
        .led_pins       = stm32f1_board_led_pins(&board),
        .led_polarities = stm32f1_board_led_polarities(&board),
        .led_count      = stm32f1_board_led_count(&board),
        .behaviors      = led_behaviors,
        .bicolor        = &bicolor_desc,
    };
    app_init(&app, &deps);
}

void bsp_tick(void)
{
    app_tick(&app);
}
```

### 3. 改 `Core/Src/main.c`
- `USER CODE BEGIN Includes`：删 `#include "app/app.h"` 和 `#include "platform/stm32f1_board.h"`，
  改为 `#include "bsp/bsp.h"`。
- `USER CODE BEGIN PV`：**删除** `board`、`app` 两个 static 实例，以及 `board_leds[]`、
  `led_behaviors[]`、`bicolor_desc` 三张表（全部搬进 bsp.c）。
- `USER CODE BEGIN 2`：删掉 `stm32f1_board_init`/`deps`/`app_init` 整段，替换为 `bsp_init();`。
- `USER CODE BEGIN 3`：`app_tick(&app);` 改为 `bsp_tick();`。
- 其余（HAL_Init、时钟、MX_GPIO_Init、MX_USART1_UART_Init）不动。

### 4. 构建工程同步 `MDK-ARM/STM32_template.uvprojx`
在 `Application/User/Framework` 组内新增一个 `<File>` 块：
```xml
<File>
  <FileName>bsp.c</FileName>
  <FileType>1</FileType>
  <FilePath>../src/bsp/bsp.c</FilePath>
</File>
```

## 验收标准（评审逐条核对真实代码）
1. `include/bsp/bsp.h` 存在，只声明 `bsp_init` / `bsp_tick`。
2. `src/bsp/bsp.c` 持有 board/app 实例 + 三张配置表，内部用 `app_dependencies_t` 装配。
3. `main.c` 的 `USER CODE BEGIN 2` 仅 `bsp_init();`；`USER CODE BEGIN 3` 仅 `bsp_tick();`；
   PV 区无 board/app/配置表残留；includes 改为 `bsp/bsp.h`。
4. **app 层未改**：`app.h`/`app.c` 与改造前一致，仍 chip-agnostic（无 GPIO_TypeDef）。
5. uvprojx 已注册 bsp.c。
6. demo 行为不变：LED1 常亮、LED2 闪烁、LED3+LED4 闪烁黄。
7. 未改 led.*、bicolor_led.*、stm32f1_board.*、hal/*、gpio_port.*、timebase.*、button.*、main.h。
8. Keil 编译 `0 Error(s), 0 Warning(s)`：
   `D:\6.IED\Keil_v5\UV4\UV4.exe -b E:\Workspace\mcu\mcu_discussion\BSP_HAL_APP\STM32_template_A\MDK-ARM\STM32_template.uvprojx -j0`

## 风险点（人工重点盯）
- **main.c 残留**：三张表和 board/app 实例必须从 main.c **彻底删除**，不能两边各留一份
  （否则重复定义 / 链接冲突）。评审重点查 main.c 的 PV 区是否干净。
- **app 一字不改**：A 方案的卖点就是 app 接口零改动。若 Codex 顺手动了 app.h/app.c 即偏离，
  REJECT。
- **include 顺序**：bsp.c 需要 `main.h` 拿引脚宏；确认 include 完整、能编过。
