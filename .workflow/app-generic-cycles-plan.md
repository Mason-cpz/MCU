# 计划:app 层泛型化为「按键循环通道」数组(STM32_template_A_codex)

> 只改 `STM32_template_A_codex` 目录。需求依据 `KEY_LED_FRAMEWORK_SPEC.md`,本文件不重复需求。

## 背景 / 为什么改

当前 Codex 工程 app 层三处硬编码,导致「加一个按键+LED 通道」要同时改结构体、init 签名、tick 分支:

- `include/app/app.h:12-23` —— `app_t` 平铺 `key1/key2/key3/led1/led2/bicolor` 六个句柄 + 三个 `*_step_index` 游标 + `led1_boot_heartbeat`。
- `include/app/app.h:25-33` 与 `src/app/app.c:84-92` —— `app_init` 七个位置参数,`led1/led2` 相邻同类型,顺序写反编译器不报错(用户原始吐槽点)。
- `src/app/app.c:142-157` —— `app_tick` 三个写死的 `if (button_poll(...))` 分支,每加一个通道加一个分支。

**注意:手势→动作映射已经是表驱动(`s_led1_steps` / `s_led2_steps` / `s_bicolor_steps`),这部分是本工程的优点,不要动、不要推翻。** 本次只泛型化「通道」这一层,让「加通道 = 加表 + 配一行数组」,不再改 init 签名与 tick 逻辑。

## 目标形态(方向,非逐字要求)

引入不含输出类型的通用通道,把 `led_t*` 与 `bicolor_led_t*` 的差异藏进 apply 回调:

```c
/* 把第 index 档效果渲染到某输出;output 具体类型由该 apply 自己解释 */
typedef void (*app_cycle_apply_fn)(void *output, uint8_t index, uint32_t now_ms);

typedef struct {
    button_t          *button;   /* 事件来源 */
    void              *output;   /* led_t* / bicolor_led_t* ... 由 apply 解释 */
    app_cycle_apply_fn apply;
    uint8_t            count;    /* 循环表长度 */
    uint8_t            index;    /* 当前档位 */
} app_cycle_t;

typedef struct { app_cycle_t cycles[APP_CYCLE_COUNT]; } app_t;
```

- `app_tick` 变成 `for` 循环遍历 `cycles[]`,每个通道:取本键事件 → 若有效则 `index=(index+1)%count` → `apply(output, index, now_ms)`。
- `app_init` **不再用七个位置参数**。二选一(Codex 自行选,评审只看结果达标):
  - (A) 命名结构 `app_io_t { button_t *keys[3]; led_t *leds[2]; bicolor_led_t *bicolor; }`,`app_init(app_t*, const app_io_t*, now_ms)`;或
  - (B) 由 bsp 传入 `app_cycle_spec[]`(button*/output*/apply/count/default_index)数组,app_init 只做 for 填表。
  - 底线:调用点不得再出现「一串同类型裸指针按位置排队」。

## 硬约束(不可违背)

1. **Demo 行为逐条不变**,严格对照 `KEY_LED_FRAMEWORK_SPEC.md:92-118`:
   - LED1 上电心跳 1Hz(500/500);首次 KEY1 有效事件退出心跳,循环 `OFF→ON→BLINK(500/500)→OFF`。
   - LED2 循环 `OFF→ON→BLINK(200/800)→BLINK(800/200)→OFF`,上电 OFF。
   - KEY3 双色灯循环 `OFF→GREEN→RED→YELLOW→YELLOW BLINK(500/500)→OFF`,上电 OFF。
   - KEY1 上升沿、KEY2/KEY3 下降沿的触发语义不变。
2. **只改 `src/app/app.c`、`include/app/app.h`、`src/bsp/bsp.c` 三个文件**。不动 `drivers/`、`platform/`、`hal/`、`main.c`。
3. `now_ms` 由调用方传入的约定不变(spec:26),核心层不得引入全局 tick。
4. 上层不得出现 `HAL_*` / `GPIOx` 厂商符号(spec:50);app 层不得直接读原始 GPIO。
5. 保留中文注释:新增结构体/枚举成员、新增/改签名的函数都要按全局规则写用途注释。

