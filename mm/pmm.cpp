/* ============================================================
 * @file pmm.cpp
 * @brief 物理页分配器（位图法）
 *
 * 层级：
 *   TurboBoxOS/mm/
 *
 * 项目内绝对路径：
 *   TurboBoxOS/mm/pmm.cpp
 *
 * 模块作用：
 *   用位图管理物理内存，每页占 1 bit。分配时线性扫描首个空闲位，
 *   释放时清位即可。前 256 页固定保留，避免覆盖 BIOS 数据区、
 *   显存和内核装载区。
 *
 * 使用者：
 *   kernel 启动时调用 pmm_init，heap.cpp 通过 pmm_alloc_page
 *   获取堆扩展所需物理页。
 *
 * 项目角色：
 *   mm 层最底层的内存管理者，是堆分配器的物理内存来源。
 *
 * 引入说明：
 *   依赖 mm.h 声明接口。
 *   依赖 runtime 提供的 memset。
 *
 * 维护记录：
 *   2026-08-30 初始创建
 * ============================================================ */

#include "mm.h"

// freestanding 下无 libc，自研 memset
extern "C" void* memset(void* dst, int val, unsigned int n);

/* ---- 内部状态 ---- */

// 最大管理 64MiB => 16384 页 => 位图 2048 字节
#define PMM_MAX_PAGES  16384u
#define PMM_BITMAP_LEN (PMM_MAX_PAGES / 8u)

// 前 1MiB = 256 页保留给 BIOS、VGA 和内核自身
#define PMM_RESERVED_PAGES 256u

static unsigned char  g_bitmap[PMM_BITMAP_LEN]; // 1=占用，0=空闲
static unsigned int   g_total_pages = 0;        // 实际可用页总数
static unsigned int   g_free_pages  = 0;        // 空闲页计数，便于诊断
static unsigned int   g_last_hint   = 0;        // 下次分配扫描起点

/* ---- 位图辅助 ---- */

static void bm_set(unsigned int i)   { g_bitmap[i >> 3] |=  (unsigned char)(1u << (i & 7u)); }
static void bm_clear(unsigned int i) { g_bitmap[i >> 3] &= (unsigned char)~(1u << (i & 7u)); }
static int  bm_test(unsigned int i)  { return (g_bitmap[i >> 3] >> (i & 7u)) & 1u; }

/*
 * 按可用 KiB 数初始化位图。
 * 先全部置 1，再放行保留区之后的页，这样位图尾部超出
 * g_total_pages 的部分天然是占用态，不会被误分配。
 */
void pmm_init(unsigned int mem_kb)
{
    g_total_pages = mem_kb / (TURBOX_PAGE_SIZE / 1024u);
    if (g_total_pages > PMM_MAX_PAGES) g_total_pages = PMM_MAX_PAGES;

    memset(g_bitmap, 0xFF, PMM_BITMAP_LEN);

    for (unsigned int i = PMM_RESERVED_PAGES; i < g_total_pages; ++i)
        bm_clear(i);

    g_free_pages = (g_total_pages > PMM_RESERVED_PAGES)
                 ? (g_total_pages - PMM_RESERVED_PAGES) : 0u;

    // 从保留区之后开始扫，跳过永远占用的前 256 页
    g_last_hint = PMM_RESERVED_PAGES;
}

/*
 * 分配一个物理页并清零后返回地址。
 * 从上次位置环形扫描，避免每次都从头开始。耗尽返回 0。
 */
void* pmm_alloc_page()
{
    for (unsigned int off = 0; off < g_total_pages; ++off) {
        unsigned int i = g_last_hint + off;
        if (i >= g_total_pages) i -= g_total_pages;  // 环形回绕
        if (i < PMM_RESERVED_PAGES) continue;        // 保留区永不分配

        if (bm_test(i) == 0) {
            bm_set(i);
            --g_free_pages;
            g_last_hint = i + 1u;  // 下次从下一页开始找

            // 经 unsigned long 中转，消除 64 位宿主编译器截断告警
            void* p = (void*)(unsigned long)(i * TURBOX_PAGE_SIZE);

            // 清零交付，防止读到上一个使用者的残留数据
            memset(p, 0, TURBOX_PAGE_SIZE);
            return p;
        }
    }
    return 0;  // 物理内存耗尽
}

/*
 * 释放物理页。
 * 只接受页对齐地址且未被保留、未被双重释放的页。
 */
void pmm_free_page(void* p)
{
    if (p == 0) return;

    unsigned int addr = (unsigned int)(unsigned long)p;

    // 非页对齐说明传了堆内指针，直接忽略
    if ((addr & (TURBOX_PAGE_SIZE - 1u)) != 0u) return;

    unsigned int i = addr / TURBOX_PAGE_SIZE;

    // 越界或保留区不释放
    if (i < PMM_RESERVED_PAGES || i >= g_total_pages) return;

    // 双重释放防御
    if (bm_test(i) == 0) return;

    bm_clear(i);
    ++g_free_pages;

    // 释放位置很可能马上又空闲，作为扫描提示加速下次分配
    if (i < g_last_hint) g_last_hint = i;
}