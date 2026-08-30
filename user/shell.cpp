/* ============================================================
 * @file shell.cpp
 * @brief Turbox 交互式命令行
 *
 * 层级：
 *   TurboBoxOS/user/
 *
 * 项目内绝对路径：
 *   TurboBoxOS/user/shell.cpp
 *
 * 模块作用：
 *   实现用户态 shell，提供 create/write/run/ls/cat 等命令。
 *   只通过 turbox_api 暴露的 8 个系统调用工作，不依赖
 *   任何宿主 OS API。
 *
 * 使用者：
 *   kernel_main 将本文件入口作为进程启动。
 *
 * 项目角色：
 *   user 层的主要交互程序，是用户与内核之间的命令行界面。
 *
 * 引入说明：
 *   依赖 syscall/turbox_api.h 提供用户态系统调用。
 *
 * 维护记录：
 *   2026-08-30 初始创建
 * ============================================================ */

#include "../syscall/turbox_api.h"

/* ---------------- 自研微型字符串工具 ----------------
 * freestanding 环境无 libc，shell 作为独立用户进程自带最小实现，
 * 保持链接自洽。 */

// 计算字符串长度，不含结尾 '\0'
static unsigned int sh_strlen(const char* s)
{
    unsigned int n = 0;
    while (s[n] != '\0') { ++n; }
    return n;
}

// 判断两个字符串是否完全相等
static int sh_streq(const char* a, const char* b)
{
    while (*a != '\0' && *a == *b) { ++a; ++b; }
    return (*a == '\0' && *b == '\0') ? 1 : 0;
}

// 拷贝字符串，调用方保证 dst 足够大
static void sh_strcpy(char* dst, const char* src)
{
    while ((*dst++ = *src++) != '\0') { }
}

/* ---------------- 终端输出封装 ---------------- */

// 逐字符输出字符串，API 只有 putc，没有 puts
static void sh_puts(const char* s)
{
    while (*s != '\0') { tbx_putc(*s++); }
}

// 以十进制输出非负整数，用于 ls 显示句柄等
static void sh_putdec(unsigned int v)
{
    char buf[11];  // 32 位无符号最多 10 位 + '\0'
    int i = 0;

    if (v == 0) { tbx_putc('0'); return; }

    while (v > 0) { buf[i++] = (char)('0' + v % 10); v /= 10; }

    // 余数是倒序产生的，回倒输出
    while (i > 0) { tbx_putc(buf[--i]); }
}

/* ---------------- 会话文件表 ----------------
 * 记录本 shell 会话内 create 成功的 (名字, 句柄)，
 * 供 ls / cat / write / run 按名字找句柄。 */

#define SH_MAX_FILES 16
#define SH_NAME_LEN  32   // 与 TFS_NAME_LEN 保持一致

struct ShFile {
    char name[SH_NAME_LEN];
    int  fd;
    int  used;           // 0/1 标记槽位占用
};

static ShFile g_files[SH_MAX_FILES];

// 按名字查找会话文件表，未找到返回 0
static ShFile* sh_find(const char* name)
{
    for (int i = 0; i < SH_MAX_FILES; ++i) {
        if (g_files[i].used && sh_streq(g_files[i].name, name)) {
            return &g_files[i];
        }
    }
    return 0;
}

// 把新建文件登记进会话文件表，同名文件覆盖更新句柄
static ShFile* sh_register(const char* name, int fd)
{
    ShFile* f = sh_find(name);
    if (f) {
        f->fd = fd;
        return f;
    }

    for (int i = 0; i < SH_MAX_FILES; ++i) {
        if (!g_files[i].used) {
            g_files[i].used = 1;
            g_files[i].fd   = fd;
            sh_strcpy(g_files[i].name, name);
            return &g_files[i];
        }
    }
    return 0;  // 表满
}

/* ---------------- 命令实现 ---------------- */

static void cmd_help()
{
    sh_puts("Turbox shell commands:\n");
    sh_puts("  create <file>        create a file\n");
    sh_puts("  write <file> <text>  write text into file\n");
    sh_puts("  run <file>           run file as mini script\n");
    sh_puts("  ls                   list files created this session\n");
    sh_puts("  cat <file>           show file content\n");
    sh_puts("  help                 show this help\n");
    sh_puts("  exit                 halt the shell\n");
}

// 创建文件并登记到会话表
static void cmd_create(const char* name)
{
    int fd = tbx_create(name);
    if (fd < 0) {
        sh_puts("create: failed\n");
        return;
    }

    if (!sh_register(name, fd)) {
        sh_puts("create: session table full\n");
        return;
    }

    sh_puts("created ");
    sh_puts(name);
    sh_puts(" (fd=");
    sh_putdec((unsigned int)fd);
    sh_puts(")\n");
}

