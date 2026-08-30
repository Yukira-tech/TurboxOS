/* ============================================================
 * @file ports.h
 * @brief x86 端口 I/O 内联汇编辅助
 *
 * 层级：
 *   TurboBoxOS/hw/
 *
 * 项目内绝对路径：
 *   TurboBoxOS/hw/ports.h
 *
 * 模块作用：
 *   封装 x86 的 in/out 端口指令，提供字节和字两种粒度的读写，
 *   并附带传统 io_wait 延时。所有函数用 static inline 放在头文件，
 *   避免 freestanding 环境下额外的链接负担。
 *
 * 使用者：
 *   drv、hal、hw 等所有需要访问外设端口的模块。
 *
 * 项目角色：
 *   hw 层最基础的端口操作原语，是其他硬件驱动的地基。
 *
 * 引入说明：
 *   自包含，不依赖其他头文件。
 *   编译参数 -ffreestanding -nostdlib -fno-exceptions -fno-rtti
 *
 * 维护记录：
 *   2026-08-30 初始创建
 * ============================================================ */

#ifndef TURBOX_HW_PORTS_H
#define TURBOX_HW_PORTS_H

typedef unsigned short u16;
typedef unsigned char  u8;

/* 向指定端口写入一个字节 */
static inline void outb(u16 port, u8 val) {
    /* "a" 约束让 val 进入 AL，out 指令只接受 AL/AX 作为数据源 */
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

/* 从指定端口读取一个字节 */
static inline u8 inb(u16 port) {
    u8 ret;
    /* volatile 防止编译器把多次读取优化成一次，硬件寄存器值可能随时变 */
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* 向指定端口写入一个字（16 位） */
static inline void outw(u16 port, u16 val) {
    __asm__ volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

/* 从指定端口读取一个字（16 位） */
static inline u16 inw(u16 port) {
    u16 ret;
    __asm__ volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* 端口操作之间的短暂延时，向未使用的 0x80 端口写值 */
static inline void io_wait(void) {
    /* 老式 PIC/PIT 编程中连续 out 需要间隔，否则硬件可能来不及响应 */
    outb(0x80, 0);
}

#endif /* TURBOX_HW_PORTS_H */