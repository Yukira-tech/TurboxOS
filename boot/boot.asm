; ============================================================
; @file boot.asm
; @brief BIOS 启动扇区，16 位实模式入口
;
; 层级：
;   TurboBoxOS/boot/
;
; 项目内绝对路径：
;   TurboBoxOS/boot/boot.asm
;
; 模块作用：
;   系统上电后第一个被 BIOS 加载执行的模块。引导扇区只有 512 字节，
;   放不下完整内核，所以这里的任务是先把 loader 和内核从磁盘搬到
;   内存安全区，再跳转过去交接控制权。
;
; 使用者：
;   BIOS 启动流程直接调用，不依赖其他模块。
;
; 项目角色：
;   系统启动第一环，从 16 位实模式开始，完成后续代码的磁盘加载。
;
; 引入说明：
;   完全自包含，不依赖其他文件或符号。
;
; 维护记录：
;   2026-08-30 初始创建
; ============================================================

BITS 16                     ; BIOS 启动时 CPU 处于 16 位实模式

; BIOS 规范固定将启动扇区加载到物理地址 0x7C00
ORG 0x7C00

; loader + 内核的加载目标段地址
; 0x1000:0x0000 => 物理地址 0x10000
; 选择 1MB 以内的内存，是为了避开 VGA 等硬件映射区，保证读写安全
KERNEL_LOAD_SEG  equ 0x1000

; 读取扇区数：loader + 内核总大小上限为 16KB
; 后续内核增长超过此值时必须同步调大
KERNEL_SECTORS   equ 32

start:
    cli                     ; 关中断，避免设置栈期间被外部中断打断
    xor ax, ax
    mov ds, ax              ; 数据段清零，保证段:偏移寻址一致
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00          ; 栈指针放在启动扇区下方，栈向下生长，不会覆盖自身代码
    sti

    mov [boot_drive], dl    ; DL 由 BIOS 传入启动盘号，后续读盘还要用，必须保存

; ------------------------------------------------------------
; 读取磁盘：CHS 模式
; 从第 2 扇区开始读，因为第 1 扇区就是当前引导扇区本身
; ES:BX 指向目标内存 0x1000:0x0000 = 物理地址 0x10000
; ------------------------------------------------------------
    mov ax, KERNEL_LOAD_SEG
    mov es, ax
    xor bx, bx
    mov ah, 0x02            ; int 13h 功能号 0x02 = 读扇区
    mov al, KERNEL_SECTORS  ; 一次读取的扇区数
    mov ch, 0               ; 柱面 0
    mov cl, 2               ; 扇区号从 1 开始，第 1 扇区是引导扇区，故从 2 开始
    mov dh, 0               ; 磁头 0
    mov dl, [boot_drive]    ; 用保存的启动盘号
    int 0x13
    jc disk_error           ; CF=1 表示读盘失败，此时绝不能继续启动

    ; 远跳转把控制权交给 loader，入口就在 0x10000
    jmp KERNEL_LOAD_SEG:0x0000

disk_error:
    mov si, err_msg
.print:
    lodsb
    or al, al
    jz .halt
    mov ah, 0x0E            ; BIOS 电传打字输出，不依赖显存初始化，最可靠
    int 0x10
    jmp .print
.halt:
    cli
    hlt                     ; 停机等人工干预，避免带着坏内核继续执行
    jmp .halt

boot_drive db 0
err_msg    db "Disk read error!", 13, 10, 0

; 启动扇区必须恰好 512 字节，不足部分用 0 填充
times 510 - ($ - $$) db 0

; BIOS 启动签名，缺少此标志 BIOS 不会认为该盘可启动
dw 0xAA55