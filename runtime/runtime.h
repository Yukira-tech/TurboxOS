/* ============================================================
 * @file runtime.h
 * @brief freestanding 运行时基础接口声明
 *
 * 层级：
 *   TurboBoxOS/runtime/
 *
 * 项目内绝对路径：
 *   TurboBoxOS/runtime/runtime.h
 *
 * 模块作用：
 *   提供 freestanding 环境下必需的字符串和内存操作接口。
 *   内核不链接 libc，这些基础函数全部自研。
 *
 * 使用者：
 *   kernel、mm、fs、middle 等所有需要内存和字符串操作的内核模块。
 *
 * 项目角色：
 *   runtime 层对外契约头，是内核最底层的基础工具库。
 *
 * 引入说明：
 *   不依赖其他头文件。
 *   编译参数 -ffreestanding -nostdlib -fno-exceptions -fno-rtti
 *
 * 维护记录：
 *   2026-08-30 初始创建
 * ============================================================ */

#ifndef TURBOX_RUNTIME_H
#define TURBOX_RUNTIME_H

/* freestanding 环境没有 <stddef.h>，自定义等价的 size_t */
typedef unsigned int size_t;

extern "C" {
/* 使用 C 链接，便于汇编和其他语言混用，同时避免 C++ 名字修饰 */

/* 内存拷贝，返回 dst */
void*  memcpy(void* dst, const void* src, size_t n);

/* 内存填充，返回 dst */
void*  memset(void* dst, int c, size_t n);

/* 内存比较，相等返回 0 */
int    memcmp(const void* a, const void* b, size_t n);

/* 计算字符串长度，不含结尾 '\0' */
size_t strlen(const char* s);

/* 字符串比较，相等返回 0 */
int    strcmp(const char* a, const char* b);

/* 字符串前 n 个字符比较 */
int    strncmp(const char* a, const char* b, size_t n);

/* 拷贝至多 n 个字符到 dst */
char*  strncpy(char* dst, const char* src, size_t n);

/* 整数转字符串，buf 至少 33 字节，返回 buf */
char*  itoa(int value, char* buf, int base);
}

#endif /* TURBOX_RUNTIME_H */