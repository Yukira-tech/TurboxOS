/* ============================================================
 * @file turboxfs.cpp
 * @brief TurboBox 内存文件系统实现
 *
 * 层级：
 *   TurboBoxOS/fs/
 *
 * 项目内绝对路径：
 *   TurboBoxOS/fs/turboxfs.cpp
 *
 * 模块作用：
 *   提供无堆依赖的静态内存文件表，支持文件创建、追加写入、
 *   读取和列表。文件内容可以被当作 mini 脚本逐行解释执行，
 *   让“运行文件”在还没有可执行格式加载器时也能演示。
 *
 * 使用者：
 *   上层通过 middle/VFS/IPC 适配调用本模块，不直接操作文件表。
 *
 * 项目角色：
 *   fs 层的自包含文件系统服务，是系统中“文件”概念的落地实现。
 *
 * 引入说明：
 *   依赖 turboxfs.h 的接口定义和 drv/drv.h 的 VGA 输出。
 *
 * 维护记录：
 *   2026-08-30 初始创建
 * ============================================================ */

#include "turboxfs.h"

// 文件系统只通过 drv.h 输出，不直接碰显存，保持层次单向依赖
#include "../drv/drv.h"

/* ---------- 自研 C 字符串辅助（freestanding，无 libc） ---------- */

// 计算字符串长度，不含结尾 '\0'
static unsigned int tfs_strlen(const char* s)
{
    unsigned int n = 0;
    while (s[n] != '\0') {
        ++n;
    }
    return n;
}

// 字符串拷贝，调用方必须保证 dst 足够大
static void tfs_strcpy(char* dst, const char* src)
{
    while ((*dst++ = *src++) != '\0') {
    }
}

// 比较两个字符串是否完全相等，相等返回 1
static int tfs_streq(const char* a, const char* b)
{
    while (*a != '\0' && *a == *b) {
        ++a;
        ++b;
    }
    return (*a == *b) ? 1 : 0;
}

// 比较前 n 个字符是否相等，用于命令前缀匹配
static int tfs_strneq(const char* a, const char* b, unsigned int n)
{
    for (unsigned int i = 0; i < n; ++i) {
        if (a[i] != b[i] || a[i] == '\0') {
            return 0;
        }
    }
    return 1;
}

/* ---------- 文件表（静态分配，避免 freestanding 下的堆不确定性） ---------- */

struct TfsFile {
    char name[TFS_NAME_LEN];  // 文件名定长，简化比较与拷贝
    char data[TFS_MAX_SIZE];  // 文件内容内联存储，免堆分配
    unsigned int size;        // 当前有效字节数
    int used;                 // 槽位是否被占用
};

static struct TfsFile g_files[TFS_MAX_FILES];  // 全局文件表
static int g_tfs_ready = 0;                    // 是否已初始化

// 按名字查找文件，找到返回句柄，未找到返回 -1
static int tfs_find(const char* name)
{
    for (int i = 0; i < TFS_MAX_FILES; ++i) {
        if (g_files[i].used && tfs_streq(g_files[i].name, name)) {
            return i;
        }
    }
    return -1;
}

// 初始化文件系统，清空全部槽位并标记可用
int tfs_init()
{
    for (int i = 0; i < TFS_MAX_FILES; ++i) {
        g_files[i].used = 0;
        g_files[i].size = 0;
        g_files[i].name[0] = '\0';
    }
    g_tfs_ready = 1;
    return 0;
}

// 创建空文件；同名文件已存在则返回原句柄，保证幂等
int tfs_create(const char* name)
{
    if (!g_tfs_ready || name == 0 || name[0] == '\0') {
        return -1;
    }
    if (tfs_strlen(name) >= TFS_NAME_LEN) {
        return -1;  // 名字过长会破坏定长槽位
    }

    int exist = tfs_find(name);
    if (exist >= 0) {
        return exist;
    }

    for (int i = 0; i < TFS_MAX_FILES; ++i) {
        if (!g_files[i].used) {
            g_files[i].used = 1;
            g_files[i].size = 0;
            tfs_strcpy(g_files[i].name, name);
            return i;
        }
    }
    return -1;  // 表满
}

