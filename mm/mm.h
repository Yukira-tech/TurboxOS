/* ============================================================
 * @file mm.h
 * @brief 物理页分配器与内核堆接口声明
 *
 * 层级：
 *   TurboBoxOS/mm/
 *
 * 项目内绝对路径：
 *   TurboBoxOS/mm/mm.h
 *
 * 模块作用：
 *   声明物理页分配器和内核堆的对外接口。pmm 以 4KiB 页为粒度
 *   管理物理内存，heap 在页分配器之上提供任意小粒度分配，
 *   两层分离便于单独替换策略。
 *
 * 使用者：
 *   kernel、fs、middle 等需要内存管理的内核模块。
 *
 * 项目角色：
 *   mm 层的对外契约头，是内核内存管理能力的统一入口。
 *
 * 引入说明：
 *   不依赖其他头文件。
 *
 * 维护记录：
 *   2026-08-30 初始创建
 * ============================================================ */

#ifndef TURBOX_MM_H
#define TURBOX_MM_H

/* 以下签名是 SPEC 4.3 接口契约，不得修改 */

/* 初始化物理页位图，参数为可用物理内存 KiB 数 */
void  pmm_init(unsigned int mem_kb);

/* 分配一个 4KiB 物理页，失败返回 0 */
void* pmm_alloc_page();

/* 释放物理页 */
void  pmm_free_page(void* p);

/* 内核堆分配，失败返回 0 */
void* kmalloc(unsigned int size);

/* 内核堆释放 */
void  kfree(void* p);

/* 页大小常量，供内核其他模块做对齐运算 */
#define TURBOX_PAGE_SIZE 4096u

#endif /* TURBOX_MM_H */