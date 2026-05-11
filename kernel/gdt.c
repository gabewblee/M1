#include "gdt.h"

#include "../lib/string.h"

typedef struct task_state_seg_t {
    u32         link       : 16;             /* Previous TSS selector */
    u32         reserved0  : 16;
    virt_addr_t esp0       : 32;             /* Ring 0 stack pointer */
    u32         ss0        : 16;             /* Ring 0 stack segment selector */
    u32         reserved1  : 16;
    virt_addr_t esp1       : 32;             /* Ring 1 stack pointer */
    u32         ss1        : 16;             /* Ring 1 stack segment selector */
    u32         reserved2  : 16;
    virt_addr_t esp2       : 32;             /* Ring 2 stack pointer */
    u32         ss2        : 16;             /* Ring 2 stack segment selector */
    u32         reserved3  : 16;
    phys_addr_t cr3        : 32;             /* Page directory base register */
    virt_addr_t eip        : 32;             /* Instruction pointer */
    u32         eflags     : 32;             /* EFLAGS register */
    u32         eax        : 32;             /* EAX register */
    u32         ecx        : 32;             /* ECX register */
    u32         edx        : 32;             /* EDX register */
    u32         ebx        : 32;             /* EBX register */
    u32         esp        : 32;             /* ESP register */
    u32         ebp        : 32;             /* EBP register */
    u32         esi        : 32;             /* ESI register */
    u32         edi        : 32;             /* EDI register */
    u32         es         : 16;             /* ES segment selector */
    u32         reserved4  : 16;
    u32         cs         : 16;             /* CS segment selector */
    u32         reserved5  : 16;
    u32         ss         : 16;             /* SS segment selector */
    u32         reserved6  : 16;
    u32         ds         : 16;             /* DS segment selector */
    u32         reserved7  : 16;
    u32         fs         : 16;             /* FS segment selector */
    u32         reserved8  : 16;
    u32         gs         : 16;             /* GS segment selector */
    u32         reserved9  : 16;
    u32         ldtr       : 16;             /* LDT segment selector */
    u32         reserved10 : 16;
    u32         reserved11 : 16;
    u32         iopb       : 16;             /* I/O map base address */
    u32         ssp        : 32;             /* Shadow stack pointer */
} __packed task_state_seg_t;

typedef struct gdt_seg_desc_t {
    u16 limit_low  : 16;                     /* Low 16 bits of the segment limit */
    u16 base_low   : 16;                     /* Low 16 bits of the segment base */
    u8  base_mid   : 8;                      /* Middle 8 bits of the segment base */
    u8  access     : 8;                      /* Segment descriptor access byte */
    u8  limit_high : 4;                      /* High 4 bits of the segment limit */
    u8  flags      : 4;                      /* Segment descriptor flags */
    u8  base_high  : 8;                      /* High 8 bits of the segment base */
} __packed gdt_seg_desc_t;

task_state_seg_t task_state_seg;             /* TSS */
extern u8        gdt_start[];                /* Start of the GDT in virtual memory */
extern u8        gdt_kernel_data_seg_desc[]; /* Start of the KDS descriptor in virtual memory */
extern u8        gdt_task_state_seg_desc[];  /* Start of the TSS descriptor in virtual memory */
extern u8        kernel_stack_top[];         /* End of the kernel stack in virtual memory */

static gdt_seg_desc_t* gdt_get_seg_desc(u16 selector) {
    return &((gdt_seg_desc_t*)gdt_start)[selector >> 3];
}

static void gdt_set_seg_desc(gdt_seg_desc_t* desc, u32 limit, virt_addr_t base, u8 access, u8 flags) {
    desc->limit_low  = limit & 0xFFFF;
    desc->base_low   = base & 0xFFFF;
    desc->base_mid   = (base >> 16) & 0xFF;
    desc->access     = access;
    desc->limit_high = (limit >> 16) & 0xF;
    desc->flags      = flags & 0xF;
    desc->base_high  = (base >> 24) & 0xFF;
}

void __init gdt_init(void) {
    task_state_seg.ss0  = gdt_kernel_data_seg_desc - gdt_start;
    task_state_seg.esp0 = (virt_addr_t)kernel_stack_top;

    gdt_seg_desc_t* task_state_seg_desc = gdt_get_seg_desc(gdt_task_state_seg_desc - gdt_start);
    gdt_set_seg_desc(task_state_seg_desc, sizeof(task_state_seg_t) - 1, (virt_addr_t)&task_state_seg, 0x89, 0x0);
    
    u16 task_state_seg_sel = gdt_task_state_seg_desc - gdt_start;
    __asm__ volatile("ltr %0" : : "r"(task_state_seg_sel));
}