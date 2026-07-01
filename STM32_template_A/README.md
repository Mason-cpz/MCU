# STM32 通用 LED 框架

一套表驱动、tick 驱动、可移植的 LED 框架。增改 LED 只需改一张**编译期静态描述表**，不阻塞、可单元测试。

## 分层

```
app          业务策略：哪个灯闪、哪个灯常亮、哪两个组双色
 └─ drivers  led / bicolor_led / button —— 与芯片无关的逻辑驱动
     └─ hal  gpio / timebase —— 函数指针抽象，上层不碰 STM32 HAL
         └─ platform/stm32f1  把 STM32 HAL 适配成 hal 抽象 + 板级描述表
```

- `hal/gpio.h`：GPIO 只暴露逻辑电平读写（`hal_gpio_read/write`）。
- `hal/timebase.h`：只给毫秒接口（`hal_millis`），业务用 tick 推进，不塞阻塞延时。
- `drivers/led.*`：单通道 LED，自带 OFF/ON/闪烁三种模式。
- `drivers/bicolor_led.*`：两个单通道组合成绿/红/黄/灭。
- `platform/stm32f1_board.*`：按描述表初始化 GPIO，极性归这一层。

## 核心概念

**一个 LED = 一行描述表**。描述表写在 `Core/Src/main.c` 的 `board_leds[]`：

```c
static const stm32f1_board_led_desc_t board_leds[] = {
  { LED1_GPIO_Port, LED1_Pin, LED_ACTIVE_HIGH },  /* port, pin, 极性 */
  { LED2_GPIO_Port, LED2_Pin, LED_ACTIVE_HIGH },
  /* ... */
};
```

`count` 由 `sizeof` 自动算，板级和 app 都会按 `count` 裁剪到 `*_LED_MAX`（默认 8）。

极性（`LED_ACTIVE_HIGH` / `LED_ACTIVE_LOW`）是**接线事实**，只写在描述表里。换一块低电平点亮的板子，只改这一列，app 一行不动。

## 怎么加 / 改一个 LED

1. 在 `Core/Inc/main.h` 确认（或新增）引脚宏 `LEDx_Pin` / `LEDx_GPIO_Port`（CubeMX 生成）。
2. 在 `main.c` 的 `board_leds[]` 加一行 `{ port, pin, 极性 }`。
3. 在 `src/app/app.c` 的 `app_init` 里给这个灯设模式（见下）。

就这三步。结构体、初始化循环、访问函数都不用动。

> 灯数超过当前数量：把 `include/platform/stm32f1_board.h` 的 `STM32F1_BOARD_LED_MAX` 和 `src/bsp/bsp.c` 的 `BSP_LED_COUNT` 一起调大即可。

## 怎么设状态

在 `app_init` 里，`self->leds[i]` 已按描述表初始化好（上电默认灭）。然后：

```c
/* 常亮 */
led_set(&self->leds[1], LED_ON);

/* 闪烁：亮 500ms / 灭 500ms（可不对称，如 200/800 表示短亮长灭） */
led_set_blink(&self->leds[0], 500U, 500U, now_ms);

/* 双色灯：leds[2]=绿、leds[3]=红，组合显示 */
bicolor_led_init(&self->bicolor, &self->leds[2], &self->leds[3]);
bicolor_led_set(&self->bicolor, BICOLOR_YELLOW);  /* OFF/GREEN/RED/YELLOW 静态 */

/* 双色灯闪烁：参与该颜色的通道按 on/off 闪，其余通道灭。
 * 两通道共用同一 now_ms，绿红同相亮灭 = 闪烁黄 */
bicolor_led_blink(&self->bicolor, BICOLOR_YELLOW, 500U, 500U, now_ms);
```

`led_set` / `led_toggle` 会把模式复位成 STEADY（停止闪烁）；`led_set_blink` 切到 BLINK 模式。`bicolor_led_set` 是静态颜色，`bicolor_led_blink` 是闪烁颜色。

## tick 驱动

闪烁不靠定时器中断，靠主循环喂时间。`app_tick` 已经遍历所有灯调 `led_tick`：

```c
void app_tick(app_t *self) {
    uint32_t now = hal_millis(self->timebase);
    for (uint8_t i = 0; i < self->led_count; ++i)
        led_tick(&self->leds[i], now);   /* STEADY 灯是 no-op */
}
```

`main.c` 的 `while(1)` 里持续调 `app_tick(&app)` 即可。时间回绕用 `(int32_t)(now - next) < 0` 处理，49.7 天溢出安全。

## 当前 demo

4 灯模板演示了三种能力：

| 灯 | 行为 | 接口 |
|---|---|---|
| LED1 (`leds[0]`) | 常亮 | `led_set(.., LED_ON)` |
| LED2 (`leds[1]`) | 闪烁 500/500 | `led_set_blink(.., 500, 500, now)` |
| LED3+LED4 (`leds[2]`绿/`leds[3]`红) | 组合闪烁黄（同相亮灭）| `bicolor_led_blink(.., BICOLOR_YELLOW, 500, 500, now)` |

只接 1 个灯时，把 `board_leds[]` 删到 1 行即可，`app_init` 里 `led_count >= 2 / >= 4` 的分支会自动跳过。

## 构建

```
D:\6.IED\Keil_v5\UV4\UV4.exe -b <项目路径>\MDK-ARM\STM32_template.uvprojx -j0
```

新增 `.c` 文件要在 `MDK-ARM/STM32_template.uvprojx` 的 `Application/User/Framework` 组里加一个 `<File>` 块。

## 后续可扩展

- **闪烁颜色已支持**：`bicolor_led_blink(self, color, on_ms, off_ms, now_ms)` 让指定颜色的通道同相闪烁（黄=绿红同步亮灭），未参与的通道置灭。
- **绿红交替闪**（亮黄→灭→亮黄 之外的轮流亮）：当前 `led_t` 无相位偏移参数，需给 `led_set_blink` 加初始相位/延迟起振能力。
- **呼吸 / PWM 调光**：需要 hal 层加 PWM 抽象，目前只有数字开关。