// 向文件末尾追加数据；容量不足时截断写入，返回实际写入字节数
int tfs_write(int fd, const char* data, unsigned int len)
{
    if (fd < 0 || fd >= TFS_MAX_FILES || !g_files[fd].used || data == 0) {
        return -1;
    }

    unsigned int room = TFS_MAX_SIZE - g_files[fd].size;
    unsigned int n = (len <= room) ? len : room;
    for (unsigned int i = 0; i < n; ++i) {
        g_files[fd].data[g_files[fd].size + i] = data[i];
    }
    g_files[fd].size += n;
    return (int)n;
}

// 从文件开头读取数据，返回实际读取字节数
int tfs_read(int fd, char* buf, unsigned int len)
{
    if (fd < 0 || fd >= TFS_MAX_FILES || !g_files[fd].used || buf == 0) {
        return -1;
    }
    unsigned int n = (len <= g_files[fd].size) ? len : g_files[fd].size;
    for (unsigned int i = 0; i < n; ++i) {
        buf[i] = g_files[fd].data[i];
    }
    return (int)n;
}

/* ---------- mini 命令解释器 ---------- */

// 执行一行 mini 命令；返回 0 表示遇到 exit，终止脚本
static int tfs_exec_line(const char* line)
{
    while (*line == ' ' || *line == '\t') {
        ++line;  // 跳过前导空格
    }
    if (*line == '\0' || *line == '#') {
        return 1;  // 空行和注释行跳过
    }

    // print <text>：原样输出剩余文本并换行
    if (tfs_strneq(line, "print", 5) && (line[5] == ' ' || line[5] == '\0')) {
        const char* p = line + 5;
        while (*p == ' ') {
            ++p;
        }
        vga_puts(p);
        vga_putc('\n');
        return 1;
    }

    // putc <char>：输出单个字符，不换行，供脚本拼字符画
    if (tfs_strneq(line, "putc", 4) && (line[4] == ' ' || line[4] == '\0')) {
        const char* p = line + 4;
        while (*p == ' ') {
            ++p;
        }
        if (*p != '\0') {
            vga_putc(*p);
        }
        return 1;
    }

    // exit：终止脚本
    if (tfs_strneq(line, "exit", 4)) {
        return 0;
    }

    // 未知命令报错但继续，方便调试脚本
    vga_puts("[tfs] unknown cmd: ");
    vga_puts(line);
    vga_putc('\n');
    return 1;
}

// 按句柄运行文件，逐行执行 mini 命令
int tfs_run_fd(int fd)
{
    if (fd < 0 || fd >= TFS_MAX_FILES || !g_files[fd].used) {
        return -1;
    }

    char line[128];  // 固定行缓冲，限制栈使用
    unsigned int li = 0;
    int running = 1;

    for (unsigned int i = 0; i < g_files[fd].size && running; ++i) {
        char c = g_files[fd].data[i];
        if (c == '\r') {
            continue;  // 忽略 CR，兼容 Windows 换行
        }
        if (c == '\n') {
            line[li] = '\0';
            running = tfs_exec_line(line);
            li = 0;
        } else if (li < sizeof(line) - 1) {
            line[li++] = c;  // 超长行静默截断，避免越界
        }
    }

    // 文件末尾没有换行时，最后一行也要执行
    if (running && li > 0) {
        line[li] = '\0';
        tfs_exec_line(line);
    }
    return 0;
}

// 按文件名运行文件，委托给 tfs_run_fd
int tfs_run(const char* name)
{
    return tfs_run_fd(tfs_find(name));
}

// 列出全部文件：名称 + 大小（十进制）
void tfs_list()
{
    char num[12];  // 32 位无符号最多 10 位数字

    for (int i = 0; i < TFS_MAX_FILES; ++i) {
        if (!g_files[i].used) {
            continue;
        }

        vga_puts(g_files[i].name);
        vga_puts("  ");

        // 无 sprintf，手工做无符号十进制转换
        unsigned int v = g_files[i].size;
        int pos = 0;

        if (v == 0) {
            num[pos++] = '0';
        } else {
            char tmp[10];
            int ti = 0;
            while (v > 0) {
                tmp[ti++] = (char)('0' + (v % 10));
                v /= 10;
            }
            while (ti > 0) {
                num[pos++] = tmp[--ti];  // 逆序回填得到正确顺序
            }
        }
        num[pos] = '\0';

        vga_puts(num);
        vga_puts(" bytes\n");
    }
}