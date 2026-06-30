#ifndef BSP_BSP_H
#define BSP_BSP_H

/* 板级支持包：持有 board/app 实例与全部配置表，对外只露两个入口。
 * 加外设、改配置都进 bsp.c，main.c 不再改动。 */
void bsp_init(void);
void bsp_tick(void);

#endif
