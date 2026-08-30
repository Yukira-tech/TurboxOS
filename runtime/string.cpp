/* ============================================================
 * @file string.cpp
 * @brief freestanding 内存/字符串函数实现
 *
 * 层级：
 *   TurboBoxOS/runtime/
 *
 * 项目内绝对路径：
 *   TurboBoxOS/runtime/string.cpp
 *
 * 模块作用：
 *   提供内核自用的 memcpy、memset、strlen、strcmp 等基础函数，
 *   完全不依赖 libc。
 *
 * 使用者：
 *   kernel、mm、fs、middle 等所有需要内存和字符串操作的内核模块。
 *
 * 项目角色：
 *   runtime 层的落地实现，是内核最底层的基础工具库。
 *
 * 引入说明：
 *   依赖 runtime.h 声明接口。
 *   编译参数 -ffreestanding -nostdlib -fno-exceptions -fno-rtti
 *
 * 维护记录：
 *   2026-08-30 初始创建
 * ============================================================ */

#include "runtime.h"

/*
 * 从 src 拷贝 n 字节到 dst，调用方需保证两区间不重叠。
 */
void* memcpy(void* dst, const void* src, size_t n) {
    unsigned char* d = (unsigned char*)dst;
    const unsigned char* s = (const unsigned char*)src;
    for (size_t i = 0; i < n; ++i) {
        d[i] = s[i];
    }
    return dst;
}

/*
 * 把 dst 开始的 n 字节填充为 c 的低 8 位。
 */
void* memset(void* dst, int c, size_t n) {
    unsigned char* d = (unsigned char*)dst;
    for (size_t i = 0; i < n; ++i) {
        d[i] = (unsigned char)c;
    }
    return dst;
}

/*
 * 比较两块内存的前 n 字节，返回第一处差异的差值。
 */
int memcmp(const void* a, const void* b, size_t n) {
    const unsigned char* pa = (const unsigned char*)a;
    const unsigned char* pb = (const unsigned char*)b;
    for (size_t i = 0; i < n; ++i) {
        if (pa[i] != pb[i]) return (int)pa[i] - (int)pb[i];
    }
    return 0;
}

/*
 * 返回字符串长度，不含结尾 '\0'。
 */
size_t strlen(const char* s) {
    size_t len = 0;
    while (s[len] != '\0') ++len;
    return len;
}

/*
 * 比较两个字符串，返回第一处差异的字符差值。
 * 必须用 unsigned char 比较，避免高位字节符号扩展导致顺序错误。
 */
int strcmp(const char* a, const char* b) {
    while (*a && (*a == *b)) { ++a; ++b; }
    return (int)(unsigned char)*a - (int)(unsigned char)*b;
}

/*
 * 比较前 n 个字符，遇到 '\0' 或差异时提前返回。
 */
int strncmp(const char* a, const char* b, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        unsigned char ca = (unsigned char)a[i];
        unsigned char cb = (unsigned char)b[i];
        if (ca != cb || ca == '\0') return (int)ca - (int)cb;
    }
    return 0;
}

/*
 * 拷贝 src 到 dst，最多 n 字符，源串短于 n 时用 '\0' 填满，
 * 和 libc 语义保持一致，避免残留旧数据。
 */
char* strncpy(char* dst, const char* src, size_t n) {
    size_t i = 0;
    for (; i < n && src[i] != '\0'; ++i) dst[i] = src[i];
    for (; i < n; ++i) dst[i] = '\0';
    return dst;
}

/*
 * 把整数转为 base 进制字符串，base 非法时兜底按十进制。
 * 仅十进制显示负号；其他进制按无符号补码处理，方便打印地址。
 * INT_MIN 通过 +1 再补 1 的方式避免取负溢出。
 */
char* itoa(int value, char* buf, int base) {
    static const char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    char tmp[33];
    int i = 0, pos = 0;
    int negative = 0;

    if (base < 2 || base > 36) base = 10;

    unsigned int v;
    if (value < 0 && base == 10) {
        negative = 1;
        v = (unsigned int)(-(value + 1)) + 1;
    } else {
        v = (unsigned int)value;
    }

    if (v == 0) {
        tmp[i++] = '0';
    }
    while (v != 0) {
        tmp[i++] = digits[v % (unsigned int)base];
        v /= (unsigned int)base;
    }

    if (negative) buf[pos++] = '-';
    while (i > 0) buf[pos++] = tmp[--i];
    buf[pos] = '\0';
    return buf;
}