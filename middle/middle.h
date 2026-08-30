/* ============================================================
 * @file middle.h
 * @brief 中间层接口声明
 *
 * 层级：
 *   TurboBoxOS/middle/
 *
 * 项目内绝对路径：
 *   TurboBoxOS/middle/middle.h
 *
 * 模块作用：
 *   声明 VFS 适配层和 IPC 适配层接口。中间层把用户态系统调用
 *   和 FS 服务解耦：本地路径直接转发给 tfs，微内核风格路径
 *   通过 IPC 委托给独立的 FS 服务进程。
 *
 * 使用者：
 *   kernel/syscall.cpp 系统调用分发、kernel/kernel.cpp 启动流程。
 *
 * 项目角色：
 *   middle 层的对外契约头，连接内核、文件系统服务和用户 API。
 *
 * 引入说明：
 *   不依赖其他头文件，纯函数声明。
 *
 * 维护记录：
 *   2026-08-30 初始创建
 * ============================================================ */

#ifndef TURBOX_MIDDLE_MIDDLE_H
#define TURBOX_MIDDLE_MIDDLE_H

/* 创建文件，直接转发给 tfs_create，成功返回句柄，失败返回 -1 */
int vfs_create(const char* name);

/* 写文件，直接转发给 tfs_write，返回实际写入字节数 */
int vfs_write(int fd, const char* data, unsigned int len);

/* 运行文件，通过 IPC 委托给 FS 服务进程执行，返回 0 表示成功 */
int vfs_run(const char* name);

/* FS 服务进程主循环，经 IPC 收请求、执行、应答，不返回 */
void fs_server_loop();

#endif /* TURBOX_MIDDLE_MIDDLE_H */