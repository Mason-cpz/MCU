# 03 LED 初始化简化 round 1 改动清单

## 修改文件清单

- `include/platform/stm32f1_board.h`
- `src/platform/stm32f1_board.c`
- `include/app/app.h`
- `src/app/app.c`
- `Core/Src/main.c`

未修改：

- `include/hal/*`
- `src/platform/stm32f1/stm32f1_gpio_port.*`
- `src/platform/stm32f1/stm32f1_timebase.*`
- `include/drivers/led.h`
- `src/drivers/led.c`
- `include/drivers/bicolor_led.h`
- `src/drivers/bicolor_led.c`
- `button.*`
- `Core/Inc/main.h`
- `MDK-ARM/STM32_template.uvprojx`

## 关键改动

### `include/platform/stm32f1_board.h`

- 删除纯中转壳 `stm32f1_board_config_t`。
- `stm32f1_board_init` 改为直接接收 LED 描述表和数量：

```c
void stm32f1_board_init(
    stm32f1_board_t *self,
    const stm32f1_board_led_desc_t *descs,
    uint8_t count);
```

### `src/platform/stm32f1_board.c`

- `stm32f1_board_init` 从 `config->descs/config->count` 改为使用形参 `descs/count`。
- 保留 count assert 与 clamp：

```c
assert(count <= STM32F1_BOARD_LED_MAX);
self->count = count;
if (self->count > STM32F1_BOARD_LED_MAX) {
    self->count = STM32F1_BOARD_LED_MAX;
}
```

- 修复 ACTIVE_LOW 初始化误亮：GPIO 配置为输出前先按极性写“灭”电平。

```c
const GPIO_PinState off_level =
    polarity == LED_ACTIVE_HIGH ? GPIO_PIN_RESET : GPIO_PIN_SET;

stm32f1_board_enable_gpio_clock(port);
HAL_GPIO_WritePin(port, pin, off_level);
```

### `include/app/app.h`

- app 层保持 chip-agnostic：未引入 `stm32f1xx_hal.h`，未出现 `GPIO_TypeDef`。
- 新增行为描述：

```c
typedef enum {
    APP_LED_OFF = 0,
    APP_LED_STEADY_ON,
    APP_LED_BLINK,
} app_led_behavior_t;

typedef struct {
    app_led_behavior_t behavior;
    uint16_t on_ms;
    uint16_t off_ms;
} app_led_behavior_desc_t;
```

- 新增显式双色配置：

```c
typedef struct {
    bool enabled;
    uint8_t green_index;
    uint8_t red_index;
    bicolor_color_t color;
    bool blink;
    uint16_t on_ms;
    uint16_t off_ms;
} app_bicolor_desc_t;
```

- `app_dependencies_t` 保留，并增加行为表和双色配置指针：

```c
const app_led_behavior_desc_t *behaviors;
const app_bicolor_desc_t *bicolor;
```

### `src/app/app.c`

- 按 `deps->behaviors[i]` 初始化每个 LED 行为：
  - `APP_LED_STEADY_ON` -> `led_set(..., LED_ON)`
  - `APP_LED_BLINK` -> `led_set_blink(..., on_ms, off_ms, now_ms)`
  - `APP_LED_OFF` -> 保持 `led_init` 后的灭灯状态
- 双色改为显式 index 组合，并支持可选 NULL：

```c
if (deps->bicolor != NULL &&
    deps->bicolor->enabled &&
    deps->bicolor->green_index < self->led_count &&
    deps->bicolor->red_index < self->led_count) {
    ...
}
```

- `app_tick` 保持循环 `led_tick()`，STEADY 灯继续 no-op。

### `Core/Src/main.c`

- 保留硬件表 `board_leds[]`。
- 删除 `stm32f1_board_config_t board_config`。
- 新增行为表 `led_behaviors[]`：

```c
static const app_led_behavior_desc_t led_behaviors[] = {
  { APP_LED_STEADY_ON, 0U, 0U },
  { APP_LED_BLINK, 500U, 500U },
  { APP_LED_OFF, 0U, 0U },
  { APP_LED_OFF, 0U, 0U },
};
```

- 新增双色配置 `bicolor_desc`：

```c
static const app_bicolor_desc_t bicolor_desc = {
  .enabled = true,
  .green_index = 2U,
  .red_index = 3U,
  .color = BICOLOR_YELLOW,
  .blink = true,
  .on_ms = 500U,
  .off_ms = 500U,
};
```

- `stm32f1_board_init` 直接使用 `board_leds[] + count`：

```c
stm32f1_board_init(
  &board,
  board_leds,
  sizeof(board_leds) / sizeof(board_leds[0]));
```

- `app_dependencies_t deps` 增加：

```c
.behaviors = led_behaviors,
.bicolor = &bicolor_desc,
```

## 行为保持

- LED1：常亮
- LED2：500ms / 500ms 闪烁
- LED3 + LED4：双色黄，500ms / 500ms 闪烁

## 验证

### 静态搜索

- `include/app` 和 `src/app` 内搜索 `GPIO_TypeDef` / `stm32f1xx_hal` / `.port`：无匹配。
- 源码内搜索 `stm32f1_board_config_t` / `board_config`：无匹配。
- `MDK-ARM/STM32_template.uvprojx` 未修改。

### Keil 编译

执行：

```text
D:\6.IED\Keil_v5\UV4\UV4.exe -b E:\Workspace\mcu\mcu_discussion\BSP_HAL_APP\STM32_template\MDK-ARM\STM32_template.uvprojx -j0
```

结果：

```text
"STM32_template\STM32_template.axf" - 0 Error(s), 0 Warning(s).
```

已生成：

- `MDK-ARM/STM32_template/STM32_template.axf`
- `MDK-ARM/STM32_template/STM32_template.hex`
