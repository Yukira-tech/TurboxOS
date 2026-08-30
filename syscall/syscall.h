/* ============================================================
 * @file syscall.h
 * @brief 系统调用号定义与 int 0x80 陷入封装
 *
 * 层级：
 *   TurboBoxOS/syscall/
 *
 * 项目内绝对路径：
 *   TurboBoxOS/syscall/syscall.h
 *
 * 模块作用：
 *   定义用户态与内核态之间唯一的系统调用号契约，并提供
 *   sys_call 内联封装，把调用号和参数通过 int 0x80 送入内核。
 *
 * 使用者：
 *   user/demo.cpp 和 user/shell.cpp 等用户态代码。
 *
 * 项目角色：
 *   syscall 层的契约头，是用户态访问内核能力的唯一入口。
 *
 * 引入说明：
 *   不依赖其他头文件。
 *
 * 维护记录：
 *   2026-08-30 初始创建
 * ============================================================ */

#ifndef TURBOX_OS_API_SYSCALL_H
#define TURBOX_OS_API_SYSCALL_H

/* 系统调用号，数值和顺序是 SPEC 4.1 契约，禁止修改 */
enum SyscallNum : unsigned int {
    SYS_FORK   = 1,   // 创建进程
    SYS_EXIT   = 2,   // 退出
    SYS_CREATE = 3,   // 创建文件
    SYS_WRITE  = 4,   // 写文件
    SYS_READ   = 5,   // 读文件
    SYS_RUN    = 6,   // 运行文件
    SYS_PUTC   = 7,   // 输出字符
    SYS_GETC   = 8    // 读取键盘字符
};

/*
 * 通过 int 0x80 陷入内核。
 * 参数约定：
 *   eax = 系统调用号
 *   ebx = 参数1
 *   ecx = 参数2
 *   edx = 参数3
 * 返回值从 eax 中取得。
 */
static inline int sys_call(unsigned int n, unsigned int a1,
                           unsigned int a2, unsigned int a3)
{
    int ret;
    /* volatile 防止编译器把陷入优化掉。
     * "=a" 直接取 eax 返回值，避免多余拷贝。
     * "memory" 阻止编译器乱序，因为内核可能读写用户内存。 */
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(n), "b"(a1), "c"(a2), "d"(a3)
        : "memory");
    return ret;
}

#endif /* TURBOX_OS_API_SYSCALL_H */