# 04-bsp-A round 1 改动清单

## 改动文件

- `include/bsp/bsp.h`
  - 新增 BSP 组合层对外入口，只暴露 `bsp_init()` / `bsp_tick()`。
- `src/bsp/bsp.c`
  - 新增 BSP 组合层实现。
  - 从 `main.c` 搬入 `board` / `app` 实例、`board_leds[]`、`led_behaviors[]`、`bicolor_desc`。
  - 在 `bsp_init()` 内完成 `stm32f1_board_init()`、`app_dependencies_t` 装配和 `app_init()`。
  - 在 `bsp_tick()` 内转调 `app_tick(&app)`。
- `Core/Src/main.c`
  - `USER CODE BEGIN Includes` 改为只 include `bsp/bsp.h`。
  - `USER CODE BEGIN PV` 删除 board/app 实例和三张配置表。
  - `USER CODE BEGIN 2` 收敛为 `bsp_init();`。
  - `USER CODE BEGIN 3` 收敛为 `bsp_tick();`。
- `MDK-ARM/STM32_template.uvprojx`
  - 在 `Application/User/Framework` 组注册 `../src/bsp/bsp.c`。

## 关键改动片段

### `include/bsp/bsp.h`

```c
#ifndef BSP_BSP_H
#define BSP_BSP_H

/* 板级支持包：持有 board/app 实例与全部配置表，对外只露两个入口。
 * 加外设、改配置都进 bsp.c，main.c 不再改动。 */
void bsp_init(void);
void bsp_tick(void);

#endif
```

### `src/bsp/bsp.c`

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

    const app_dependencies_t deps = {
        .timebase = stm32f1_board_timebase(&board),
        .led_pins = stm32f1_board_led_pins(&board),
        .led_polarities = stm32f1_board_led_polarities(&board),
        .led_count = stm32f1_board_led_count(&board),
        .behaviors = led_behaviors,
        .bicolor = &bicolor_desc,
    };

    app_init(&app, &deps);
}

void bsp_tick(void)
{
    app_tick(&app);
}
```

### `Core/Src/main.c`

```c
/* USER CODE BEGIN Includes */
#include "bsp/bsp.h"

/* USER CODE END Includes */
```

```c
/* USER CODE BEGIN PV */

/* USER CODE END PV */
```

```c
/* USER CODE BEGIN 2 */
bsp_init();

/* USER CODE END 2 */
```

```c
/* USER CODE BEGIN 3 */
bsp_tick();
```

### `MDK-ARM/STM32_template.uvprojx`

```xml
<File>
  <FileName>bsp.c</FileName>
  <FileType>1</FileType>
  <FilePath>../src/bsp/bsp.c</FilePath>
</File>
```

## 验证

### Keil 编译

执行命令：

```powershell
& 'D:\6.IED\Keil_v5\UV4\UV4.exe' -b 'E:\Workspace\mcu\mcu_discussion\BSP_HAL_APP\STM32_template_A\MDK-ARM\STM32_template.uvprojx' -j0
```

结果：

- `MDK-ARM\STM32_template_A\STM32_template_A.build_log.htm` 显示 `compiling bsp.c...`
- `MDK-ARM\STM32_template_A\STM32_template_A.build_log.htm` 显示 `"STM32_template_A\STM32_template_A.axf" - 0 Error(s), 0 Warning(s).`

### 静态核对

- `Core/Src/main.c` 中搜索以下旧装配残留，无输出：
  - `app/app.h`
  - `platform/stm32f1_board.h`
  - `board_leds`
  - `led_behaviors`
  - `bicolor_desc`
  - `stm32f1_board_init`
  - `app_tick(&app)`
- `include/app/app.h`、`src/app/app.c` 中搜索以下 STM32 相关内容，无输出：
  - `GPIO_TypeDef`
  - `stm32f1xx_hal`
  - `platform/stm32f1_board`

## 未做/跳过/风险项

- 未修改 `app.h` / `app.c`，保留 A 方案的 `app_dependencies_t` 接缝。
- 未修改 `led.*`、`bicolor_led.*`、`stm32f1_board.*`、`hal/*`、`gpio_port.*`、`timebase.*`、`button.*`、`Core/Inc/main.h`。
- 本地只完成 Keil 编译验证；LED1 常亮、LED2 闪烁、LED3+LED4 闪烁黄仍需上板观察确认。
