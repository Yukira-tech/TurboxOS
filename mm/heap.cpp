/* ============================================================
 * @file heap.cpp
 * @brief 内核堆分配器实现
 *
 * 层级：
 *   TurboBoxOS/mm/
 *
 * 项目内绝对路径：
 *   TurboBoxOS/mm/heap.cpp
 *
 * 模块作用：
 *   基于显式空闲链表实现 kmalloc/kfree。采用首次适配，
 *   释放时合并相邻空闲块。后备存储来自 pmm 整页分配，
 *   堆不够时按页扩张。
 *
 * 使用者：
 *   kernel、fs、middle 等需要动态内存的内核模块。
 *
 * 项目角色：
 *   mm 层堆管理实现，提供内核运行时动态内存能力。
 *
 * 引入说明：
 *   依赖 mm.h 声明接口。
 *   依赖 pmm_alloc_page 提供物理页。
 *   依赖 runtime 提供 memset。
 *
 * 维护记录：
 *   2026-08-30 初始创建
 * ============================================================ */

#include "mm.h"

// runtime 自研 memset，freestanding 环境无 libc
extern "C" void* memset(void* dst, int val, unsigned int n);

/* ---- 块头结构 ---- */

struct HeapBlock {
    unsigned int size;  // 本块总字节数，含块头
    int          free;  // 1=空闲，0=已分配
    HeapBlock*   next;  // 下一块，链表按地址递增
};

#define HEAP_HDR_SIZE ((unsigned int)sizeof(HeapBlock))

// 剩余空间至少还能放一个块头加 16 字节才值得分裂
#define HEAP_MIN_SPLIT (HEAP_HDR_SIZE + 16u)

static HeapBlock* g_heap_head = 0;  // 空闲/占用混合链表头，0 表示堆尚未建立

// 向上对齐到 4 字节，避免非对齐访存，同时让块头保持整齐
static unsigned int align4(unsigned int n)
{
    return (n + 3u) & ~3u;
}

/*
 * 向 pmm 申请整页并加入堆链表尾部。
 * 多页请求会拆成多个独立块挂入，合并逻辑后续自然整理。
 */
static int heap_grow(unsigned int need)
{
    unsigned int want = align4(need) + HEAP_HDR_SIZE;
    unsigned int pages = (want + TURBOX_PAGE_SIZE - 1u) / TURBOX_PAGE_SIZE;
    if (pages == 0) pages = 1;

    void* base = pmm_alloc_page();
    if (base == 0) return 0;

    HeapBlock* blk = (HeapBlock*)base;
    blk->size = TURBOX_PAGE_SIZE;
    blk->free = 1;

    // 挂到链表尾部，保持地址递增
    if (g_heap_head == 0) {
        g_heap_head = blk;
        blk->next = 0;
    } else {
        HeapBlock* cur = g_heap_head;
        while (cur->next != 0) cur = cur->next;
        cur->next = blk;
        blk->next = 0;

        // 尾块和新块物理相邻，立即合并
        if ((unsigned char*)cur + cur->size == (unsigned char*)blk) {
            cur->size += blk->size;
            cur->next = 0;
            blk = cur;
        }
    }

    // 多页请求的剩余页各自独立挂入
    for (unsigned int i = 1; i < pages; ++i) {
        void* pg = pmm_alloc_page();
        if (pg == 0) break;  // 部分增长也比失败强

        HeapBlock* b = (HeapBlock*)pg;
        b->size = TURBOX_PAGE_SIZE;
        b->free = 1;

        HeapBlock* cur = g_heap_head;
        while (cur->next != 0) cur = cur->next;
        cur->next = b;
        b->next = 0;

        if ((unsigned char*)cur + cur->size == (unsigned char*)b) {
            cur->size += b->size;
            cur->next = 0;
        }
    }
    return 1;
}

/*
 * 分配 size 字节，首次适配。第一遍找不到会扩张堆再扫一遍。
 * 返回用户区指针，失败返回 0。
 */
void* kmalloc(unsigned int size)
{
    if (size == 0) return 0;
    unsigned int want = align4(size);

    for (int pass = 0; pass < 2; ++pass) {
        HeapBlock* cur = g_heap_head;
        while (cur != 0) {
            if (cur->free && cur->size >= want + HEAP_HDR_SIZE) {
                // 剩余空间足够就分裂，避免大块被小请求独占
                if (cur->size >= want + HEAP_HDR_SIZE + HEAP_MIN_SPLIT) {
                    HeapBlock* rest = (HeapBlock*)((unsigned char*)cur + HEAP_HDR_SIZE + want);
                    rest->size = cur->size - HEAP_HDR_SIZE - want;
                    rest->free = 1;
                    rest->next = cur->next;
                    cur->size = HEAP_HDR_SIZE + want;
                    cur->next = rest;
                }
                cur->free = 0;
                return (unsigned char*)cur + HEAP_HDR_SIZE;
            }
            cur = cur->next;
        }
        if (pass == 0 && !heap_grow(want)) return 0;
    }
    return 0;
}

/*
 * 释放 p 指向的用户区，并把块标记为空闲。
 * 然后全链表扫描，合并物理相邻且都空闲的块。
 */
void kfree(void* p)
{
    if (p == 0) return;

    HeapBlock* blk = (HeapBlock*)((unsigned char*)p - HEAP_HDR_SIZE);
    blk->free = 1;

    HeapBlock* cur = g_heap_head;
    while (cur != 0 && cur->next != 0) {
        if (cur->free && cur->next->free &&
            (unsigned char*)cur + cur->size == (unsigned char*)cur->next) {
            cur->size += cur->next->size;
            cur->next = cur->next->next;
            continue;  // 合并后 cur 可能还能和新的 next 合并
        }
        cur = cur->next;
    }

    // memset 在此处暂无必要，保留声明避免编译警告
    (void)memset;
}