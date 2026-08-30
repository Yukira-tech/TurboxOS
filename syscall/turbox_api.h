/* ============================================================
 * @file turbox_api.h
 * @brief 面向用户程序的 Turbox API 声明
 *
 * 层级：
 *   TurboBoxOS/syscall/
 *
 * 项目内绝对路径：
 *   TurboBoxOS/syscall/turbox_api.h
 *
 * 模块作用：
 *   为用户程序提供统一的 tbx_* 系统调用封装接口。
 *   用户程序只包含本头文件，不直接接触系统调用号和内联汇编，
 *   内核 ABI 变化时用户代码无需改动。
 *
 * 使用者：
 *   user/demo.cpp 和 user/shell.cpp 等用户态程序。
 *
 * 项目角色：
 *   syscall 层的对外 API 头，是用户程序访问内核能力的唯一入口。
 *
 * 引入说明：
 *   不依赖其他头文件。
 *
 * 维护记录：
 *   2026-08-30 初始创建
 * ============================================================ */

#ifndef TURBOX_OS_API_TURBOX_API_H
#define TURBOX_OS_API_TURBOX_API_H

/* 创建新进程，入口为 entry 函数。成功返回 pid，失败返回负值。 */
int tbx_fork(void (*entry)());

/* 结束当前进程。不返回。 */
void tbx_exit(int code);

/* 创建文件，成功返回句柄，失败返回负值。 */
int tbx_create(const char* name);

/* 写文件，返回实际写入字节数，失败返回负值。 */
int tbx_write(int fd, const char* data, unsigned int len);

/* 读文件，返回实际读取字节数，失败返回负值。 */
int tbx_read(int fd, char* buf, unsigned int len);

/* 运行文件，返回结果码，0 成功，负值失败。 */
int tbx_run(const char* name);

/* 向终端输出一个字符，返回 0 成功。 */
int tbx_putc(char c);

/* 阻塞读取一个键盘字符，返回字符值。 */
int tbx_getc();

#endif /* TURBOX_OS_API_TURBOX_API_H */