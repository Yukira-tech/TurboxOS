; ============================================================
; @file syscall_stub.asm
; @brief int 0x80 系统调用汇编入口桩
;
; 层级：
;   TurboBoxOS/kernel/
;
; 项目内绝对路径：
;   TurboBoxOS/kernel/syscall_stub.asm
;
; 模块作用：
;   用户态 API 通过 int 0x80 陷入内核。本模块负责保存用户态现场，
;   把寄存器里的调用号和参数传给 C++ 的 syscall_dispatch，
;   再用 iretd 返回用户态。
;
; 使用者：
;   hw/idt.cpp 把 0x80 向量指向本入口。
;   os-api 层通过 int 0x80 触发。
;
; 项目角色：
;   kernel 层唯一的汇编入口，是用户态与内核态之间的桥。
;
; 引入说明：
;   依赖 kernel.cpp 提供 extern "C" 的 syscall_dispatch 符号。
;
; 维护记录：
;   2026-08-30 初始创建
; ============================================================

BITS 32

extern syscall_dispatch          ; C++ 分发函数，位于 kernel.cpp

global isr_syscall_stub

section .text

; ------------------------------------------------------------
; int 0x80 入口。
; 调用约定：
;   eax = 系统调用号
;   ebx = 参数1
;   ecx = 参数2
;   edx = 参数3
; ------------------------------------------------------------
isr_syscall_stub:
    pushad                       ; 保存 8 个通用寄存器，防止 C++ 破坏用户态现场

    push edx                     ; a3 先压栈
    push ecx                     ; a2
    push ebx                     ; a1
    push eax                     ; num（系统调用号）

    call syscall_dispatch        ; 返回后结果在 eax

    add  esp, 16                 ; 清掉 4 个参数，恢复 pushad 后的栈顶

    ; 把返回值写回 pushad 保存的 eax 区域。
    ; pushad 保存的 eax 位于当前 esp 偏移 28 处，
    ; 这样 popad 之后用户看到的 eax 就是系统调用返回值。
    mov  [esp + 28], eax

    popad                        ; 恢复全部寄存器，eax 已是返回值
    iretd                        ; 从陷阱门返回用户态