; Multiboot constants
MAGIC    equ 0x1BADB002
MBALIGN  equ 1 << 0
MEMINFO  equ 1 << 1
MBFLAGS  equ MBALIGN | MEMINFO
CHECKSUM equ -(MAGIC + MBFLAGS)

%define PG_SZ                4096
%define HIGHER_HALF_OFFSET   0xC0000000
%define _KERNEL_CODE_SEG_SEL gdt_kernel_code_seg_desc - gdt_start
%define _KERNEL_DATA_SEG_SEL gdt_kernel_data_seg_desc - gdt_start
%define _TASK_STATE_SEG_SEL  gdt_task_state_seg_desc - gdt_start
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
    mov esp, __pa(kstack_top)

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
    mov esp, kstack_top

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

    ; Set up TSS
    mov dword [task_state_seg + 4], kstack_top
    mov word [task_state_seg + 8], _KERNEL_DATA_SEG_SEL

    ; Set up TSS descriptor
    mov word [gdt_task_state_seg_desc + 0], task_state_seg_end - task_state_seg_start - 1
    mov eax, task_state_seg
    mov word [gdt_task_state_seg_desc + 2], ax
    shr eax, 16
    mov byte [gdt_task_state_seg_desc + 4], al
    mov byte [gdt_task_state_seg_desc + 5], 0x89
    mov byte [gdt_task_state_seg_desc + 6], 0x00
    mov byte [gdt_task_state_seg_desc + 7], ah

    ; Load TSS
    mov ax, _TASK_STATE_SEG_SEL
    ltr ax
    
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
alignb 4
task_state_seg_start:

global task_state_seg
task_state_seg:
    resb 108

task_state_seg_end:

alignb 16
kstack_bottom:
    resb 8192
global kstack_top
kstack_top:
