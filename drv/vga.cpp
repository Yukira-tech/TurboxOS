/* ============================================================
 * @file vga.cpp
 * @brief VGA 文本模式驱动
 *
 * 层级：
 *   TurboBoxOS/drv/
 *
 * 项目内绝对路径：
 *   TurboBoxOS/drv/vga.cpp
 *
 * 模块作用：
 *   直接写 0xB8000 显存实现 80x25 文本输出，
 *   支持换行、退格、自动折行和滚屏。
 *
 * 使用者：
 *   kernel、user/shell 等模块通过 drv.h 调用本模块。
 *
 * 项目角色：
 *   系统最底层的字符输出设备，保护模式下唯一稳定可靠的人机交互输出。
 *
 * 引入说明：
 *   依赖 drv.h 的接口声明，不依赖其他硬件模块。
 *
 * 维护记录：
 *   2026-08-30 初始创建
 * ============================================================ */

#include "drv.h"

// 文本模式显存物理地址，保护模式下线性映射后可直接写
#define VGA_MEM   ((volatile unsigned short*)0xB8000)
#define VGA_COLS  80
#define VGA_ROWS  25

// 属性字节：黑底浅灰字，和经典 DOS 终端一致
#define VGA_ATTR  0x07

static unsigned int g_col = 0;  // 光标列
static unsigned int g_row = 0;  // 光标行

/*
 * 屏幕整体上滚一行，末行用空格清空。
 * 只在光标越出底边时调用，避免频繁搬移显存。
 */
static void vga_scroll()
{
    // 把第 1..24 行整体搬到第 0..23 行
    for (unsigned int r = 1; r < VGA_ROWS; ++r) {
        for (unsigned int c = 0; c < VGA_COLS; ++c) {
            VGA_MEM[(r - 1) * VGA_COLS + c] = VGA_MEM[r * VGA_COLS + c];
        }
    }

    // 末行填带属性的空格，清掉上一屏残留
    for (unsigned int c = 0; c < VGA_COLS; ++c) {
        VGA_MEM[(VGA_ROWS - 1) * VGA_COLS + c] =
            (unsigned short)((VGA_ATTR << 8) | ' ');
    }

    g_row = VGA_ROWS - 1;
}

/*
 * 清屏并把光标归到左上角。
 * 所有 VGA 输出前必须先调用一次。
 */
void vga_init()
{
    for (unsigned int i = 0; i < VGA_COLS * VGA_ROWS; ++i) {
        VGA_MEM[i] = (unsigned short)((VGA_ATTR << 8) | ' ');
    }
    g_col = 0;
    g_row = 0;
}

/*
 * 输出一个字符。
 * '\n' 换行，'\b' 左移光标但不擦字符，其余直接写显存。
 */
void vga_putc(char c)
{
    if (c == '\n') {
        // 换行只归零列并下行，不处理回车，让上层语义更简单
        g_col = 0;
        ++g_row;
    } else if (c == '\b') {
        // 退格只移动光标，不清字符，方便 shell 做行编辑
        if (g_col > 0) {
            --g_col;
        }
    } else {
        VGA_MEM[g_row * VGA_COLS + g_col] =
            (unsigned short)((VGA_ATTR << 8) | (unsigned char)c);
        ++g_col;

        if (g_col >= VGA_COLS) {
            // 行尾自动折行，防止写出显存
            g_col = 0;
            ++g_row;
        }
    }

    if (g_row >= VGA_ROWS) {
        vga_scroll();  // 光标越界才滚屏，减少显存搬移
    }
}

/*
 * 输出以 '\0' 结尾的字符串。
 * 内部逐个字符交给 vga_putc，换行和滚屏规则完全一致。
 */
void vga_puts(const char* s)
{
    while (*s != '\0') {
        vga_putc(*s++);
    }
}