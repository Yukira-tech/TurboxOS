/* ============================================================
 * @file hal.cpp
 * @brief 硬件抽象层初始化与定时器实现
 *
 * 层级：
 *   TurboBoxOS/hal/
 *
 * 项目内绝对路径：
 *   TurboBoxOS/hal/hal.cpp
 *
 * 模块作用：
 *   按正确顺序聚合 VGA、键盘、定时器初始化，并实现
 *   PIT 定时器抽象。显存与键盘的底层细节在 drv/ 层，
 *   本文件只负责初始化顺序和定时器接口。
 *
 * 使用者：
 *   kernel 启动流程调用 hal_init()，调度器使用
 *   hal_ticks() 获取节拍。
 *
 * 项目角色：
 *   硬件抽象层的落地模块，是内核与 drv/hw 层之间的桥。
 *
 * 引入说明：
 *   依赖 hal.h 和 hw/ports.h。
 *   编译参数 -ffreestanding -nostdlib -fno-exceptions -fno-rtti
 *
 * 维护记录：
 *   2026-08-30 初始创建
 * ============================================================ */

#include "hal.h"
#include "../hw/ports.h"

/* PIT 8253/8254 端口与基准频率 */
#define PIT_CMD_PORT   0x43
#define PIT_CH0_PORT   0x40
#define PIT_BASE_FREQ  1193182u   /* 晶振固定频率，分频值由此算出 */

/* volatile：tick 在中断处理程序中被更新，不能优化成常量读取 */
static volatile unsigned int g_ticks = 0;

/*
 * 初始化 PIT 定时器，通道 0 设为方波发生器。
 * 分频值 = 晶振频率 / 目标频率，16 位宽。
 * hz 为 0 时回退到 100Hz，供协作式调度计时。
 */
void hal_timer_init(unsigned int hz) {
    if (hz == 0) hz = 100;
    unsigned int divisor = PIT_BASE_FREQ / hz;
    if (divisor > 0xFFFF) divisor = 0xFFFF;

    /* 命令字节：通道0、先低后高字节、模式3（方波）、二进制计数 */
    outb(PIT_CMD_PORT, 0x36);
    outb(PIT_CH0_PORT, (u8)(divisor & 0xFF));        /* 先低字节 */
    outb(PIT_CH0_PORT, (u8)((divisor >> 8) & 0xFF)); /* 后高字节 */
}

/* 定时器中断内调用，累加 tick，供 IRQ0 处理程序使用 */
void hal_timer_tick() {
    ++g_ticks;
}

/* 返回启动以来的 tick 数 */
unsigned int hal_ticks() {
    return g_ticks;
}

/*
 * 初始化全部硬件抽象。
 * 顺序不能乱：
 *   1. VGA 先初始化，否则后面出错也没地方显示
 *   2. 键盘次之，尽快让 shell 可交互
 *   3. 定时器最后，给协作式调度提供节拍
 */
void hal_init() {
    vga_init();
    kbd_init();
    hal_timer_init(100);
}