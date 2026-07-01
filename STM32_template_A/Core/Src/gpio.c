/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    gpio.c
  * @brief   This file provides code for the configuration
  *          of all used GPIO pins.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "gpio.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                             */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/** GPIO 初始化职责已下沉到 BSP 层（src/bsp/bsp.c）。
 *  所有 LED/KEY 引脚的方向、上下拉、时钟使能均由
 *  platform/stm32f1/stm32f1_gpio_port.c 统一管理。
 *  本函数保留为空壳，仅为兼容 CubeMX 生成流程与 main.c 调用顺序。
 */
void MX_GPIO_Init(void)
{
    /* 故意留空：GPIO 配置由 bsp_init() 接管。 */
}

/* USER CODE BEGIN 2 */

/* USER CODE END 2 */
