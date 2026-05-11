; Multiboot constants
MAGIC    equ 0x1BADB002
MBALIGN  equ 1 << 0
MEMINFO  equ 1 << 1
MBFLAGS  equ MBALIGN | MEMINFO
CHECKSUM equ -(MAGIC + MBFLAGS)

; Higher half kernel macros
%define HIGHER_HALF_OFFSET       0xC0000000
%define __pa(x)                  ((x) - HIGHER_HALF_OFFSET)

; GDT macros
%define KERNEL_CODE_SEG_SELECTOR gdt_kernel_code_seg_desc - gdt_start
%define KERNEL_DATA_SEG_SELECTOR gdt_kernel_data_seg_desc - gdt_start

; System macros
%define KERNEL_NUM_PG_TABLES     2
%define PG_SZ                    4096

; Multiboot header
section .multiboot
align 4
    dd MAGIC
    dd MBFLAGS
    dd CHECKSUM

; Global entry point
global _start
_start:
    ; Save magic and multiboot information
    mov dword [__pa(magic)], eax
    mov dword [__pa(mbi)], ebx

    ; Set up stack
    mov esp, __pa(kernel_stack_top)

    ; Load page directory
    mov eax, __pa(kernel_pg_dir)
    mov cr3, eax

    ; Enable paging
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax

    ; Enter higher half kernel
    jmp higher_half_kernel

section .init
align PG_SZ
kernel_pg_dir:
    %assign i 0
    %rep KERNEL_NUM_PG_TABLES
        dd __pa(identity_pg_table_%+i) + 3
    %assign i i+1
    %endrep

    times (768 - KERNEL_NUM_PG_TABLES) dd 0

    %assign i 0
    %rep KERNEL_NUM_PG_TABLES
        dd __pa(kernel_pg_table_%+i) + 3
    %assign i i+1
    %endrep

align PG_SZ
%macro identity_pg_table_stub 1
identity_pg_table_%+%1:
%assign pg (%1 * 1024)
%rep 1024
    dd (pg * PG_SZ) + 3
    %assign pg pg+1
%endrep
%endmacro

align PG_SZ
%macro kernel_pg_table_stub 1
kernel_pg_table_%+%1:
%assign pg (%1 * 1024)
%rep 1024
    dd (pg * PG_SZ) + 3
    %assign pg pg+1
%endrep
%endmacro

%assign i 0
%rep KERNEL_NUM_PG_TABLES
    identity_pg_table_stub i
    kernel_pg_table_stub i
%assign i i+1
%endrep

higher_half_kernel:
    ; Unmap identity mapping
    %assign i 0
    %rep KERNEL_NUM_PG_TABLES
        mov dword [kernel_pg_dir + (i * 4)], 0
    %assign i i+1
    %endrep

    ; Flush TLB
    mov eax, cr3
    mov cr3, eax

    ; Set up stack
    mov esp, kernel_stack_top

    ; GDT setup
    lgdt [gdt_desc]
    jmp KERNEL_CODE_SEG_SELECTOR:.reload
        
.reload:
    ; Reload segment registers
    mov ax, KERNEL_DATA_SEG_SELECTOR
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Enter C code
    extern kmain
    call kmain
    cli

.hang:
    hlt
    jmp .hang

section .data
global magic
magic:
    dd 0

global mbi
mbi:
    dd 0

gdt_desc:
    dw gdt_end - gdt_start - 1
    dd gdt_start

align 4
global gdt_start
gdt_start:
gdt_null_desc:
    dq 0x0000000000000000

global gdt_kernel_code_seg_desc
gdt_kernel_code_seg_desc:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0x9A
    db 0xCF
    db 0x00

global gdt_kernel_data_seg_desc
gdt_kernel_data_seg_desc:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0x92
    db 0xCF
    db 0x00

gdt_user_code_seg_desc:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0xFA
    db 0xCF
    db 0x00

gdt_user_data_seg_desc:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0xF2
    db 0xCF
    db 0x00

global gdt_task_state_seg_desc
gdt_task_state_seg_desc:
    dq 0x0000000000000000

gdt_end:

section .bss
align 16
kernel_stack_bottom:
    resb 8192
global kernel_stack_top
kernel_stack_top:
