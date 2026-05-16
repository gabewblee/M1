#include "gdt.h"

#include "../lib/string.h"

typedef struct task_state_seg_t {
    u32         link       : 16;
    u32         reserved0  : 16;
    virt_addr_t esp0       : 32;
    u32         ss0        : 16;
    u32         reserved1  : 16;
    virt_addr_t esp1       : 32;
    u32         ss1        : 16;
    u32         reserved2  : 16;
    virt_addr_t esp2       : 32;
    u32         ss2        : 16;
    u32         reserved3  : 16;
    phys_addr_t cr3        : 32;
    virt_addr_t eip        : 32;
    u32         eflags     : 32;
    u32         eax        : 32;
    u32         ecx        : 32;
    u32         edx        : 32;
    u32         ebx        : 32;
    u32         esp        : 32;
    u32         ebp        : 32;
    u32         esi        : 32;
    u32         edi        : 32;
    u32         es         : 16;
    u32         reserved4  : 16;
    u32         cs         : 16;
    u32         reserved5  : 16;
    u32         ss         : 16;
    u32         reserved6  : 16;
    u32         ds         : 16;
    u32         reserved7  : 16;
    u32         fs         : 16;
    u32         reserved8  : 16;
    u32         gs         : 16;
    u32         reserved9  : 16;
    u32         ldtr       : 16;
    u32         reserved10 : 16;
    u32         reserved11 : 16;
    u32         iopb       : 16;
    u32         ssp        : 32;
} __packed task_state_seg_t;

typedef struct gdt_seg_desc_t {
    u16 limit_low  : 16;
    u16 base_low   : 16;
    u8  base_mid   : 8;
    u8  access     : 8;
    u8  limit_high : 4;
    u8  flags      : 4;
    u8  base_high  : 8;
} __packed gdt_seg_desc_t;

task_state_seg_t task_state_seg;

extern u8 gdt_start[];
extern u8 gdt_kernel_data_seg_desc[];
extern u8 gdt_task_state_seg_desc[];
extern u8 kernel_stack_top[];

static gdt_seg_desc_t* get_seg_desc(u16 selector) {
    return &((gdt_seg_desc_t*)gdt_start)[selector >> 3];
}

static void set_seg_desc(gdt_seg_desc_t* desc, u32 limit, virt_addr_t base, u8 access, u8 flags) {
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

    gdt_seg_desc_t* task_state_seg_desc = get_seg_desc(gdt_task_state_seg_desc - gdt_start);
    set_seg_desc(task_state_seg_desc, sizeof(task_state_seg_t) - 1, (virt_addr_t)&task_state_seg, 0x89, 0x0);
    
    u16 task_state_seg_sel = gdt_task_state_seg_desc - gdt_start;
    __asm__ volatile("ltr %0" : : "r"(task_state_seg_sel));
}