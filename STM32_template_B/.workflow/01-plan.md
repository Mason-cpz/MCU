# LED 框架第一版打磨 — 执行计划

## 目标
在不改动分层边界（hal 抽象 / platform 适配 / app 数组化）的前提下，打磨 LED 驱动本身的 API 与健壮性。

## 范围
- 主改：`include/drivers/led.h`、`src/drivers/led.c`
- 同步调用点：`src/app/app.c`、`src/services/status_light.c`
- **不动**：`include/hal/gpio.h`、`src/platform/stm32f1/*`、app 的 `led1..led4` 字段结构

## 三项改动

### 改动 1：布尔参数换成枚举
在 `led.h` 新增两个枚举：
```c
typedef enum { LED_OFF = 0, LED_ON = 1 } led_state_t;
typedef enum { LED_ACTIVE_LOW = 0, LED_ACTIVE_HIGH = 1 } led_polarity_t;
```
函数签名调整：
- `void led_init(led_t *self, const hal_gpio_pin_t *pin, led_polarity_t polarity);`
- `void led_set(led_t *self, led_state_t state);`
- `led_state_t led_is_on(const led_t *self);`
- `led_toggle` 签名不变。

结构体 `led_t`：
- `bool active_high` → `led_polarity_t polarity`
- `bool is_on` → `led_state_t state`

`led.c` 内部实现同步：
- `gpio_level_for` 改成基于 `led_state_t` / `led_polarity_t` 计算电平。
- `led_toggle` 用 `led_set(self, self->state == LED_ON ? LED_OFF : LED_ON)`。

调用点同步：
- `app.c`：`led_init(&self->ledN, deps->ledN_pin, LED_ACTIVE_HIGH);`；`led_set(&self->led3, LED_ON);` 等。
- `status_light.c`：`led_set(self->led1, self->led1_state != 0U ? LED_ON : LED_OFF);`（`led1_state`/`led2_state` 这两个 service 内部的 `uint8_t` 标志保持不变，只在传给 `led_set` 时转成枚举）。

### 改动 2：assert 前置检查
- `led.c` 顶部 `#include <assert.h>`。
- `led_init`：`assert(self != NULL); assert(pin != NULL);`
- `led_set` / `led_toggle` / `led_is_on`：入口 `assert(self != NULL);`
- 用标准 `assert`，依赖 `NDEBUG` 在 release 下消除，不引入运行时 `if`。

### 改动 3：GPIO 配置契约写进注释
- 在 `led.h` 头部注释补充：`led_init` 假设引脚已由平台层（CubeMX `MX_GPIO_Init`）配置为**推挽输出**；本驱动只读写电平，不负责方向配置。
- **只写注释**，不给 hal 抽象加 configure 能力（留到下一轮）。

## 验收标准（评审逐条核对真实代码）
1. `led.h` 含 `led_state_t` 与 `led_polarity_t` 两个枚举，且 `LED_OFF=0/LED_ON=1`、`LED_ACTIVE_LOW=0/LED_ACTIVE_HIGH=1`。
2. 四个函数签名与结构体字段已按枚举改造，`led.c` 内无残留 `bool` 形式的 on/active_high 逻辑。
3. `app.c`、`status_light.c` 调用点全部改为枚举常量，无 `true/false` 直接传入 `led_set`/`led_init`。
4. `led.c` 四个函数入口有对应 `assert`，且 `#include <assert.h>`。
5. `led.h` 注释明确写出"引脚须预先配为推挽输出"的前置契约。
6. 不存在对 `hal/gpio.h`、`platform/*`、app 结构体字段数组化的改动。
7. 工程可编译（若环境允许）。

## 风险点（人工重点盯）
- **`status_light.c` 的语义不要改错**：`led1_state ^= 1U` 这类内部翻转逻辑保持原样，只在传参给 `led_set` 时转枚举；不要把 service 内部状态也改成枚举（那会扩大范围）。
- **溢出回绕判断别动**：`(int32_t)(now_ms - next_toggle_ms) < 0` 是正确的，不要"顺手优化"。
- **不要越界**：确认 Codex 没有顺手给 hal 加配置函数或把 app 改成数组——这两项明确留到下一轮。
