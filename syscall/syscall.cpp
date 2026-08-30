/* ============================================================
 * @file syscall.cpp
 * @brief 用户态系统调用 API 封装
 *
 * 层级：
 *   TurboBoxOS/syscall/
 *
 * 项目内绝对路径：
 *   TurboBoxOS/syscall/syscall.cpp
 *
 * 模块作用：
 *   实现 turbox_api.h 声明的 tbx_* 用户态 API。每个函数都是
 *   sys_call 的薄封装，把类型安全参数转成寄存器值，保持 ABI
 *   单点收敛。
 *
 * 使用者：
 *   user/demo.cpp 和 user/shell.cpp 等用户态代码。
 *
 * 项目角色：
 *   syscall 层实现，提供用户态访问内核能力的入口。
 *
 * 引入说明：
 *   依赖 syscall.h 和 turbox_api.h。
 *
 * 维护记录：
 *   2026-08-30 初始创建
 * ============================================================ */

#include "syscall.h"
#include "turbox_api.h"

// 指针转寄存器值：i686 上 unsigned int 与指针同为 32 位
static inline unsigned int ptr_to_u32(const void* p)
{
    return (unsigned int)p;
}

/*
 * 创建新进程，返回新 pid，失败返回负值。
 */
int tbx_fork(void (*entry)())
{
    return sys_call(SYS_FORK, ptr_to_u32((const void*)entry), 0, 0);
}

/*
 * 结束当前进程。正常路径不会返回。
 */
void tbx_exit(int code)
{
    sys_call(SYS_EXIT, (unsigned int)code, 0, 0);
    // 兜底死循环，防止内核未回收时继续执行
    for (;;) { }
}

/*
 * 创建文件，返回句柄，失败返回负值。
 */
int tbx_create(const char* name)
{
    return sys_call(SYS_CREATE, ptr_to_u32(name), 0, 0);
}

/*
 * 向文件写入数据，返回实际写入字节数，失败返回负值。
 */
int tbx_write(int fd, const char* data, unsigned int len)
{
    return sys_call(SYS_WRITE, (unsigned int)fd, ptr_to_u32(data), len);
}

/*
 * 从文件读取数据，返回实际读取字节数，失败返回负值。
 */
int tbx_read(int fd, char* buf, unsigned int len)
{
    return sys_call(SYS_READ, (unsigned int)fd, ptr_to_u32(buf), len);
}

/*
 * 解释运行文件内容，返回 0 成功，负值失败。
 */
int tbx_run(const char* name)
{
    return sys_call(SYS_RUN, ptr_to_u32(name), 0, 0);
}

/*
 * 输出一个字符，返回 0 成功。
 */
int tbx_putc(char c)
{
    return sys_call(SYS_PUTC, (unsigned int)(unsigned char)c, 0, 0);
}

/*
 * 阻塞读取一个键盘字符，返回字符值。
 */
int tbx_getc()
{
    return sys_call(SYS_GETC, 0, 0, 0);
}