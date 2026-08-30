/* ============================================================
 * @file gdt.cpp
 * @brief 全局描述符表定义与加载
 *
 * 层级：
 *   TurboBoxOS/hw/
 *
 * 项目内绝对路径：
 *   TurboBoxOS/hw/gdt.cpp
 *
 * 模块作用：
 *   在 C++ 侧重建内核正式 GDT，替换 loader 里的临时表。
 *   正式表只建内核代码段和数据段，但结构上可扩展用户段和 TSS。
 *
 * 使用者：
 *   kernel 启动流程调用 gdt_init()。
 *
 * 项目角色：
 *   hw 层基础模块，保护模式寻址的地基。
 *
 * 引入说明：
 *   依赖 ports.h 提供的端口 IO。
 *   编译参数 -ffreestanding -nostdlib -fno-exceptions -fno-rtti
 *
 * 维护记录：
 *   2026-08-30 初始创建
 * ============================================================ */

#include "ports.h"

// x86 规定的 8 字节段描述符，必须 packed，硬件按严格偏移解析
struct GdtEntry {
    unsigned short limit_low;   // 段限界低 16 位
    unsigned short base_low;    // 基址低 16 位
    unsigned char  base_mid;    // 基址中 8 位
    unsigned char  access;      // P/DPL/S/类型
    unsigned char  granularity; // 粒度位 + 限界高 4 位
    unsigned char  base_high;   // 基址高 8 位
} __attribute__((packed));

// lgdt 的操作数：2 字节限界 + 4 字节基址
struct GdtPtr {
    unsigned short limit;
    unsigned int   base;
} __attribute__((packed));

static GdtEntry gdt[3];   // 空 / 内核代码 / 内核数据
static GdtPtr   gdt_ptr;

// 填充一个 GDT 描述符
static void gdt_set_gate(int num, unsigned int base, unsigned int limit,
                         unsigned char access, unsigned char gran) {
    gdt[num].base_low    = (unsigned short)(base & 0xFFFF);
    gdt[num].base_mid    = (unsigned char)((base >> 16) & 0xFF);
    gdt[num].base_high   = (unsigned char)((base >> 24) & 0xFF);
    gdt[num].limit_low   = (unsigned short)(limit & 0xFFFF);

    // 限界高 4 位和粒度位复用同一字节，先拆后并
    gdt[num].granularity = (unsigned char)(((limit >> 16) & 0x0F) | (gran & 0xF0));
    gdt[num].access      = access;
}

/*
 * 建立平坦模型 GDT 并执行 lgdt。
 * 之后必须重载数据段寄存器和 CS，否则旧选择子还指向 loader 的临时表。
 */
void gdt_init() {
    gdt_ptr.limit = sizeof(gdt) - 1;         // 硬件要求限界 = 表大小 - 1
    gdt_ptr.base  = (unsigned int)&gdt[0];

    gdt_set_gate(0, 0, 0, 0, 0);             // 第 0 项空描述符
    gdt_set_gate(1, 0, 0xFFFFF, 0x9A, 0xCF); // 内核代码段，DPL=0
    gdt_set_gate(2, 0, 0xFFFFF, 0x92, 0xCF); // 内核数据段，DPL=0

    __asm__ volatile ("lgdt (%0)" : : "r"(&gdt_ptr));

    // 数据段寄存器全部切到 0x10，CS 用远跳转刷新到 0x08
    __asm__ volatile (
        "mov $0x10, %ax\n"
        "mov %ax, %ds\n"
        "mov %ax, %es\n"
        "mov %ax, %fs\n"
        "mov %ax, %gs\n"
        "mov %ax, %ss\n"
        "ljmp $0x08, $1f\n"
        "1:\n"
    );
}