/* ============================================================
 * @file turboxfs.h
 * @brief Turbox 内存文件系统接口声明
 *
 * 层级：
 *   TurboBoxOS/fs/
 *
 * 项目内绝对路径：
 *   TurboBoxOS/fs/turboxfs.h
 *
 * 模块作用：
 *   定义内存文件系统的容量常量和最小接口集合。
 *   FS 作为独立服务模块存在，上层只通过这些接口委托调用，
 *   不直接接触文件表实现。
 *
 * 使用者：
 *   middle 层通过 IPC 适配调用本模块。
 *
 * 项目角色：
 *   fs 层的对外契约头，是文件系统服务与上层之间的唯一接口。
 *
 * 引入说明：
 *   不依赖其他头文件，纯接口和常量定义。
 *
 * 维护记录：
 *   2026-08-30 初始创建
 * ============================================================ */

#ifndef TURBOX_FS_TURBOXFS_H
#define TURBOX_FS_TURBOXFS_H

/* freestanding 环境下用静态数组替代动态容器，
 * 避免堆分配带来的复杂性和不确定性 */
#define TFS_MAX_FILES 64
#define TFS_NAME_LEN  32
#define TFS_MAX_SIZE  4096

/* 初始化文件系统，清空文件表。成功返回 0。 */
int  tfs_init();

/* 创建空文件。成功返回句柄，失败返回 -1。 */
int  tfs_create(const char* name);

/* 向文件追加写入数据。返回实际写入字节数，失败返回 -1。 */
int  tfs_write(int fd, const char* data, unsigned int len);

/* 读取文件内容到缓冲区。返回实际读取字节数，失败返回 -1。 */
int  tfs_read(int fd, char* buf, unsigned int len);

/* 按文件名解释运行文件内容，输出到 VGA。成功返回 0，失败返回 -1。 */
int  tfs_run(const char* name);

/* 列出所有文件名和大小到 VGA。无返回值。 */
void tfs_list();

/* 按句柄解释运行文件。成功返回 0，失败返回 -1。
 * 供 middle 层 IPC 服务在收到“运行 fd”消息后调用，
 * 是对外接口 tfs_run 的内部执行体。 */
int  tfs_run_fd(int fd);

#endif /* TURBOX_FS_TURBOXFS_H */