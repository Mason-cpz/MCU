# CLAUDE.md

本文件给后续协作的 AI 提供项目上下文。读完即可接上进度，无需从头摸索。

## 这个仓库是什么

一套 **STM32F103 通用 LED 框架** 的探索。目标：用一套分层、可移植、表驱动的框架，
覆盖多种实际 LED 场景，且「增改 LED 配置」的成本尽量低。

要覆盖的场景（最初需求）：
1. 只有一个 LED。
2. 4 个 LED 分别表示网络 / 故障 / 运行等状态。
3. 状态显示分常亮与闪烁。
4. 双引脚双色灯：一脚绿、一脚红，同亮为黄。

## 仓库结构：三份工程

| 目录 | 角色 | 状态 |
|---|---|---|
| `STM32_template` | 基线。表驱动 LED 框架 + 在 main.c 里直接装配 | 完成，作对照 |
| `STM32_template_A` | 方案 A：新增 BSP 组合层，**保留** chip-agnostic 接缝 | 完成，待选型 |
| `STM32_template_B` | 方案 B：新增 BSP 组合层，**砍掉** 接缝（app 直吃 board） | 完成，待选型 |

> **当前待用户决策点**：A vs B 二选一（见下）。三份都能编译、demo 行为一致。
> 选定后，另外两份可删。

每个工程内部结构一致：
```
include/  hal/        gpio.h / timebase.h        —— 函数指针抽象，上层不碰 STM32 HAL
          drivers/    led.h / bicolor_led.h / button.h
          platform/   stm32f1_board.h, stm32f1/stm32f1_gpio_port.h, stm32f1_timebase.h
          app/        app.h
          bsp/        bsp.h                       —— 仅 A/B 有
          services/   (status_light 已删除)
src/      与 include 镜像
Core/ Drivers/ MDK-ARM/  —— CubeMX 生成 + Keil 工程
.workflow/  规划与改动清单（见「协作工作流」）
README.md   每份工程各有一份，讲该工程的用法
```

## 分层与数据流

```
app          业务策略：哪个灯闪、哪个常亮、哪两个组双色（chip-agnostic）
 └─ drivers  led / bicolor_led / button —— 与芯片无关的逻辑驱动
     └─ hal  gpio / timebase —— 函数指针抽象（hal_gpio_pin_t.read/write、hal_millis）
         └─ platform/stm32f1  把 STM32 HAL 适配成 hal 抽象 + 板级描述表
bsp          组合根（仅 A/B）：持有 board/app 实例与全部配置表，对外只露 bsp_init/bsp_tick
```

main.c 在 A/B 里已收敛成两行：`bsp_init();` 和循环里的 `bsp_tick();`。

## 核心设计决策（已定，勿推翻除非用户要求）

1. **编译期静态描述表**管理 LED 集合。加灯 = 表里加一行，不用改结构体。
2. **闪烁折进 `led_t`**：模式 `LED_MODE_STEADY / LED_MODE_BLINK` + `on_ms/off_ms` +
   `led_tick(led, now_ms)`。tick 驱动，不用定时器中断，不阻塞。
3. **时间回绕**统一用 `(int32_t)(now - next) < 0` 判断，49.7 天溢出安全。勿「顺手优化」。
4. **`bicolor_led`** 由两个单通道 `led_t` 组合出 OFF/GREEN/RED/YELLOW；
   `bicolor_led_blink` 让参与通道同相闪烁（黄 = 绿红同步亮灭）。双色是「两个灯的关系」，
   用显式 index 配置，不做成 per-led 行为枚举。
5. **极性归板级**（`led_polarity_t` 在描述表里）。换低电平点亮的板子只改表，app 不动。
   board 层 GPIO 初始化按极性写「灭」电平，避免 ACTIVE_LOW 上电误亮。
6. **`status_light` 服务已删除**：它原来写死「两个灯一起闪」，能力已并入 `led_tick`。

## A vs B 的唯一区别（选型关键）

两者都有 bsp 层、main.c 都是两行。区别只在 `app_dependencies_t` 这层接缝：

| | 方案 A | 方案 B |
|---|---|---|
| `app.h` includes | 只有 drivers/hal | 多 `platform/stm32f1_board.h` |
| `app_init` 签名 | `(self, const app_dependencies_t*)` | `(self, board, behaviors, bicolor)` |
| `app_dependencies_t` | 保留 | 删除 |
| bsp.c 装配 | 多 ~6 行 deps bundling | 直接传 board |
| app 层耦合 | **chip-agnostic，可换芯片/PC 单测** | **绑死 STM32F1** |
| 加按键时 | deps 加字段，app_init 签名不变 | app_init 再加参数，耦合加深 |

**助手立场**：倾向 A。框架最初目标就是「通用」，B 省下的几行是一次性小钱，
赔进去的是可移植性与可测试性。但这是用户的取舍，用户想要极简也合理。

## 协作工作流（codex-flow）

本项目用 codex-flow：**Claude 规划 + 评审，桌面 Codex 执行**，所有交接经用户人工转交。

- 复杂任务的规划写进 `.workflow/<任务名>-plan.md`，含「验收标准」和「风险点」。
- Codex 用 executor-mode 执行，落改动清单到 `.workflow/<任务名>-round-N-changes.md`。
- **评审铁律**：只读真实代码 / diff，绝不信执行者的文字自述；逐条核对验收标准给 PASS/REJECT。
- `.workflow/` 编号历史（已全部 PASS）：
  - `01` bool→枚举重构
  - `02` 通用 LED 框架（表驱动 + 模式折入 + bicolor + 删 status_light）
  - `03` 初始化简化（去 config 壳 + 极性下沉 + 修 ACTIVE_LOW 误亮）
  - `04` 新增 bsp 组合层（A/B 各一份）

## 构建

Keil MDK，命令行编译：
```
D:\6.IED\Keil_v5\UV4\UV4.exe -b <工程目录>\MDK-ARM\STM32_template.uvprojx -j0
```
三份工程的 TargetName/输出目录已各自区分（STM32_template / _A / _B），互不冲突。
新增 `.c` 要在对应 `.uvprojx` 的 `Application/User/Framework` 组里加 `<File>` 块。

## 环境与版本控制注意

- 编译产物（`MDK-ARM/<Target>/`、`*.o/.axf/.hex/.map` 等）由 `.gitignore` 排除，
  clone 后本地重新 build 生成。`.uvprojx/.uvoptx` 入库，`.uvguix.*` 不入库。
- 仓库根 = 本目录（`BSP_HAL_APP`），远端 `github.com/Mason-cpz/MCU`，分支 `main`。
- git 身份是 **本地配置**（非 global）：换电脑 clone 后需重设
  `git config user.name "Mason"` / `user.email "563552982@qq.com"`。

## 下一步候选（未开工）

- 用户拍板 A / B，删掉落选工程。
- 加 button 服务接入 bsp，验证「加外设」的真实成本（A/B 差距的实测）。
- 可选扩展：呼吸 / PWM 调光（需 hal 加 PWM 抽象）。
