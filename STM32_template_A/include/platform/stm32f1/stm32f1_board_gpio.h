#ifndef PLATFORM_STM32F1_BOARD_GPIO_H
#define PLATFORM_STM32F1_BOARD_GPIO_H

/* STM32F1 平台的板级 GPIO 抽象实现。
 *
 * 本文件把 STM32 HAL 的具体类型/宏封装为统一的 board_* 名字，
 * 让 bsp.c 不直接出现 stm32/GPIO_TypeDef/GPIOA 等。
 *
 * 移植到其它 MCU 时，新建对应的 <mcu>_board_gpio.h，
 * 提供相同的 board_* 类型/宏/函数，bsp.c 只改 #include 路径。
 */

#include "hal/gpio.h"
#include "hal/timebase.h"
#include "platform/stm32f1/stm32f1_gpio_port.h"  /* stm32f1_gpio_pin_ctx_t */

#include <stdbool.h>

/* ===== 类型别名：bsp.c 只用 board_* ===== */
typedef GPIO_TypeDef    *board_gpio_port_t;     /* 端口句柄 */
typedef uint16_t         board_pin_mask_t;      /* 位掩码   */
typedef stm32f1_gpio_pin_ctx_t board_gpio_pin_ctx_t; /* 平台私有 ctx */

/* ===== 引脚宏：bsp.c 描述表用这些，不直接写 GPIOA / GPIO_PIN_1 ===== */
#define BOARD_PORT_A     GPIOA
#define BOARD_PORT_B     GPIOB
#define BOARD_PORT_C     GPIOC
#define BOARD_PORT_D     GPIOD

#define BOARD_PIN_0      GPIO_PIN_0
#define BOARD_PIN_1      GPIO_PIN_1
#define BOARD_PIN_2      GPIO_PIN_2
#define BOARD_PIN_3      GPIO_PIN_3
#define BOARD_PIN_4      GPIO_PIN_4
#define BOARD_PIN_5      GPIO_PIN_5
#define BOARD_PIN_6      GPIO_PIN_6
#define BOARD_PIN_7      GPIO_PIN_7
#define BOARD_PIN_8      GPIO_PIN_8
#define BOARD_PIN_9      GPIO_PIN_9
#define BOARD_PIN_10     GPIO_PIN_10
#define BOARD_PIN_11     GPIO_PIN_11
#define BOARD_PIN_12     GPIO_PIN_12
#define BOARD_PIN_13     GPIO_PIN_13
#define BOARD_PIN_14     GPIO_PIN_14
#define BOARD_PIN_15     GPIO_PIN_15

/* 一次性初始化：配 GPIO 为推挽输出 + 构造 hal_gpio_pin_t。
 * initial_level=true 表示初始高电平。 */
void board_gpio_pin_init(
    hal_gpio_pin_t *pin,
    board_gpio_pin_ctx_t *ctx,
    board_gpio_port_t port,
    board_pin_mask_t pin_mask,
    bool initial_level);

/* 初始化板级时基 */
void board_timebase_init(hal_timebase_t *timebase);

#endif
