/* ============================================================
 * @file hal.h
 * @brief 硬件抽象层聚合接口
 *
 * 层级：
 *   TurboBoxOS/hal/
 *
 * 项目内绝对路径：
 *   TurboBoxOS/hal/hal.h
 *
 * 模块作用：
 *   聚合 VGA、键盘、定时器初始化，内核只面向 hal_init()
 *   一个入口，不感知具体驱动细节。驱动实现更换时内核代码
 *   不需要改动。
 *
 * 使用者：
 *   kernel 启动流程和调度器。
 *
 * 项目角色：
 *   HAL 层对外接口，向上屏蔽 drv/ 层具体实现。
 *
 * 引入说明：
 *   不依赖其他头文件。
 *   编译参数 -ffreestanding -nostdlib -fno-exceptions -fno-rtti
 *
 * 维护记录：
 *   2026-08-30 初始创建
 * ============================================================ */

#ifndef TURBOX_HAL_H
#define TURBOX_HAL_H

/* 以下驱动由 drv/ 层实现，HAL 只负责聚合调用 */
void vga_init();
void kbd_init();

/* 初始化 PIT 定时器到指定频率，hz 为 0 时回退到 100Hz */
void hal_timer_init(unsigned int hz);

/* 返回启动以来的定时器 tick 数 */
unsigned int hal_ticks();

/* 初始化全部硬件抽象，顺序：VGA → 键盘 → 定时器 */
void hal_init();

#endif /* TURBOX_HAL_H */