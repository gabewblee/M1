#pragma once

#include "compiler.h"
#include "types.h"

/* System configurations           */
#define PG_SZ             4096       /* Page size                   */
#define CONSOLE_MAX_CNT   8          /* Maximum console count       */

/* Klog subsystem configurations   */
#define KLOG_BUF_SZ       4096       /* Kernel log ring buffer size */

/* VGA subsystem configurations    */
#define VGA_COLOR_BLACK   0x00       /* VGA color black             */
#define VGA_COLOR_WHITE   0x0F       /* VGA color white             */
#define VGA_PHYS_ADDR     0xB8000    /* VGA buffer physical address */

/* IDT subsystem configurations    */
#define IDT_EXCEPTION_CNT 32         /* CPU exception count         */
#define IDT_IRQ_CNT       16         /* PIC IRQ count               */
#define IDT_DESC_CNT      256        /* IDT descriptor count        */

/* IPC subsystem configurations    */
#define IPC_INLINE_SZ     56         /* IPC inline size             */
#define IPC_QUEUE_CAP     16         /* IPC queue capacity          */

/* Task subsystem configurations   */
#define TASK_MAX_CNT      64         /* Maximum task count          */

/* Thread subsystem configurations */
#define THREAD_KSTACK_SZ  8192       /* Kernel stack size           */
#define THREAD_MAX_CNT    256        /* Maximum thread count        */

/* PMM subsystem configurations    */
#define FRM_MAX_CNT       (1u << 20) /* Maximum frame count         */

#define ENTER_CRIT_SEC(flags) \
    u32 flags;                \
    __asm__ volatile("pushf; pop %0; cli" : "=r"(flags) : : "memory")

#define EXIT_CRIT_SEC(flags)  \
    __asm__ volatile("push %0; popf" : : "r"(flags) : "memory", "cc")
