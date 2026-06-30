# 02 generic LED round 1 改动清单

## 修改文件清单
- `include/drivers/led.h`
- `src/drivers/led.c`
- `include/drivers/bicolor_led.h`
- `src/drivers/bicolor_led.c`
- `include/platform/stm32f1_board.h`
- `src/platform/stm32f1_board.c`
- `include/app/app.h`
- `src/app/app.c`
- `Core/Src/main.c`
- `MDK-ARM/STM32_template.uvprojx`
- 删除 `include/services/status_light.h`
- 删除 `src/services/status_light.c`

## 关键改动片段

### `include/drivers/led.h`
- 新增 `led_mode_t`：
  ```c
  typedef enum {
      LED_MODE_STEADY = 0,
      LED_MODE_BLINK = 1,
  } led_mode_t;
  ```
- `led_t` 增加 blink 状态：
  ```c
  led_mode_t mode;
  led_state_t state;
  uint16_t on_ms;
  uint16_t off_ms;
  uint32_t next_toggle_ms;
  ```
- 新增接口：
  ```c
  void led_set_blink(led_t *self, uint16_t on_ms, uint16_t off_ms, uint32_t now_ms);
  void led_tick(led_t *self, uint32_t now_ms);
  ```

### `src/drivers/led.c`
- 新增内部 `led_apply()`，统一写状态和 GPIO 电平。
- `led_set()` / `led_toggle()` 会回到 `LED_MODE_STEADY`。
- `led_set_blink()` 设置 `LED_MODE_BLINK`，先点亮再设置下一次翻转时间。
- `led_tick()` 使用计划指定的回绕判断：
  ```c
  if ((int32_t)(now_ms - self->next_toggle_ms) < 0) {
      return;
  }
  ```
- 闪烁下一周期按 `on_ms/off_ms` 区分：
  ```c
  self->next_toggle_ms = now_ms + (next == LED_ON ? self->on_ms : self->off_ms);
  ```

### `include/drivers/bicolor_led.h` / `src/drivers/bicolor_led.c`
- 新增双色 LED 组合驱动。
- 颜色映射：
  - `BICOLOR_OFF`：green off, red off
  - `BICOLOR_GREEN`：green on, red off
  - `BICOLOR_RED`：green off, red on
  - `BICOLOR_YELLOW`：green on, red on
- `bicolor_led_get()` 用两个通道 `led_is_on()` 反推颜色。

### `include/platform/stm32f1_board.h` / `src/platform/stm32f1_board.c`
- 从 `led1..led4` 固定字段改为描述表 + 数组：
  ```c
  enum { STM32F1_BOARD_LED_MAX = 8 };

  typedef struct {
      GPIO_TypeDef *port;
      uint16_t pin;
      led_polarity_t polarity;
  } stm32f1_board_led_desc_t;
  ```
- `stm32f1_board_t` 保存：
  ```c
  stm32f1_gpio_pin_ctx_t ctx[STM32F1_BOARD_LED_MAX];
  hal_gpio_pin_t pins[STM32F1_BOARD_LED_MAX];
  led_polarity_t polarities[STM32F1_BOARD_LED_MAX];
  uint8_t count;
  ```
- `stm32f1_board_init()` 循环初始化 LED，引脚极性来自描述表，并保留 `count <= STM32F1_BOARD_LED_MAX` assert 与 clamp。

### `include/app/app.h` / `src/app/app.c`
- APP 从 `led1..led4` 改为：
  ```c
  led_t leds[APP_LED_MAX];
  uint8_t led_count;
  bicolor_led_t bicolor;
  bool has_bicolor;
  ```
- `app_dependencies_t` 改为接收板级数组基址：
  ```c
  const hal_gpio_pin_t *led_pins;
  const led_polarity_t *led_polarities;
  uint8_t led_count;
  ```
- demo 行为：
  - `leds[0]`：`led_set_blink(..., 500U, 500U, now_ms)`
  - `leds[1]`：`LED_ON` 常亮
  - `leds[2] + leds[3]`：双色灯，设置为 `BICOLOR_YELLOW`
- `app_tick()` 循环调用 `led_tick()`，STEADY 灯为 no-op。

### `Core/Src/main.c`
- 新增板级描述表：
  ```c
  static const stm32f1_board_led_desc_t board_leds[] = {
    { LED1_GPIO_Port, LED1_Pin, LED_ACTIVE_HIGH },
    { LED2_GPIO_Port, LED2_Pin, LED_ACTIVE_HIGH },
    { LED3_GPIO_Port, LED3_Pin, LED_ACTIVE_HIGH },
    { LED4_GPIO_Port, LED4_Pin, LED_ACTIVE_HIGH },
  };
  ```
- `board_config` 改为 `.descs + .count`。
- `app_dependencies_t` 改为传 `stm32f1_board_led_pins()`、`stm32f1_board_led_polarities()`、`stm32f1_board_led_count()`。

### `MDK-ARM/STM32_template.uvprojx`
- `Application/User/Framework` 组中新增 `bicolor_led.c`。
- 移除 `status_light.c`。

## 删除项
- 已删除 `include/services/status_light.h`
- 已删除 `src/services/status_light.c`
- 源码和 `uvprojx` 内无 `status_light` 残留。

## 未改边界
- 未修改 `include/hal/gpio.h`
- 未修改 `include/hal/timebase.h`
- 未修改 `src/platform/stm32f1/stm32f1_gpio_port.*`
- 未修改 `src/platform/stm32f1/stm32f1_timebase.*`
- 未修改 `button.*`
- 未修改 `Core/Inc/main.h` 引脚宏

## 验证
- 执行：
  ```text
  D:\6.IED\Keil_v5\UV4\UV4.exe -b E:\Workspace\mcu\mcu_discussion\BSP_HAL_APP\STM32_template\MDK-ARM\STM32_template.uvprojx -j0
  ```
- 结果：
  ```text
  "STM32_template\STM32_template.axf" - 0 Error(s), 0 Warning(s).
  ```
- 构建日志显示已编译 `bicolor_led.c`，未再编译 `status_light.c`。
- 已生成：
  - `MDK-ARM/STM32_template/STM32_template.axf`
  - `MDK-ARM/STM32_template/STM32_template.hex`
