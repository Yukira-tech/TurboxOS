/* ============================================================
 * @file sched.cpp
 * @brief 协作式轮转调度器
 *
 * 层级：
 *   TurboBoxOS/kernel/
 *
 * 项目内绝对路径：
 *   TurboBoxOS/kernel/sched.cpp
 *
 * 模块作用：
 *   实现协作式轮转调度。没有抢占和上下文栈切换，
 *   sched_yield 只按轮转顺序挑选下一个 READY 进程并调用入口。
 *   进程只有在主动让出或阻塞在 IPC 时才交还 CPU。
 *
 * 使用者：
 *   kernel_main、ipc.cpp、syscall.cpp 等需要让出 CPU 的模块。
 *
 * 项目角色：
 *   kernel 层调度器实现，决定哪个进程接下来运行。
 *
 * 引入说明：
 *   依赖 kernel.h。
 *   依赖 fork.cpp 提供的 PCB 访问接口。
 *
 * 维护记录：
 *   2026-08-30 初始创建
 * ============================================================ */
#include "kernel.h"

/* fork.cpp 提供的进程表访问，同层私有 */
extern int  proc_current();
extern void proc_set_current(int idx);
extern int  proc_max();
extern int  proc_state_of(int idx);
extern void proc_run(int idx);

/* 与 fork.cpp 内部 ProcState 保持一致 */
#define ST_READY 1

/*
 * 轮转选择下一个 READY 进程并运行。
 * 没有其他就绪进程时立即返回，当前进程继续执行。
 */
void sched_yield()
{
    int n   = proc_max();
    int cur = proc_current();

    // 从 cur+1 开始环形扫描，保证轮转公平，
    // 槽位 0 是内核主上下文，不参与调度
    for (int off = 1; off <= n; ++off) {
        int idx = (cur + off) % n;
        if (idx == 0) continue;
        if (proc_state_of(idx) != ST_READY) continue;

        proc_set_current(idx);
        // 协作式切换：直接调用入口函数。
        // 进程应通过 sched_yield 或 IPC 阻塞交还 CPU；
        // 如果入口直接返回，也视作本轮执行结束
        proc_run(idx);
        proc_set_current(cur);  // 恢复当前进程记录
        return;
    }
    // 没有其他就绪进程，当前进程继续
}