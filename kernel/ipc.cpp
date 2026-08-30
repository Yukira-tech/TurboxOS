/* ============================================================
 * @file ipc.cpp
 * @brief 微内核进程间通信
 *
 * 层级：
 *   TurboBoxOS/kernel/
 *
 * 项目内绝对路径：
 *   TurboBoxOS/kernel/ipc.cpp
 *
 * 模块作用：
 *   提供简化版 Minix rendezvous IPC。消息是单个 int，
 *   投递到目标 PCB 里的单字信箱。发送方装满信箱后通过
 *   协作式让出等待对方读走，接收方空箱时同样让出等待。
 *   用自旋让出替代真正的阻塞，适合当前单线程协作式内核。
 *
 * 使用者：
 *   syscall.cpp 系统调用入口、user/demo 或 user/shell 的
 *   上层封装最终调用本模块。
 *
 * 项目角色：
 *   kernel 层的消息传递实现，让进程间可以互相通信，
 *   是微内核“服务化”的基础。
 *
 * 引入说明：
 *   依赖 kernel.h 的接口声明。
 *   依赖 sched_yield() 实现协作式让出。
 *   依赖 fork.cpp 提供的 PCB 信箱操作接口。
 *
 * 维护记录：
 *   2026-08-30 初始创建
 * ============================================================ */

#include "kernel.h"

/* fork.cpp 提供的进程表访问接口，同层私有 */
extern int proc_current();
extern int proc_max();
extern int proc_state_of(int idx);
extern int proc_ipc_deliver(int idx, int msg, int from);
extern int proc_ipc_take(int idx, int* src);

/* 与 fork.cpp 内部 ProcState 保持一致 */
#define ST_FREE 0

/* 自旋让出上限，防止发送方或接收方因为对端消失而永远空转 */
#define IPC_SPIN_LIMIT 1024

/*
 * 向目标进程发送一个消息字。
 * 成功返回 0，目标不存在或长时间无法送达返回 -1。
 */
int ipc_send(int dest, int msg)
{
    // pid = 槽位下标 + 1，这里反推目标槽位
    int idx = dest - 1;

    // 槽位 0 是内核主上下文，不允许作为 IPC 目标；
    // 超出表范围的 pid 也直接判非法
    if (idx < 1 || idx >= proc_max()) return -1;

    // 目标槽位空闲说明进程不存在，没有信箱可以投递
    if (proc_state_of(idx) == ST_FREE) return -1;

    int from_pid = proc_current() + 1;

    // 单字信箱被占时不能覆盖，必须让出 CPU 等待目标读走旧消息。
    // 有次数上限，避免目标进程已死亡但状态未及时回收时死循环。
    for (int spin = 0; spin < IPC_SPIN_LIMIT; ++spin) {
        if (proc_ipc_deliver(idx, msg, from_pid)) return 0;
        sched_yield();
    }

    return -1;  // 超时仍未送达，按失败处理
}

/*
 * 接收一个消息字。
 * src 非空时写入发送者 pid。
 * 有消息返回消息内容，超时无消息返回 -1。
 */
int ipc_recv(int* src)
{
    int self = proc_current();

    // 信箱为空就让出，等待发送方投递。
    // 同样限制次数，防止没有发送者时接收方永远空转。
    for (int spin = 0; spin < IPC_SPIN_LIMIT; ++spin) {
        int m = proc_ipc_take(self, src);
        if (m != -1) return m;
        sched_yield();
    }

    return -1;  // 超时仍未收到消息
}