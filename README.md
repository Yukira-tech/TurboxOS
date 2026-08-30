# TurboxOS

<p align="center">
  <a href="#en"><img src="https://img.shields.io/badge/English-0078D4?style=for-the-badge&logo=readme&logoColor=white" alt="English"></a>
  <a href="#zh"><img src="https://img.shields.io/badge/中文-FF6B00?style=for-the-badge&logo=readme&logoColor=white" alt="中文"></a>
</p>

---

<a id="en"></a>
# English Version

> Toy minimal micro-kernel OS for education demo. Single-threaded, simple custom filesystem, bootloader support. No network. Just a hobby teaching toy OS.

## TABLE OF CONTENTS

1. [What Is This](#1-what-is-this)
2. [Features](#2-features)
3. [Tech Stack](#3-tech-stack)
4. [Directory Structure](#4-directory-structure)
5. [Build & Run](#5-build--run)
6. [Web Demo](#6-web-demo)
7. [Known Limitations](#7-known-limitations)
8. [Learning Value](#8-learning-value)
9. [Documentation](#9-documentation)
10. [License](#10-license)

---

## 1. WHAT IS THIS

Turbox OS is a minimal micro-kernel operating system built for **educational purposes**. It is not a production system, not a Unix clone, not a performance benchmark. It exists to answer one question:

> What happens between pressing the power button and seeing a shell prompt?

Every layer is implemented from scratch, with readability prioritized over performance.

---

## 2. FEATURES

| Feature | Description |
|---------|-------------|
| Micro-kernel Structure | Kernel only keeps process, IPC, scheduling, memory. Filesystem runs as a separate service process |
| Cooperative Scheduling | Single-threaded, no preemption, processes are coroutines |
| Bitmap Physical Allocator | 4KiB pages, first 1MiB reserved, double-free detection |
| Free-list Heap Allocator | First-fit with block splitting and adjacent coalescing |
| Custom Filesystem | TurboxFS: static file table, create/write/read/run/list |
| Bootloader Support | Two-stage boot, real mode → protected mode → C++ kernel |
| System Call Layer | `int 0x80` trap, unified ABI, user-side API wrappers |
| IPC | Simplified Minix-style rendezvous, single-word mailbox |
| Web Terminal | Browser-based terminal emulator with dual-mode sync/fallback |

---

## 3. TECH STACK

| Layer | Tech | Responsibility |
|-------|------|----------------|
| boot | NASM assembly | Real-mode boot, A20, GDT, protected-mode jump |
| kernel | C++ / assembly | Process, IPC, scheduling, syscall dispatch |
| mm | C++ | Physical page allocator, kernel heap |
| fs | C++ | Memory filesystem + mini script interpreter |
| drv | C++ | VGA text output, PS/2 keyboard |
| hal | C++ | Hardware abstraction + PIT timer |
| middle | C++ | VFS adapter + IPC filesystem service |
| user | C++ | Shell and demo user programs |
| tools | Python | Build scripts, image packing, data bridging |
| web | HTML/CSS/JS | Terminal emulator |

---

## 4. DIRECTORY STRUCTURE

```
TurboBoxOS/
├── boot/       # Boot sector and loader
├── drv/        # Device drivers
├── fs/         # TurboxFS memory filesystem
├── hal/        # Hardware abstraction layer
├── hw/         # GDT/IDT/ports
├── kernel/     # Micro-kernel core
├── middle/     # VFS and IPC adapter
├── mm/         # Memory management
├── runtime/    # Custom string/memory functions
├── syscall/    # System call wrappers
├── tools/      # Build and bridge tools
├── user/       # Shell and demo programs
├── web/        # Web terminal
└── linker.ld   # Linker script
```

---

## 5. BUILD & RUN

### Requirements

- NASM
- i686-elf cross-compilation toolchain (gcc/g++/ld)

### Build

```bash
python tools/build.py
python tools/pack_image.py
```

Outputs: `turbox.bin` kernel binary, `turbox.img` bootable image.

### Run

```bash
qemu-system-i386 -fda turbox.img
```

After boot, you'll enter a shell. Type `help` to see available commands.

---

## 6. WEB DEMO

You can try the terminal without booting the kernel:

```bash
python tools/bridge.py --demo
```

Then open `web/index.html` in a browser. The terminal supports `create`, `write`, `run`, `ls`, `cat` commands, emulating TurboxFS.

---

## 7. KNOWN LIMITATIONS

- No network
- No preemptive scheduling
- No user/kernel address space isolation
- No disk filesystem
- No graphical interface
- Physical memory capped at 64MiB
- File table fixed at 64 files, max 4096 bytes per file

These are deliberate teaching trade-offs, not bugs.

---

## 8. LEARNING VALUE

This project walks through the core path of an operating system:

```
BIOS → bootloader → protected mode → C++ kernel → process
     → IPC → filesystem → user program
```

Each layer keeps only the minimal implementation. Code is readable, auditable, and easy to experiment with. Suitable for:

- Understanding x86 boot flow
- Understanding micro-kernel and Minix-style IPC
- Understanding memory management and heap allocation
- Understanding system calls and user API design
- Understanding filesystem abstraction

---

## 9. DOCUMENTATION

- [SPEC.md](docs/SPEC.md) — Complete system specification

---

## 10. LICENSE

MIT License. See `LICENSE` for details.

---

<p align="center"><a href="#top">Back to Top ↑</a></p>

---

<hr>

<a id="zh"></a>
# 中文版本

> 教学演示用的微型内核操作系统。单线程，极简自定义文件系统，支持引导加载。无网络。只是一个兴趣向的教学玩具 OS。

## 目录

1. [这是什么](#1-这是什么)
2. [特性](#2-特性)
3. [技术栈](#3-技术栈)
4. [目录结构](#4-目录结构)
5. [构建与运行](#5-构建与运行)
6. [Web 演示](#6-web-演示)
7. [已知限制](#7-已知限制)
8. [学习价值](#8-学习价值)
9. [文档](#9-文档)
10. [许可证](#10-许可证)

---

## 1. 这是什么

Turbox OS 是一个为**教学目的**而写的微型内核。它不是生产系统，不是 Unix 克隆，也不是性能测试。它只回答一个问题：

> 从按下电源键到看到 shell 提示符，中间发生了什么？

每一层都从零实现，可读性优先于性能。

---

## 2. 特性

| 特性 | 说明 |
|------|------|
| 微内核结构 | 内核只保留进程、IPC、调度、内存。文件系统作为独立服务进程运行 |
| 协作式调度 | 单线程，无抢占，进程即协程 |
| 位图物理页分配器 | 4KiB 页，前 1MiB 保留，带双重释放检测 |
| 空闲链表堆分配器 | 首次适配，支持块分裂和相邻合并 |
| 自定义文件系统 | TurboxFS：静态文件表，支持 create/write/read/run/list |
| Bootloader 支持 | 两阶段启动，实模式 → 保护模式 → C++ 内核 |
| 系统调用层 | `int 0x80` 陷入，统一 ABI，用户态 API 封装 |
| IPC | 简化版 Minix 风格 rendezvous，单字信箱 |
| Web 终端 | 浏览器终端模拟器，支持同步/回退双模式 |

---

## 3. 技术栈

| 层 | 技术 | 职责 |
|----|------|------|
| boot | NASM 汇编 | 实模式引导，A20，GDT，保护模式跳转 |
| kernel | C++ / 汇编 | 进程、IPC、调度、系统调用分发 |
| mm | C++ | 物理页分配、内核堆 |
| fs | C++ | 内存文件系统 + mini 脚本解释器 |
| drv | C++ | VGA 文本输出、PS/2 键盘 |
| hal | C++ | 硬件抽象 + PIT 定时器 |
| middle | C++ | VFS 适配 + IPC 文件系统服务 |
| user | C++ | Shell 和 demo 用户程序 |
| tools | Python | 构建脚本、镜像打包、数据桥接 |
| web | HTML/CSS/JS | 终端模拟器 |

---

## 4. 目录结构

```
TurboBoxOS/
├── boot/       # 引导扇区和加载器
├── drv/        # 设备驱动
├── fs/         # TurboxFS 内存文件系统
├── hal/        # 硬件抽象层
├── hw/         # GDT/IDT/端口
├── kernel/     # 微内核核心
├── middle/     # VFS 和 IPC 适配
├── mm/         # 内存管理
├── runtime/    # 自研字符串/内存函数
├── syscall/    # 系统调用封装
├── tools/      # 构建和桥接工具
├── user/       # Shell 和 demo
├── web/        # 网页终端
└── linker.ld   # 链接脚本
```

---

## 5. 构建与运行

### 依赖

- NASM
- i686-elf 交叉编译工具链（gcc/g++/ld）

### 构建

```bash
python tools/build.py
python tools/pack_image.py
```

产物：`turbox.bin` 内核二进制，`turbox.img` 启动镜像。

### 运行

```bash
qemu-system-i386 -fda turbox.img
```

启动后进入 shell，输入 `help` 查看命令。

---

## 6. Web 演示

不启动内核也能体验终端：

```bash
python tools/bridge.py --demo
```

然后打开 `web/index.html`。浏览器里的终端支持 `create`、`write`、`run`、`ls`、`cat` 等命令，模拟 TurboxFS。

---

## 7. 已知限制

- 无网络
- 无抢占式调度
- 无用户态/内核态地址空间隔离
- 无磁盘文件系统
- 无图形界面
- 物理内存上限 64MiB
- 文件表固定 64 个文件，单文件最大 4096 字节

这些都是刻意的教学取舍，不是缺陷。

---

## 8. 学习价值

这个项目完整走了一遍操作系统最核心的路径：

```
BIOS → bootloader → 保护模式 → C++ 内核 → 进程
     → IPC → 文件系统 → 用户程序
```

每一层都只保留最小实现，代码可读、可审计、可实验。适合：

- 理解 x86 启动流程
- 理解微内核和 Minix 风格 IPC
- 理解内存管理和堆分配
- 理解系统调用和用户 API 设计
- 理解文件系统抽象

---

## 9. 文档

- [SPEC.md](docs/SPEC.md) — 完整系统规范

---

## 10. 许可证

MIT 许可证。详见 `LICENSE` 文件。

---

<p align="center"><a href="#top">返回顶部 ↑</a></p>

---

<p align="center">
  <b>保持好奇，保持学习。理解底层，才能走得更远。</b>
</p>