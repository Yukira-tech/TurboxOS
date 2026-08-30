/* ============================================================
 * @file idt.cpp
 * @brief 中断描述符表定义与加载
 *
 * 层级：
 *   TurboBoxOS/hw/
 *
 * 项目内绝对路径：
 *   TurboBoxOS/hw/idt.cpp
 *
 * 模块作用：
 *   建立 256 项 IDT，安装默认异常处理桩和 int 0x80 系统调用门，
 *   并执行 lidt 加载 IDT。
 *
 * 使用者：
 *   kernel 启动流程调用 idt_init()。
 *
 * 项目角色：
 *   hw 层中断管理基础模块，是系统调用和异常处理的入口。
 *
 * 引入说明：
 *   不依赖其他头文件。
 *   编译参数 -ffreestanding -nostdlib -fno-exceptions -fno-rtti
 *
 * 维护记录：
 *   2026-08-30 初始创建
 * ============================================================ */

/* IDT 门描述符：x86 硬件规定的 8 字节结构 */
struct IdtEntry {
    unsigned short offset_low;  /* 处理函数地址低 16 位 */
    unsigned short selector;    /* 处理函数所在代码段选择子 */
    unsigned char  zero;        /* 恒为 0 */
    unsigned char  type_attr;   /* 门类型 + DPL + P 位 */
    unsigned short offset_high; /* 处理函数地址高 16 位 */
} __attribute__((packed));

/* lidt 指令的操作数 */
struct IdtPtr {
    unsigned short limit;
    unsigned int   base;
} __attribute__((packed));

#define IDT_ENTRIES     256
#define SYSCALL_VECTOR  0x80   /* SPEC 4.1 约定的系统调用中断号 */

static IdtEntry idt[IDT_ENTRIES];
static IdtPtr   idt_ptr;

/* 系统调用汇编桩，由 kernel 层以汇编/属性方式提供 */
extern "C" void isr_syscall_stub();

/* 兜底异常处理桩：未安装具体 handler 的向量统一指向它 */
extern "C" void isr_default_stub() __attribute__((weak));

/* 填充一个 IDT 门描述符 */
static void idt_set_gate(int num, unsigned int handler,
                         unsigned short sel, unsigned char flags) {
    idt[num].offset_low  = (unsigned short)(handler & 0xFFFF);
    idt[num].offset_high = (unsigned short)((handler >> 16) & 0xFFFF);
    idt[num].selector    = sel;
    idt[num].zero        = 0;
    idt[num].type_attr   = flags;
}

/*
 * 初始化 IDT：安装兜底桩与 int 0x80 系统调用门，并执行 lidt。
 * 默认所有向量指向兜底桩，避免意外中断跳转到野地址。
 */
void idt_init() {
    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base  = (unsigned int)&idt[0];

    unsigned int def = (unsigned int)&isr_default_stub;
    for (int i = 0; i < IDT_ENTRIES; ++i) {
        idt_set_gate(i, def, 0x08, 0x8E);   /* 32 位中断门，DPL=0 */
    }

    /* int 0x80 系统调用门：必须是陷阱门且 DPL=3，
       否则用户态触发 int 0x80 会被 CPU 以 #GP 拒绝 */
    idt_set_gate(SYSCALL_VECTOR, (unsigned int)&isr_syscall_stub, 0x08, 0xEE);

    __asm__ volatile ("lidt (%0)" : : "r"(&idt_ptr));
}