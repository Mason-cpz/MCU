# 04-bsp-B round 1 改动清单

## 改动文件

- `include/bsp/bsp.h`
- `src/bsp/bsp.c`
- `include/app/app.h`
- `src/app/app.c`
- `Core/Src/main.c`
- `MDK-ARM/STM32_template.uvprojx`

## 关键改动片段

### `include/bsp/bsp.h`

新增 BSP 对外入口，只暴露初始化与周期调度：

```c
#ifndef BSP_BSP_H
#define BSP_BSP_H

void bsp_init(void);
void bsp_tick(void);

#endif
```

### `src/bsp/bsp.c`

新增 BSP 组合层，集中持有 board/app 实例、LED 硬件表、行为表和双色灯描述；`app_init` 直接接收 board，不再构造 `app_dependencies_t`：

```c
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
```

保留 demo 行为：LED1 常亮、LED2 500/500 ms 闪烁、LED3/LED4 作为黄灯 500/500 ms 闪烁。

### `include/app/app.h`

删除 `app_dependencies_t`，B 方案按计划让 app 直接 include STM32F1 board 头：

```c
#include "platform/stm32f1_board.h"

void app_init(
    app_t *self,
    const stm32f1_board_t *board,
    const app_led_behavior_desc_t *behaviors,
    const app_bicolor_desc_t *bicolor);
```

`app_led_behavior_desc_t`、`app_bicolor_desc_t`、`app_t` 保留。

### `src/app/app.c`

`app_init` 改为直接从 board 访问 timebase、LED pin、极性和数量：

```c
const hal_timebase_t *timebase = stm32f1_board_timebase(board);
const hal_gpio_pin_t *pins = stm32f1_board_led_pins(board);
const led_polarity_t *polarities = stm32f1_board_led_polarities(board);
const uint32_t now_ms = hal_millis(timebase);

self->timebase = timebase;
self->led_count = stm32f1_board_led_count(board);
```

LED 初始化改为使用 `behaviors[i]`，双色灯描述改为直接使用 `bicolor` 指针，并保留 `bicolor != NULL` 判空：

```c
led_init(&self->leds[i], &pins[i], polarities[i]);

switch (behaviors[i].behavior) {
case APP_LED_STEADY_ON:
    led_set(&self->leds[i], LED_ON);
    break;
case APP_LED_BLINK:
    led_set_blink(
        &self->leds[i],
        behaviors[i].on_ms,
        behaviors[i].off_ms,
        now_ms);
    break;
case APP_LED_OFF:
default:
    break;
}
```

`app_tick` 未改。

### `Core/Src/main.c`

`main.c` 用户 includes 只保留 BSP 头：

```c
#include "bsp/bsp.h"
```

PV 区已删除 board/app 实例、LED 硬件表、行为表、双色灯描述。

`USER CODE BEGIN 2` 收成：

```c
bsp_init();
```

`USER CODE BEGIN 3` 收成：

```c
bsp_tick();
```

### `MDK-ARM/STM32_template.uvprojx`

在 `Application/User/Framework` 组注册新增源文件：

```xml
<File>
  <FileName>bsp.c</FileName>
  <FileType>1</FileType>
  <FilePath>../src/bsp/bsp.c</FilePath>
</File>
```

## 未触碰范围

本轮未修改计划禁止范围：`led.*`、`bicolor_led.*`、`stm32f1_board.*`、`hal/*`、`gpio_port.*`、`timebase.*`、`button.*`、`Core/Inc/main.h`。

## 验证

执行 Keil 构建：

```powershell
& 'D:\6.IED\Keil_v5\UV4\UV4.exe' -b 'E:\Workspace\mcu\mcu_discussion\BSP_HAL_APP\STM32_template_B\MDK-ARM\STM32_template.uvprojx' -j0
```

构建日志结果：

```text
"STM32_template_B\STM32_template_B.axf" - 0 Error(s), 0 Warning(s).
```

补充静态检查：

- `rg` 未发现 `app_dependencies_t` 残留。
- `uvprojx` 已出现 `../src/bsp/bsp.c` 注册项。
- `main.c` 中 `board_leds`、`led_behaviors`、`bicolor_desc`、`app_t`、`stm32f1_board_t` 装配残留已移除。