// 向已登记文件写入文本
static void cmd_write(const char* name, const char* text)
{
    ShFile* f = sh_find(name);
    if (!f) {
        // 没有 open 系统调用，只能操作本会话创建过的文件
        sh_puts("write: unknown file (create it first)\n");
        return;
    }

    int n = tbx_write(f->fd, text, sh_strlen(text));
    if (n < 0) {
        sh_puts("write: failed\n");
        return;
    }

    sh_puts("wrote ");
    sh_putdec((unsigned int)n);
    sh_puts(" bytes\n");
}

// 运行文件，交给内核 mini 解释器
static void cmd_run(const char* name)
{
    if (tbx_run(name) < 0) {
        sh_puts("run: failed\n");
    }
}

// 列出本会话创建的文件
static void cmd_ls()
{
    int count = 0;

    for (int i = 0; i < SH_MAX_FILES; ++i) {
        if (g_files[i].used) {
            sh_puts(g_files[i].name);
            sh_puts("\n");
            ++count;
        }
    }

    if (count == 0) { sh_puts("(no files)\n"); }
}

// 显示文件内容
static void cmd_cat(const char* name)
{
    ShFile* f = sh_find(name);
    if (!f) {
        sh_puts("cat: unknown file\n");
        return;
    }

    char buf[1024];  // 一次读 1KiB，够 mini 脚本体量
    int n = tbx_read(f->fd, buf, sizeof(buf) - 1);
    if (n < 0) {
        sh_puts("cat: read failed\n");
        return;
    }

    buf[n] = '\0';  // 读到的字节不一定有结尾符，手动补上
    sh_puts(buf);
    sh_puts("\n");
}

/* ---------------- 命令行解析 ---------------- */

// 跳过前导空格
static const char* sh_skip_spaces(const char* s)
{
    while (*s == ' ') { ++s; }
    return s;
}

// 取下一个空格分隔的词，词尾 '\0' 就地写入，p 指向下一位置
static char* sh_next_token(const char** p)
{
    const char* s = sh_skip_spaces(*p);
    if (*s == '\0') { *p = s; return 0; }

    char* tok = (char*)s;
    while (*s != '\0' && *s != ' ') { ++s; }

    if (*s == ' ') {
        *(char*)s = '\0';  // 就地截断，省掉临时缓冲
        ++s;
    }

    *p = s;
    return tok;
}

// 解析并执行一行命令，返回 1 表示请求退出 shell
static int sh_exec_line(char* line)
{
    const char* p = line;
    char* cmd = sh_next_token(&p);
    if (!cmd) { return 0; }  // 空行忽略

    if (sh_streq(cmd, "help")) {
        cmd_help();
    } else if (sh_streq(cmd, "exit")) {
        return 1;
    } else if (sh_streq(cmd, "ls")) {
        cmd_ls();
    } else if (sh_streq(cmd, "create")) {
        char* name = sh_next_token(&p);
        if (name) { cmd_create(name); } else { sh_puts("usage: create <file>\n"); }
    } else if (sh_streq(cmd, "write")) {
        char* name = sh_next_token(&p);
        // 文本参数取整行剩余部分，允许包含空格
        const char* text = sh_skip_spaces(p);
        if (name && *text != '\0') { cmd_write(name, text); }
        else { sh_puts("usage: write <file> <text>\n"); }
    } else if (sh_streq(cmd, "run")) {
        char* name = sh_next_token(&p);
        if (name) { cmd_run(name); } else { sh_puts("usage: run <file>\n"); }
    } else if (sh_streq(cmd, "cat")) {
        char* name = sh_next_token(&p);
        if (name) { cmd_cat(name); } else { sh_puts("usage: cat <file>\n"); }
    } else {
        sh_puts("unknown command: ");
        sh_puts(cmd);
        sh_puts(" (type help)\n");
    }
    return 0;
}

/*
 * shell 主循环：
 * 打印提示符 → 读一行 → 执行，直到 exit 命令结束。
 */
void shell_main()
{
    char line[256];  // 单行命令上限

    sh_puts("Turbox shell ready. type help\n");

    for (;;) {
        sh_puts("turbox> ");

        unsigned int len = 0;
        for (;;) {
            char c = (char)tbx_getc();  // 阻塞读键

            if (c == '\r' || c == '\n') {
                tbx_putc('\n');
                break;
            }
            if (c == '\b') {
                if (len > 0) { --len; sh_puts("\b \b"); }
                continue;
            }
            if (len < sizeof(line) - 1) {
                line[len++] = c;
                tbx_putc(c);  // 本地回显
            }
        }

        line[len] = '\0';
        if (sh_exec_line(line)) { break; }
    }

    sh_puts("shell exit\n");
    tbx_exit(0);
}