# 01 round 1 改动清单

## 修改文件
- `include/drivers/led.h`
- `src/drivers/led.c`
- `src/app/app.c`
- `src/services/status_light.c`

## 关键改动
- `include/drivers/led.h`
  - 新增 `led_state_t` 与 `led_polarity_t`。
  - `led_t` 字段从 `bool active_high / bool is_on` 改为 `led_polarity_t polarity / led_state_t state`。
  - `led_init`、`led_set`、`led_is_on` 改为枚举接口。
  - 补充注释：引脚须先由平台层配置为推挽输出。
- `src/drivers/led.c`
  - 引入 `assert.h`。
  - 四个函数入口增加 `assert`。
  - `gpio_level_for` 按 `led_state_t` / `led_polarity_t` 计算电平。
  - `led_toggle` 改为枚举状态翻转。
- `src/app/app.c`
  - 4 个 LED 初始化改用 `LED_ACTIVE_HIGH`。
  - `LED3`、`LED4` 改为 `LED_ON` 常亮。
- `src/services/status_light.c`
  - 仅把 `led_set` 入参改为 `LED_ON / LED_OFF`。
  - 保持内部 `uint8_t` 翻转逻辑与溢出回绕判断不变。

## 验证
- 执行：
  - `D:\6.IED\Keil_v5\UV4\UV4.exe -b E:\Workspace\mcu\mcu_discussion\BSP_HAL_APP\STM32_template\MDK-ARM\STM32_template.uvprojx -j0`
- 结果：
  - `0 Error(s), 0 Warning(s)`
  - 已生成 `STM32_template.axf` 和 `STM32_template.hex`

## 边界
- 未修改 `hal/gpio.h`
- 未修改 `platform/*`
- 未把 `app` 的 `led1..led4` 改成数组
- 未改 `status_light` 的内部翻转语义和回绕判断
