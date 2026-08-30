/* ============================================================
 * @file fork.cpp
 * @brief 进程创建与 PCB 表管理
 *
 * 层级：
 *   TurboBoxOS/kernel/
 *
 * 项目内绝对路径：
 *   TurboBoxOS/kernel/fork.cpp
 *
 * 模块作用：
 *   管理固定 16 项的静态 PCB 表，提供进程创建、退出、
 *   状态修改和 IPC 信箱操作。协作式单线程下进程不是独立
 *   地址空间，只是一个可恢复的执行入口。
 *
 * 使用者：
 *   kernel/sched.cpp 调度器、kernel/ipc.cpp 消息传递、
 *   kernel/syscall.cpp 系统调用入口。
 *
 * 项目角色：
 *   kernel 层的任务管理基础模块，是调度和 IPC 共同依赖的
 *   数据源。
 *
 * 引入说明：
 *   依赖 kernel.h 提供进程相关类型与接口声明。
 *   依赖 runtime 提供的 memset。
 *
 * 维护记录：
 *   2026-08-30 初始创建
 * ============================================================ */
#include "kernel.h"

// runtime 自研内存函数
extern "C" void* memset(void* dst, int val, unsigned int n);

/* ---- PCB 定义 ---- */

#define MAX_PROCS 16   // 进程表容量上限

enum ProcState {
    PROC_FREE = 0,     // 空槽位
    PROC_READY,        // 就绪，可被调度执行
    PROC_BLOCKED,      // 阻塞，等待 IPC 消息
    PROC_ZOMBIE        // 已退出，等待回收
};

struct PCB {
    int           pid;         // 进程号 = 槽位下标 + 1，0 表示无效
    ProcState     state;       // 调度状态
    void        (*entry)();    // 入口函数，协作式下从这里开始执行
    int           ipc_msg;     // 最近一次收到的消息内容
    int           ipc_from;    // 消息发送者 pid
    int           ipc_pending; // 1 表示信箱里有未读消息
};

static PCB  g_procs[MAX_PROCS];   // 静态 PCB 表
static int  g_current = 0;        // 当前运行进程下标，0 代表内核主上下文
static int  g_next_pid_hint = 1;  // 下一次 pid 分配的起始查找位置

/* 同层私有接口：供 sched.cpp 和 ipc.cpp 使用，不进公共头文件 */
PCB* proc_table()           { return g_procs; }
int  proc_current()         { return g_current; }
void proc_set_current(int idx) { g_current = idx; }
int  proc_max()             { return MAX_PROCS; }

/*
 * 初始化进程表，槽位 0 保留给内核主上下文。
 * 必须在调度开始前调用一次。
 */
void procs_init()
{
    memset(g_procs, 0, sizeof(g_procs));

    // pid 1 固定给内核主上下文，永远存在，不需要入口函数
    g_procs[0].pid   = 1;
    g_procs[0].state = PROC_READY;
    g_procs[0].entry = 0;

    g_current = 0;
    g_next_pid_hint = 1;
}

/*
 * 用指定入口创建新进程。
 * 成功返回 pid，失败返回 -1。
 */
int kernel_fork(void (*entry)())
{
    if (entry == 0) return -1;  // 没有入口的进程无法运行

    // 从上次分配位置开始环形查找空槽，避免每次从头扫描
    for (int off = 0; off < MAX_PROCS - 1; ++off) {
        int idx = 1 + (g_next_pid_hint - 1 + off) % (MAX_PROCS - 1);
        if (g_procs[idx].state == PROC_FREE) {
            memset(&g_procs[idx], 0, sizeof(PCB));

            // pid 和槽位绑定，保证 pid 稳定且容易反查
            g_procs[idx].pid   = idx + 1;
            g_procs[idx].state = PROC_READY;
            g_procs[idx].entry = entry;

            g_next_pid_hint = idx + 1;
            return g_procs[idx].pid;
        }
    }
    return -1;  // 16 个槽位全部占用
}

/*
 * 当前进程退出并回收槽位。
 * 内核主上下文不可退出，其他进程退出后统一回到内核主上下文。
 */
void proc_exit_current()
{
    if (g_current == 0) return;

    // 协作式单线程下没有等待回收的父进程语义，直接置空
    g_procs[g_current].state       = PROC_FREE;
    g_procs[g_current].entry       = 0;
    g_procs[g_current].ipc_pending = 0;

    g_current = 0;
}

// 供 sched.cpp 查询指定槽位状态
ProcState proc_state_of(int idx) { return g_procs[idx].state; }

/*
 * 执行指定槽位的入口函数。
 * 入口为空时不做任何事，由调度器保证传入的是 READY 槽位。
 */
void proc_run(int idx)
{
    if (g_procs[idx].entry != 0)
        g_procs[idx].entry();
}

// 供 ipc.cpp 阻塞/唤醒进程时修改状态
void proc_set_state(int idx, int st) { g_procs[idx].state = (ProcState)st; }

/*
 * 向目标进程投递消息。
 * 单字信箱：已有一条未读消息时拒绝投递，返回 0。
 */
int proc_ipc_deliver(int idx, int msg, int from)
{
    if (g_procs[idx].ipc_pending) return 0;

    g_procs[idx].ipc_msg     = msg;
    g_procs[idx].ipc_from    = from;
    g_procs[idx].ipc_pending = 1;
    return 1;
}

/*
 * 取走指定进程信箱中的消息。
 * 无消息返回 -1；src 非空时写入发送者 pid。
 */
int proc_ipc_take(int idx, int* src)
{
    if (!g_procs[idx].ipc_pending) return -1;

    if (src != 0) *src = g_procs[idx].ipc_from;

    g_procs[idx].ipc_pending = 0;
    return g_procs[idx].ipc_msg;
}