## 建议(可采纳,能简化就采纳)

- **用「循环表默认档」消解 led1 心跳特例**:LED1 心跳恰好等于循环表里的 `BLINK(500/500)` 档。让 LED1 通道默认 `index` 停在该 BLINK 档,首次短按 `+1` 自然回到 `OFF`,即可**删掉 `led1_boot_heartbeat` 布尔和它的特殊分支**(`app.c:19` 的 `s_led1_boot_step`、`app.c:118-125` 的心跳分支)。这样心跳不再是特例,全部通道走同一条 `index+1` 逻辑。若采纳,验收时确认心跳行为与首次按键行为仍与 spec 一致。

## 风险点(人工重点盯这里)

- ⚠️ **心跳退出语义**:无论用不用上面的化简,必须确认「首次 KEY1 事件后 LED1 从心跳 500/500 变为 OFF」。这是最容易在重构中跑偏的一条,务必对照 spec:106 核对。
- ⚠️ **默认档 / 首次事件方向**:LED1 首次事件是「退出心跳到 OFF」,LED2/双色灯首次事件是「从 OFF 前进到下一档」。泛型化后三者都走 `index+1`,要保证各自的**默认起始 index** 选对(LED1 起始在 BLINK 档,LED2/双色灯起始在 OFF 档),否则首次按下行为会错。
- ⚠️ **多通道共享按键**(前瞻,非当前 bug):当前是严格 1:1。若泛型化后仍让每个通道各自 `button_poll` 消费,则将来两个通道指向同一按键时,第二个通道会拿到 `NONE`(`button_poll` 有副作用、单次消费)。**本轮不要求实现事件分发层**,但若 Codex 顺手做了「每 tick 先统一 poll 每键一次、通道再消费缓存事件」,视为加分且需保证 Demo 行为不变;若不做,需在改动清单里注明「通道仍独占 poll,暂不支持多通道共享同一按键」。
- ⚠️ **apply 回调类型安全**:`void *output` 丢失类型,apply 内部强转前应有约定(注释写清该 apply 只接受哪种 output),避免把 `led_t*` 传进 bicolor 的 apply。

## 验收清单(评审逐条打勾,以真实代码 / 编译为准)

1. `app_t` 不再平铺六个句柄字段;改为通道数组(或等价的可扩展结构)。
2. `app_init` 调用点(`src/bsp/bsp.c:140-144`)不再是一串同类型裸指针按位置排队。
3. `app_tick` 不再是「每通道一个写死 if 分支」;是遍历数组的统一逻辑。
4. 「加第四个通道」在代码上只需:加一张 step 表 + 加一个 apply(若新输出类型) + 数组里加一行 + `APP_CYCLE_COUNT +1`;**无需改 `app_init` 签名、无需改 `app_tick` 逻辑**。评审时口头走一遍这个流程验证。
5. Demo 行为逐条对齐 spec 验收表(见硬约束 1),尤其 LED1 心跳→OFF。
6. 用 Keil 编译:`D:\6.IED\Keil_v5\UV4\UV4.exe` 打开 `STM32_template_A_codex\MDK-ARM\STM32_template.uvprojx` 编译 0 error(warning 尽量为 0)。无法本地跑实机则说明,并用逻辑推演对照 spec。
7. 中文注释齐全(新结构体成员、新/改签名函数)。
8. 未触碰 `drivers/`、`platform/`、`hal/`、`main.c`;`git diff --stat` 只应出现那三个文件。

## 交付物

- Codex 按 `executor-mode` 落改动清单到 `.workflow/app-generic-cycles-round-N-changes.md`,含:改了哪些文件、通道结构最终形态、是否采纳心跳化简、是否做了事件分发层、编译结果。
