/* ============================================================
 * @file vfs.cpp
 * @brief VFS 适配层实现
 *
 * 层级：
 *   TurboBoxOS/middle/
 *
 * 项目内绝对路径：
 *   TurboBoxOS/middle/vfs.cpp
 *
 * 模块作用：
 *   实现 VFS 适配层。当前只有内存文件系统后端，
 *   create/write 直接转发给 turboxfs，run 走 IPC 适配路径。
 *   未来加入磁盘文件系统时，可在这里按挂载点路由。
 *
 * 使用者：
 *   syscall_dispatch 通过 middle.h 调用本模块。
 *
 * 项目角色：
 *   middle 层的 VFS 落地实现，是文件系统后端和上层接口之间的桥。
 *
 * 引入说明：
 *   依赖 middle.h 和 fs/turboxfs.h。
 *
 * 维护记录：
 *   2026-08-30 初始创建
 * ============================================================ */

#include "middle.h"
#include "../fs/turboxfs.h"

/*
 * 创建文件，直接转发给 turboxfs。
 * 当前没有权限或挂载检查，保留本函数作为未来路由点。
 */
int vfs_create(const char* name)
{
    return tfs_create(name);
}

/*
 * 写文件，直接转发给 turboxfs。
 */
int vfs_write(int fd, const char* data, unsigned int len)
{
    return tfs_write(fd, data, len);
}