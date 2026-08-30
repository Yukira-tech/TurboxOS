/* ============================================================
 * @file kernel.h
 * @brief 微内核核心接口声明
 *
 * 层级：
 *   TurboBoxOS/kernel/
 *
 * 项目内绝对路径：
 *   TurboBoxOS/kernel/kernel.h
 *
 * 模块作用：
 *   声明进程创建、IPC、调度和系统调用分发接口。
 *   os-api、user、middle 层包含本头文件即可使用内核服务，
 *   PCB 和消息队列等实现细节封装在各 .cpp 内部。
 *
 * 使用者：
 *   user/、middle/、os-api/ 等上层模块。
 *
 * 项目角色：
 *   kernel 层的对外契约头，是内核与上层之间的唯一接口。
 *
 * 引入说明：
 *   不依赖其他头文件。
 *
 * 维护记录：
 *   2026-08-30 初始创建
 * ============================================================ */

#ifndef TURBOX_KERNEL_H
#define TURBOX_KERNEL_H

/* 以下签名是 SPEC 4.2 接口契约，不得修改 */

/* loader 跳转后的内核入口，extern "C" 防止名字修饰 */
extern "C" void kernel_main();

/* 创建进程，返回 pid */
int  kernel_fork(void (*entry)());

/* 微内核 IPC 发送，成功返回 0 */
int  ipc_send(int dest, int msg);

/* 接收 IPC 消息，src 非空时写入发送者 pid */
int  ipc_recv(int* src);

/* 协作式让出 CPU，单线程语义下轮转调度 */
void sched_yield();

/* 系统调用分发入口。
 * os-api 层通过 int 0x80 进入本函数。
 * 必须 extern "C"，因为 syscall_stub.asm 按未修饰符号调用，
 * 带 C++ 名字修饰会导致链接失败。 */
extern "C" int syscall_dispatch(unsigned int num, unsigned int a1, unsigned int a2, unsigned int a3);

#endif /* TURBOX_KERNEL_H */