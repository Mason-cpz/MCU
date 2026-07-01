#ifndef PLATFORM_PLATFORM_GPIO_H
#define PLATFORM_PLATFORM_GPIO_H

#include "hal/gpio.h"

#include <stdbool.h>
#include <stdint.h>

/* 平台无关的 GPIO 门面。
 *
 * 这一层是上层(bsp/drivers/app)与厂商 HAL 之间的唯一接口：
 * 上层只用中性的 (port_id, pin_num) 描述引脚，绝不出现 HAL_* / GPIOx 符号。
 * 迁移到 GD32 等芯片时，只需另写一份实现这套函数的 platform 源文件，
 * bsp / drivers / app 一行不用改。
 *
 * 引脚上下文由平台实现内部管理(静态池)，上层无需分配，
 * 因此上层也不必知道任何厂商相关的上下文类型。
 */

/* 中性端口编号：由平台实现映射到具体寄存器/时钟。 */
typedef enum {
    PORT_A = 0,
    PORT_B,
    PORT_C,
    PORT_D,
    PORT_E,
    PORT_COUNT,
} port_id_t;

/* 中性上下拉语义。 */
typedef enum {
    GPIO_PULL_NONE = 0,
    GPIO_PULL_UP,
    GPIO_PULL_DOWN,
} gpio_pull_t;

/* 配置为推挽输出并写入初始电平；填充 pin 供上层通过 hal_gpio_* 读写。 */
void platform_gpio_init_output(
    hal_gpio_pin_t *pin,
    port_id_t       port,
    uint8_t         pin_num,
    bool            initial_level);

/* 配置为带上下拉的输入；填充 pin 供上层(按键驱动)读电平。 */
void platform_gpio_init_input(
    hal_gpio_pin_t *pin,
    port_id_t       port,
    uint8_t         pin_num,
    gpio_pull_t     pull);

#endif
