/* ============================================================
 * @file demo.cpp
 * @brief 用户程序：演示创建、写入、运行文件流程
 *
 * 层级：
 *   TurboBoxOS/user/
 *
 * 项目内绝对路径：
 *   TurboBoxOS/user/demo.cpp
 *
 * 模块作用：
 *   演示“创建文件 → 写入 mini 脚本 → 运行文件 → 读回验证”
 *   的完整用户态流程，全部通过 tbx_* 系统调用完成。
 *
 * 使用者：
 *   kernel 将本文件入口作为进程启动，也可被 shell 调用。
 *
 * 项目角色：
 *   user 层的基础演示程序，用来验证系统调用链路和前端终端效果。
 *
 * 引入说明：
 *   依赖 syscall/turbox_api.h 的用户态接口。
 *
 * 维护记录：
 *   2026-08-30 初始创建
 * ============================================================ */

#include "../syscall/turbox_api.h"

/*
 * 逐字符输出字符串。
 * 用户态 API 只有 putc，不用额外引入字符串输出 ABI。
 */
static void dm_puts(const char* s)
{
    while (*s != '\0') { tbx_putc(*s++); }
}

/*
 * 计算字符串长度，freestanding 环境下没有 libc，只能自带。
 */
static unsigned int dm_strlen(const char* s)
{
    unsigned int n = 0;
    while (s[n] != '\0') { ++n; }
    return n;
}

/*
 * 演示完整流程：
 * 1. 创建 hello.tbx
 * 2. 写入 mini 脚本内容
 * 3. 运行该文件，脚本输出会打印到终端
 * 4. 读回内容验证写入和运行的是同一份数据
 *
 * 任何一步失败都打印 FAIL 并退出，方便前端直接核对。
 */
void demo_main()
{
    // TurboxFS 内置 mini 命令集的 print 指令
    const char* script = "print Hello from Turbox demo!\n";

    // 步骤 1：创建文件
    dm_puts("[demo] create hello.tbx\n");
    int fd = tbx_create("hello.tbx");
    if (fd < 0) {
        dm_puts("[demo] FAIL: create\n");
        tbx_exit(1);
    }

    // 步骤 2：写入脚本
    dm_puts("[demo] write script\n");
    int n = tbx_write(fd, script, dm_strlen(script));
    if (n < 0) {
        dm_puts("[demo] FAIL: write\n");
        tbx_exit(1);
    }

    // 步骤 3：运行文件，脚本输出由内核 mini 解释器直接打印
    dm_puts("[demo] run hello.tbx\n");
    if (tbx_run("hello.tbx") < 0) {
        dm_puts("[demo] FAIL: run\n");
        tbx_exit(1);
    }

    // 步骤 4：读回内容，验证写入和运行的是同一份数据
    char buf[128];
    n = tbx_read(fd, buf, sizeof(buf) - 1);
    if (n < 0) {
        dm_puts("[demo] FAIL: read back\n");
        tbx_exit(1);
    }
    buf[n] = '\0';  // 读回字节不含结尾符，补上才能当字符串打印

    dm_puts("[demo] read back: ");
    dm_puts(buf);

    dm_puts("[demo] done\n");
    tbx_exit(0);
}