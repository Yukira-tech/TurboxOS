/* ============================================================
 * @file ipc_adapter.cpp
 * @brief IPC 适配层，以消息方式调用 FS 服务
 *
 * 层级：
 *   TurboBoxOS/middle/
 *
 * 项目内绝对路径：
 *   TurboBoxOS/middle/ipc_adapter.cpp
 *
 * 模块作用：
 *   实现 Minix 风格的 IPC 委托调用。文件系统不是内核里的
 *   一组函数，而是一个独立服务进程；调用方通过 kernel IPC
 *   发消息委托执行，再阻塞等应答。本文件同时实现调用方路径
 *   vfs_run 和服务侧循环 fs_server_loop。
 *
 * 使用者：
 *   syscall_dispatch 调用 vfs_run；
 *   kernel_main 把 fs_server_loop 作为进程入口 fork。
 *
 * 项目角色：
 *   middle 层核心，连接内核 IPC 与文件系统服务。
 *
 * 引入说明：
 *   依赖 middle.h 和 fs/turboxfs.h，
 *   依赖 kernel 层的 ipc_send / ipc_recv 接口。
 *
 * 维护记录：
 *   2026-08-30 初始创建
 * ============================================================ */

#include "middle.h"
#include "../fs/turboxfs.h"

/* kernel 层提供的 IPC 接口，签名与 SPEC 4.2 一致 */
extern int ipc_send(int dest, int msg);
extern int ipc_recv(int* src);

/* FS 服务固定占用 pid 2：
 * pid 1 是内核主上下文，kernel_main 在 fork shell 之前先 fork
 * fs_server_loop，因此 FS 服务稳定拿到 pid 2 */
#define FS_SERVER_PID 2

/* IPC 载荷只有单个 int，无法传指针或字符串。
 * 请求消息 = 操作码(低8位) | (fd << 8)，应答消息 = 结果码。
 * 这对应真实微内核中“先解析得到句柄，再按句柄请求”的惯例 */
#define IPC_OP_RUN 0x01

/*
 * 调用方路径：运行文件。
 * 1. 先把文件名解析成 fd，因为 IPC 消息只能携带 int；
 * 2. 向 FS 服务发送“运行 fd”请求；
 * 3. 协作式调度下控制权切到服务进程执行；
 * 4. 阻塞等应答。
 * 若 IPC 尚未就绪（ipc_send 返回负值），退化为本地直接调用 tfs_run，
 * 保证分阶段集成时功能仍可用。
 */
int vfs_run(const char* name)
{
    // tfs_create 是幂等的，已存在则返回现有句柄
    int fd = tfs_create(name);
    if (fd < 0) {
        return -1;
    }

    // 委托给 FS 服务进程，而不是自己直接执行
    int sent = ipc_send(FS_SERVER_PID, IPC_OP_RUN | (fd << 8));
    if (sent < 0) {
        // 最小内核 fallback：无服务进程时本地直调
        return tfs_run(name);
    }

    // 协作式调度保证应答必然送达，这里不需要超时机制
    int src = 0;
    int reply = ipc_recv(&src);
    (void)src; // 单 FS 服务模型，不校验来源

    // 约定应答 0 为成功，其余视为失败
    return (reply == 0) ? 0 : -1;
}

/*
 * FS 服务进程主循环：收请求 → 执行 → 回应答，永不返回。
 * 由 kernel 通过 fork 启动，作为服务进程入口。
 */
void fs_server_loop()
{
    for (;;) {
        int src = 0;
        int msg = ipc_recv(&src); // 阻塞等待调用方请求

        int op = msg & 0xFF;
        int fd = (msg >> 8) & 0xFFFFFF;
        int result = -1;

        if (op == IPC_OP_RUN) {
            // 服务侧与 FS 同层，按句柄直接执行；非法句柄返回 -1
            result = tfs_run_fd(fd);
        }
        // 未知操作码保持 result = -1，让调用方感知协议错误

        ipc_send(src, result); // rendezvous 应答，唤醒调用方
    }
}