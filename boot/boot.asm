; Multiboot constants
MAGIC    equ 0x1BADB002
MBALIGN  equ 1 << 0
MEMINFO  equ 1 << 1
MBFLAGS  equ MBALIGN | MEMINFO
CHECKSUM equ -(MAGIC + MBFLAGS)

%define HIGHER_HALF_OFFSET   0xC0000000
%define PG_SZ                4096
%define _KERNEL_CODE_SEG_SEL gdt_kernel_code_seg_desc - gdt_start
%define _KERNEL_DATA_SEG_SEL gdt_kernel_data_seg_desc - gdt_start
%define __pa(x)              ((x) - HIGHER_HALF_OFFSET)

extern swapper_pg_dir
extern swapper_pg_table_cnt
extern setup_swapper_pg_dir

section .multiboot
align 4
    dd MAGIC
    dd MBFLAGS
    dd CHECKSUM

global _start
_start:
    ; Save magic and multiboot information
    mov dword [__pa(magic)], eax
    mov dword [__pa(mbi)], ebx

    ; Set up stack
    mov esp, __pa(kernel_stack_top)

    ; Initialize swapper kernel page directory
    call setup_swapper_pg_dir

    ; Load swapper kernel page directory
    mov eax, __pa(swapper_pg_dir)
    mov cr3, eax

    ; Enable paging
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax

    ; Enter higher half kernel
    jmp higher_half_kernel

section .init
higher_half_kernel:
    mov ecx, [swapper_pg_table_cnt]
    xor edi, edi

.unmap_identity_pg_tables:
    cmp edi, ecx
    jge .done
    mov dword [swapper_pg_dir + edi * 4], 0
    inc edi
    jmp .unmap_identity_pg_tables
    
.done:
    ; Flush TLB
    mov eax, cr3
    mov cr3, eax

    ; Set up stack
    mov esp, kernel_stack_top

    ; GDT setup
    lgdt [gdt_desc]
    jmp _KERNEL_CODE_SEG_SEL:.reload
        
.reload:
    ; Reload segment registers
    mov ax, _KERNEL_DATA_SEG_SEL 
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

; Global Descriptor Table descriptor (GDT descriptor)
gdt_desc:
    dw gdt_end - gdt_start - 1
    dd gdt_start

align 4
; Global Descriptor Table start (GDT start)
global gdt_start
gdt_start:

; Null descriptor
gdt_null_desc:
    dq 0x0000000000000000

; Kernel Code Segment descriptor (KCS descriptor)
global gdt_kernel_code_seg_desc
gdt_kernel_code_seg_desc:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0x9A
    db 0xCF
    db 0x00

; Kernel Data Segment descriptor (KDS descriptor)
global gdt_kernel_data_seg_desc
gdt_kernel_data_seg_desc:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0x92
    db 0xCF
    db 0x00

; User Code Segment descriptor (UCS descriptor)
gdt_user_code_seg_desc:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0xFA
    db 0xCF
    db 0x00

; User Data Segment descriptor (UDS descriptor)
gdt_user_data_seg_desc:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0xF2
    db 0xCF
    db 0x00

; Task State Segment descriptor (TSS descriptor)
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
