#pragma once

#include "uapi/compiler.h"
#include "uapi/types.h"

/* System configurations                 */
#define PG_SZ            4096       /* Page size                    */
#define CONSOLE_MAX_CNT  8          /* Maximum console count        */

/* Klog subsystem configurations         */
#define KLOG_BUF_SZ      4096       /* Kernel log ring buffer size  */

/* EVGA subsystem configurations         */
#define EVGA_BLACK_COLOR 0x00       /* EVGA color black             */
#define EVGA_WHITE_COLOR 0x0F       /* EVGA color white             */
#define EVGA_PHYS_ADDR   0xB8000    /* EVGA buffer physical address */

/* IDT subsystem configurations          */
#define IDT_EXC_CNT      32         /* CPU exception count          */
#define IDT_IRQ_CNT      16         /* PIC IRQ count                */
#define IDT_VEC_MAX_CNT  256        /* Maximum vector count         */

/* IPC subsystem configurations          */
#define IPC_QUEUE_CAP    16         /* IPC queue capacity           */

/* Block device subsystem configurations */
#define BLK_DEV_MAX_CNT   8         /* Maximum block device count   */

/* Task subsystem configurations         */
#define TASK_MAX_CNT     64         /* Maximum task count           */

/* Thread subsystem configurations       */
#define THREAD_KSTACK_SZ 8192       /* Kernel stack size            */
#define THREAD_MAX_CNT   256        /* Maximum thread count         */

/* PMM subsystem configurations          */
#define FRM_MAX_CNT      (1u << 20) /* Maximum frame count          */

#define ENTER_CRIT_SEC(flags) \
    u32 flags;                \
    __asm__ volatile("pushf; pop %0; cli" : "=r"(flags) : : "memory")

#define EXIT_CRIT_SEC(flags)  \
    __asm__ volatile("push %0; popf" : : "r"(flags) : "memory", "cc")
