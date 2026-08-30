/* ============================================================
 * @file kernel.cpp
 * @brief 内核主流程入口与系统调用分发器
 *
 * 层级：
 *   TurboBoxOS/kernel/
 *
 * 项目内绝对路径：
 *   TurboBoxOS/kernel/kernel.cpp
 *
 * 模块作用：
 *   kernel_main 是保护模式下 C++ 世界的入口，负责按顺序初始化
 *   HAL、物理内存、进程表，创建 FS 服务进程和 shell 进程，
 *   最后进入 idle 调度循环。syscall_dispatch 根据调用号把
 *   用户态请求转发到对应模块。
 *
 * 使用者：
 *   loader 在保护模式下跳转到 kernel_main。
 *   用户程序通过 int 0x80 进入 syscall_dispatch。
 *
 * 项目角色：
 *   kernel 层的核心控制模块，把所有子系统串起来。
 *
 * 引入说明：
 *   依赖 kernel.h 和 mm/mm.h。
 *   通过 extern 声明使用 hal、drv、middle、user、fork 等模块。
 *
 * 维护记录：
 *   2026-08-30 初始创建
 * ============================================================ */
#include "kernel.h"
#include "../mm/mm.h"

/* ---- 其它层接口（extern 声明，链接时解析；签名取自 SPEC 4.4~4.6） ---- */

/* hal/：硬件抽象总初始化 */
extern void hal_init();

/* drv/：VGA 输出与键盘输入 */
extern void vga_init();
extern void vga_putc(char c);
extern void vga_puts(const char* s);
extern void kbd_init();
extern char kbd_getc();

/* middle/：虚拟文件系统层，内部经 IPC 适配到 tfs */
extern int  vfs_create(const char* name);
extern int  vfs_write(int fd, const char* data, unsigned int len);
extern int  vfs_run(const char* name);

/* fs/：读操作 middle 层未封装，直接声明 tfs 读接口 */
extern int  tfs_read(int fd, char* buf, unsigned int len);

/* middle/：FS 服务进程入口，Minix 式文件系统服务 */
extern void fs_server_loop();

/* user/：shell 进程入口，内核启动后以进程方式运行 */
extern void shell_main();

/* fork.cpp 内部初始化（同层私有接口） */
extern void procs_init();
extern void proc_exit_current();

/* ---- 系统调用号（与 os-api/syscall.h 的枚举一致） ---- */
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
 * 内核主入口。
 * 初始化顺序：HAL → 物理内存 → 进程表 → 打印横幅
 * → 创建 FS 服务进程（pid=2）→ 创建 shell 进程 → idle 循环。
 * 不返回。
 */
extern "C" void kernel_main()
{
    // 1. HAL 先行，否则输出和中断都不可用
    hal_init();

    // 2. 物理内存管理，先按 16MiB 保守初始化
    pmm_init(16u * 1024u);

    // 3. 进程表，必须在 fork 之前建立
    procs_init();

    // 4. 打印启动横幅，确认进入 C++ 世界
    vga_puts("Turbox microkernel: kernel_main entered\n");

    // 5a. 先 fork FS 服务进程，确保它占用 pid 2
    int fs_pid = kernel_fork(fs_server_loop);
    if (fs_pid < 0) {
        vga_puts("Turbox: failed to fork fs server\n");
    } else {
        vga_puts("Turbox: fs server forked\n");
    }

    // 5b. 再 fork shell 进程，验证 fork 路径并让策略外移
    int shell_pid = kernel_fork(shell_main);
    if (shell_pid < 0) {
        vga_puts("Turbox: failed to fork shell\n");
    } else {
        vga_puts("Turbox: shell forked\n");
    }

    // 6. 内核退化为 idle，不断让出 CPU
    for (;;) {
        sched_yield();
    }
}

/*
 * 系统调用总分发器。
 * 根据调用号转发到对应子系统，未知调用返回 -1。
 * a1~a3 参数含义依调用号而定，指针参数由 unsigned int 转回。
 */
int syscall_dispatch(unsigned int num, unsigned int a1, unsigned int a2, unsigned int a3)
{
    switch (num) {
    case SYS_FORK:
        // a1 = 入口函数指针
        return kernel_fork((void (*)())(unsigned long)a1);

    case SYS_EXIT:
        proc_exit_current();
        sched_yield();   // 立刻让出，避免僵尸继续跑
        return 0;

    case SYS_CREATE:
        // a1 = 文件名字符串指针
        return vfs_create((const char*)(unsigned long)a1);

    case SYS_WRITE:
        // a1 = 句柄，a2 = 数据指针，a3 = 长度
        return vfs_write((int)a1, (const char*)(unsigned long)a2, a3);

    case SYS_READ:
        // 读未走 VFS 封装，直接调 tfs
        return tfs_read((int)a1, (char*)(unsigned long)a2, a3);

    case SYS_RUN:
        // a1 = 文件名
        return vfs_run((const char*)(unsigned long)a1);

    case SYS_PUTC:
        // a1 低 8 位为待输出字符
        vga_putc((char)(a1 & 0xFFu));
        return 0;

    case SYS_GETC:
        // 阻塞读一个键，返回其字符值
        return (int)(unsigned char)kbd_getc();

    default:
        // 未知调用号，返回 -1 给用户态
        return -1;
    }
}